/* see end of file for fragments */

#include "encap.h"

#if NENCAP > 0

#define MAX_SECRET_LEN 1024
#define SIGLEN 4
#define IDLEN 1
#define ENC_DEFAULT_MTU 1024
#define IDPAD (8-(SIGLEN+IDLEN))

#if (SIGLEN+IDLEN+IDPAD) % 8
#error IDPAD is wrong!
#endif

#include "opt_inet.h"
#ifndef INET
#error ENCAP without INET?
#endif

#include <sys/tty.h> /* for ENCAP_DEBUG */
#include <net/if_pppvar.h> /* for ENCAP_DEBUG */

#include <sys/sha1.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/sockio.h>
#include <sys/kernel.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/if_types.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <machine/stdarg.h>
#include <netinet/ip_icmp.h>
#include <dev/pseudo/if_encap.h>

#ifdef ENCAP_REMOTE_REBOOT
#include <sys/reboot.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <sys/time.h>
#endif

/* What the hell...?  Why doesn't SHA1Update already take a const void *
   as its second argument?! */
#define SHA1Update(h,b,n) (SHA1Update)((h),(const void *)(b),(n))

#define SHA1_HASHLEN 20

typedef struct encap_softc SOFTC;
typedef struct encap_rt RT;
typedef struct encap_list LIST;
typedef struct encap_dlist DLIST;
typedef struct encap_listentry LISTENTRY;
typedef struct encap_dlistentry DLISTENTRY;
typedef struct encap_poison POISON;

static SOFTC *softc;
static int nencap;
static unsigned int flags;

#ifdef STONE_ENCAP_HACKS

/* Incoming:
	permit anything to stone port 4444
	block anything to or from an RFC1918 address
	block anything loaded with ENCAP_BLOCK
	block anything poisoned by touching poisoned addresses
   Outgoing:
	permit anything from stone port 4444
	block anything to or from an RFC1918 address
	block anything loaded with ENCAP_BLOCK
	block anything poisoned by touching poisoned addresses
*/

#define STONE_O 0xd82e0509
#define STONE_PORT 4444
#define PORTMAP_PORT 111
#define NETBIOS_NS_PORT 137

static unsigned int nextstamp;

static __inline__ int rfc1918addr(unsigned long int a)
{
 return( ((a & 0xff000000) == 0x0a000000) ||	/* 10.0.0.0/8 */
	 ((a & 0xfff00000) == 0xac100000) ||	/* 172.16.0.0/12 */
	 ((a & 0xffff0000) == 0xc0a80000) );	/* 192.168.0.0/16 */
}

static LISTENTRY *listmember(unsigned long int a, LIST *l)
{
 int i;

 for (i=l->len-1;i>=0;i--) if ((a & l->list[i].mask) == l->list[i].net) return(&l->list[i]);
 return(0);
}

static DLISTENTRY *dlistmember(unsigned long int a, DLIST *l)
{
 int i;

 for (i=l->len-1;i>=0;i--) if ((a & l->list[i].mask) == l->list[i].net) return(&l->list[i]);
 return(0);
}

static void init_list(LIST *l, SOFTC *sc)
{
 l->len = 0;
 l->list = 0;
}

static void init_dlist(DLIST *l, SOFTC *sc)
{
 l->sc = sc;
 l->len = 0;
 l->alloc = 0;
 l->list = 0;
 l->flags = 0;
}

static int add_poisoned(DLIST *l, unsigned int addr, unsigned int mask, void *poison)
{
 int s;
 int i;
 DLISTENTRY *e;

 printf("[add_poisoned ");
 s = splhigh();
 if (l->flags & EDLF_LOCK)
  { splx(s);
    printf("locked] ");
    return(0);
  }
 l->flags |= EDLF_LOCK;
 splx(s);
 for (i=l->len-1;i>=0;i--)
  { e = &l->list[i];
    if (e->mask & ~mask) continue;
    if ((e->net ^ addr) & mask) continue;
    printf("already] ");
    goto unlock_ret;
  }
 if (l->len >= l->alloc)
  { void *new;
    void *old;
    printf("grow [a=%d,l=%d] ",l->alloc,l->len);
    new = malloc((l->len+1)*sizeof(DLISTENTRY),M_DEVBUF,M_NOWAIT);
    if (new == 0)
     { printf("no mem] ");
       goto unlock_ret;
     }
    old = l->list;
    if (old) bcopy(old,new,l->len*sizeof(DLISTENTRY));
    s = splhigh();
    l->alloc = l->len + 1;
    l->list = new;
    splx(s);
    if (old) free(old,M_DEVBUF);
  }
 else
  { printf("%d/%d ",l->len,l->alloc);
  }
 e = &l->list[l->len];
 e->net = addr;
 e->mask = mask;
 e->stamp = nextstamp++;
 e->poison = poison;
 s = splhigh();
 l->len ++;
 splx(s);
 printf("ok] ");
 s = splhigh();
 l->flags &= ~EDLF_LOCK;
 splx(s);
 return(1);
unlock_ret:;
 s = splhigh();
 l->flags &= ~EDLF_LOCK;
 splx(s);
 return(0);
}

static void remove_poisoned(DLIST *l, unsigned int addr, unsigned int mask)
{
 int s;
 int i;
 DLISTENTRY *e;

 printf("remove_poisoned %08x",addr);
 if (l->len < 1)
  { printf(" empty\n");
    return;
  }
 s = splhigh();
 if (l->flags & EDLF_LOCK)
  { splx(s);
    printf(" locked\n");
    return;
  }
 l->flags |= EDLF_LOCK;
 splx(s);
 if (l->len < 1)
  { printf(" empty!\n");
    goto unlock_ret;
  }
 for (i=l->len-1;i>=0;i--)
  { e = &l->list[i];
    if ((e->net != addr) || (e->mask != mask)) continue;
    printf(" %d",i);
    s = splhigh();
    if (i != l->len-1) l->list[i] = l->list[l->len-1];
    l->len --;
    splx(s);
  }
 printf(" done\n");
unlock_ret:;
 s = splhigh();
 l->flags &= ~EDLF_LOCK;
 splx(s);
}

static void unpoison_address(void *vp)
{
 POISON *p;

 p = vp;
 remove_poisoned(&p->sc->poisoned,p->addr,0xffffffff);
 free(p,M_TEMP);
}

static void remove_oldest(DLIST *l)
{
 int s;
 int i;
 int ox;
 DLISTENTRY *e;
 DLISTENTRY *oe;

 printf("[remove_oldest ");
 if (l->len < 1)
  { printf("len=%d] ",l->len);
    return;
  }
 s = splhigh();
 if (l->flags & EDLF_LOCK)
  { splx(s);
    printf("locked] ");
    return;
  }
 l->flags |= EDLF_LOCK;
 splx(s);
 if (l->len < 1)
  { printf("len=%d!] ",l->len);
    goto unlock_ret;
  }
 ox = 0;
 oe = &l->list[0];
 for (i=l->len-1;i>0;i--)
  { e = &l->list[i];
    if (e->stamp < oe->stamp)
     { ox = i;
       oe = e;
     }
  }
 printf("ox=%d",ox);
 if (oe->poison)
  { untimeout(unpoison_address,oe->poison);
    free(oe->poison,M_TEMP);
  }
 s = splhigh();
 if (ox != l->len-1) l->list[ox] = l->list[l->len-1];
 l->len --;
 splx(s);
 printf("] ");
unlock_ret:;
 s = splhigh();
 l->flags &= ~EDLF_LOCK;
 splx(s);
}

static void poison_address(SOFTC *sc, unsigned long int va, unsigned long int pa, int pp, const char *prototag)
{
 void *t;
 POISON *p;
 DLISTENTRY *dle;

 printf("poison %08lx",va);
 if (pa) printf(" %08lx",pa);
 if (prototag) printf(" %s",prototag);
 if (pp != -1) printf(" %d",pp);
 printf(": ");
 dle = dlistmember(va,&sc->poisoned);
 if (dle && dle->poison)
  { untimeout(unpoison_address,dle->poison);
    timeout(unpoison_address,dle->poison,24*60*60*hz);
    printf("freshen\n");
    return;
  }
 t = malloc(sizeof(POISON),M_TEMP,M_NOWAIT);
 if (t == 0)
  { printf("no mem\n");
    return;
  }
 p = t;
 p->sc = sc;
 p->addr = va;
 timeout(unpoison_address,p,24*60*60*hz);
 if (sc->poisoned.len >= 64) remove_oldest(&sc->poisoned);
 if (! add_poisoned(&sc->poisoned,va,0xffffffff,p)) free(t,M_TEMP);
 printf("done\n");
}

static struct mbuf *stone_encap_incoming(SOFTC *sc, struct mbuf *m)
{
 struct ip *ip;
 struct tcphdr *th;
 struct udphdr *uh;
 struct icmp *ih;
 struct ipovly *io;
 struct ipovly iosave;
 struct ipovly ionew;
 int recksum;
 unsigned long int dstaddr;
 unsigned short int dstport;
 unsigned long int srcaddr;

 m = m_pullup(m,sizeof(struct ip));
 if (m == 0) return(0);
 ip = mtod(m,struct ip *);
 recksum = 0;
 srcaddr = ntohl(ip->ip_src.s_addr);
 if (rfc1918addr(srcaddr))
  { m_freem(m);
    return(0);
  }
 dstaddr = ntohl(ip->ip_dst.s_addr);
 if (rfc1918addr(dstaddr))
  { m_freem(m);
    return(0);
  }
 /* Don't try to look at protocol headers except for first fragments.
    It's possible in principle for a first fragment to be too short to
    hold the full header of a perfectly legitimate packet; we don't worry
    about that case, because TCP and UDP headers are small enough that
    such a packet in practice probably means someone is trying to sneak
    something nefarious past us. */
 if ((ip->ip_off & IP_OFFMASK) == 0)
  { switch (ip->ip_p)
     { case IPPROTO_UDP:
	  m = m_pullup(m,(ip->ip_hl<<2)+sizeof(struct udphdr));
	  if (m == 0) return(0);
	  ip = mtod(m,struct ip *);
	  uh = (struct udphdr *) (mtod(m,char *) + (ip->ip_hl<<2));
	  dstport = ntohs(uh->uh_dport);
	  break;
       case IPPROTO_TCP:
	  m = m_pullup(m,(ip->ip_hl<<2)+sizeof(struct tcphdr));
	  if (m == 0) return(0);
	  ip = mtod(m,struct ip *);
	  th = (struct tcphdr *) (mtod(m,char *) + (ip->ip_hl<<2));
	  dstport = ntohs(th->th_dport);
	  switch (dstport)
	   { case STONE_PORT:
		if (dstaddr == STONE_O) return(m);
		break;
	   }
	  break;
       case IPPROTO_ICMP:
	  m = m_pullup(m,(ip->ip_hl<<2)+ICMP_MINLEN);
	  if (m == 0) return(0);
	  ip = mtod(m,struct ip *);
	  ih = (struct icmp *) (mtod(m,char *) + (ip->ip_hl<<2));
	  break;
     }
  }
 if (listmember(srcaddr,&sc->block))
  { m_freem(m);
    return(0);
  }
 if ((ip->ip_off & IP_OFFMASK) == 0)
  { switch (ip->ip_p)
     { case IPPROTO_UDP:
	  switch (dstport)
	   { case NETBIOS_NS_PORT:
	     case PORTMAP_PORT:
		poison_address(sc,srcaddr,dstaddr,dstport,"udp");
		m_freem(m);
		return(0);
		break;
	   }
	  break;
       case IPPROTO_TCP:
	  switch (dstport)
	   { case PORTMAP_PORT:
		poison_address(sc,srcaddr,dstaddr,dstport,"tcp");
		m_freem(m);
		return(0);
		break;
	   }
	  break;
     }
  }
 if (listmember(dstaddr,&sc->poison))
  { const char *s;
    s = 0;
    if ((ip->ip_off & IP_OFFMASK) == 0)
     { switch (ip->ip_p)
	{ case IPPROTO_UDP:
	     poison_address(sc,srcaddr,dstaddr,dstport,"udp");
	     break;
	  case IPPROTO_TCP:
	     poison_address(sc,srcaddr,dstaddr,dstport,"tcp");
	     break;
	  case IPPROTO_ICMP:
	     switch (ih->icmp_type)
	      { case ICMP_ECHOREPLY:
		   s = "icmp echoreply";
		   break;
		case ICMP_UNREACH:
		   switch (ih->icmp_code)
		    { case ICMP_UNREACH_NET: s = "icmp unreach net"; break;
		      case ICMP_UNREACH_HOST: s = "icmp unreach host"; break;
		      case ICMP_UNREACH_PROTOCOL: s = "icmp unreach proto"; break;
		      case ICMP_UNREACH_PORT: s = "icmp unreach port"; break;
		      case ICMP_UNREACH_NEEDFRAG: s = "icmp unreach frag"; break;
		      case ICMP_UNREACH_SRCFAIL: s = "icmp unreach srcrt"; break;
		      case ICMP_UNREACH_NET_UNKNOWN: s = "icmp unreach net unknown"; break;
		      case ICMP_UNREACH_HOST_UNKNOWN: s = "icmp unreach host unknown"; break;
		      case ICMP_UNREACH_ISOLATED: s = "icmp unreach isolated"; break;
		      case ICMP_UNREACH_NET_PROHIB: s = "icmp unreach net prohib"; break;
		      case ICMP_UNREACH_HOST_PROHIB: s = "icmp unreach host prohib"; break;
		      case ICMP_UNREACH_TOSNET: s = "icmp unreach net TOS"; break;
		      case ICMP_UNREACH_TOSHOST: s = "icmp unreach host TOS"; break;
		      case ICMP_UNREACH_ADMIN_PROHIBIT: s = "icmp unreach admin prohib"; break;
		    }
		   poison_address(sc,srcaddr,dstaddr,s?-1:ih->icmp_code,s?:"icmp code");
		   s = 0;
		   break;
		case ICMP_SOURCEQUENCH:
		   s = "icmp sourcequench";
		   break;
		case ICMP_REDIRECT:
		   switch (ih->icmp_code)
		    { case ICMP_REDIRECT_NET: s = "icmp redir net"; break;
		      case ICMP_REDIRECT_HOST: s = "icmp redir host"; break;
		      case ICMP_REDIRECT_TOSNET: s = "icmp redir TOS/net"; break;
		      case ICMP_REDIRECT_TOSHOST: s = "icmp redir TOS/host"; break;
		    }
		   poison_address(sc,srcaddr,dstaddr,s?-1:ih->icmp_code,s?:"icmp redir");
		   s = 0;
		   break;
		case ICMP_ECHO:
		   s = "icmp echo";
		   break;
		case ICMP_ROUTERADVERT:
		   s = "icmp router advertisement";
		   break;
		case ICMP_ROUTERSOLICIT:
		   s = "icmp router solicitation";
		   break;
		case ICMP_TIMXCEED:
		   switch (ih->icmp_code)
		    { case ICMP_TIMXCEED_INTRANS: s = "icmp timxceed intrans"; break;
		      case ICMP_TIMXCEED_REASS: s = "icmp timxceed reass"; break;
		    }
		   poison_address(sc,srcaddr,dstaddr,s?-1:ih->icmp_code,s?:"icmp timxceed");
		   s = 0;
		   break;
		case ICMP_PARAMPROB:
		   switch (ih->icmp_code)
		    { case ICMP_PARAMPROB_OPTABSENT: s = "icmp required option absent"; break;
		    }
		   poison_address(sc,srcaddr,dstaddr,s?-1:ih->icmp_code,s?:"icmp parameter problem");
		   s = 0;
		   break;
		case ICMP_TSTAMP:
		   s = "icmp timestamp request";
		   break;
		case ICMP_TSTAMPREPLY:
		   s = "icmp timestamp reply";
		   break;
		case ICMP_IREQ:
		   s = "icmp info request";
		   break;
		case ICMP_IREQREPLY:
		   s = "icmp info reply";
		   break;
		case ICMP_MASKREQ:
		   s = "icmp mask request";
		   break;
		case ICMP_MASKREPLY:
		   s = "icmp mask reply";
		   break;
		default:
		   poison_address(sc,srcaddr,dstaddr,ih->icmp_type,"icmp type");
		   break;
	      }
	     if (s) poison_address(sc,srcaddr,dstaddr,-1,s);
	     break;
	  default:
	     poison_address(sc,srcaddr,dstaddr,ip->ip_p,"proto");
	     break;
	}
     }
    else
     { poison_address(sc,srcaddr,dstaddr,-1,"fragment");
     }
    m_freem(m);
    return(0);
  }
 if (dlistmember(srcaddr,&sc->poisoned))
  { m_freem(m);
    return(0);
  }
 if (recksum)
  { int n;
    n = (ip->ip_hl<<2) - sizeof(struct ipovly);
    m->m_data += n;
    m->m_len -= n;
    io = mtod(m,struct ipovly *);
    iosave = *io;
    bzero(&ionew.ih_x1[0],sizeof(ionew.ih_x1));
    ionew.ih_pr = ip->ip_p;
    ionew.ih_len = ip->ip_len - (ip->ip_hl<<2);
    ionew.ih_src = ip->ip_src;
    ionew.ih_dst = ip->ip_dst;
    *io = ionew;
    th->th_sum = 0;
    th->th_sum = in_cksum(m,ionew.ih_len+sizeof(struct ipovly));
    *io = iosave;
    m->m_data -= n;
    m->m_len += n;
    ip->ip_sum = 0;
    ip->ip_sum = in_cksum(m,ip->ip_hl<<2);
  }
 return(m);
}

static struct mbuf *stone_encap_outgoing(SOFTC *sc, struct mbuf *m)
{
 struct ip *ip;
 struct tcphdr *th;
 struct ipovly *io;
 struct ipovly iosave;
 struct ipovly ionew;
 int recksum;
 unsigned long int srcaddr;
 unsigned short int srcport;
 unsigned long int dstaddr;

 m = m_pullup(m,sizeof(struct ip));
 if (m == 0) return(0);
 ip = mtod(m,struct ip *);
 recksum = 0;
 dstaddr = ntohl(ip->ip_dst.s_addr);
 if (rfc1918addr(dstaddr))
  { m_freem(m);
    return(0);
  }
 srcaddr = ntohl(ip->ip_src.s_addr);
 if (rfc1918addr(srcaddr))
  { m_freem(m);
    return(0);
  }
 if ((ip->ip_off & IP_OFFMASK) == 0)
  { switch (srcaddr)
     { case STONE_O:
	  if (ip->ip_p == IPPROTO_TCP)
	   { m = m_pullup(m,(ip->ip_hl<<2)+sizeof(struct tcphdr));
	     if (m == 0) return(0);
	     ip = mtod(m,struct ip *);
	     th = (struct tcphdr *) (mtod(m,char *) + (ip->ip_hl<<2));
	     srcport = ntohs(th->th_sport);
	     switch (srcport)
	      { case STONE_PORT:
		   return(m);
		   break;
	      }
	   }
	  break;
     }
  }
 if (listmember(dstaddr,&sc->block) || dlistmember(dstaddr,&sc->poisoned))
  { m_freem(m);
    return(0);
  }
 if (recksum)
  { int n;
    n = (ip->ip_hl<<2) - sizeof(struct ipovly);
    m->m_data += n;
    m->m_len -= n;
    io = mtod(m,struct ipovly *);
    iosave = *io;
    bzero(&ionew.ih_x1[0],sizeof(ionew.ih_x1));
    ionew.ih_pr = ip->ip_p;
    ionew.ih_len = ip->ip_len - (ip->ip_hl<<2);
    ionew.ih_src = ip->ip_src;
    ionew.ih_dst = ip->ip_dst;
    *io = ionew;
    th->th_sum = 0;
    th->th_sum = in_cksum(m,ionew.ih_len+sizeof(struct ipovly));
    *io = iosave;
    m->m_data -= n;
    m->m_len += n;
    ip->ip_sum = 0;
    ip->ip_sum = in_cksum(m,ip->ip_hl<<2);
  }
 return(m);
}

#endif

static void dbgdump(const void *data, int len)
{
 const unsigned char *dp;
 char buf[(16*3)+1];
 char *bp;
 int f;

 f = 0;
 bp = &buf[0];
 for (dp=data;len>0;dp++,len--)
  { *bp++ = ' ';
    *bp++ = "0123456789abcdef"[(*dp)>>4];
    *bp++ = "0123456789abcdef"[(*dp)&15];
    f ++;
    if (f >= 16)
     { *bp = '\0';
       printf("%s\n",&buf[0]);
       f = 0;
       bp = &buf[0];
     }
  }
 if (f > 0)
  { *bp = '\0';
    printf("%s\n",&buf[0]);
  }
}

static void compute_sig(RT *r, struct mbuf *m, int skip, void *sigloc, struct in_addr a1, struct in_addr a2)
{
 int o;
 SHA1_CTX hs;
 unsigned char sha1_sig[SHA1_HASHLEN];
 int i;

 SHA1Init(&hs);
 if (flags & ESF_D_SIG)
  { printf("compute_sig BEGIN\n");
    dbgdump(r->secret,r->secretlen);
  }
 SHA1Update(&hs,r->secret,r->secretlen);
 if (flags & ESF_D_SIG) dbgdump(&a1,sizeof(struct in_addr));
 SHA1Update(&hs,&a1,sizeof(struct in_addr));
 while (1)
  { if (m->m_len <= skip)
     { skip -= m->m_len;
       m = m->m_next;
       if (skip == 0)
	{ o = 0;
	  break;
	}
       if (m == 0) panic("encap: empty signature");
     }
    else
     { SHA1Update(&hs,mtod(m,char *)+skip,m->m_len-skip);
       if (flags & ESF_D_SIG) dbgdump(mtod(m,char *)+skip,m->m_len-skip);
       m = m->m_next;
       break;
     }
  }
 while (m)
  { SHA1Update(&hs,mtod(m,char *),m->m_len);
    if (flags & ESF_D_SIG) dbgdump(mtod(m,char *),m->m_len);
    m = m->m_next;
  }
 if (flags & ESF_D_SIG) dbgdump(&a2,sizeof(struct in_addr));
 SHA1Update(&hs,&a2,sizeof(struct in_addr));
 if (flags & ESF_D_SIG) dbgdump(r->secret,r->secretlen);
 SHA1Update(&hs,r->secret,r->secretlen);
 SHA1Final(&sha1_sig[0],&hs);
 if (flags & ESF_D_SIG)
  { printf("compute_sig RESULT\n");
    dbgdump(&sha1_sig[0],20);
  }
 i = 0;
 for (o=SIGLEN;o<SHA1_HASHLEN;o++)
  { sha1_sig[i] ^= sha1_sig[o];
    if (i >= SIGLEN-1) i = 0; else i ++;
  }
 if (flags & ESF_D_SIG)
  { printf("compute_sig SIG\n");
    dbgdump(&sha1_sig[0],SIGLEN);
    printf("compute_sig DONE\n");
  }
 memcpy(sigloc,&sha1_sig[0],SIGLEN);
}

/* m has already had the outer IP header stripped, but nothing more.
   It has been pulled up so that the signature is accessible with mtod. */
static int good_sig(RT *r, struct mbuf *m, struct in_addr pktsrc)
{
 unsigned char goodsig[SIGLEN];
 unsigned char *msig;
 int i;

 compute_sig(r,m,SIGLEN,&goodsig[0],pktsrc,r->src);
 msig = mtod(m,unsigned char *);
 for (i=0;i<SIGLEN;i++) if (msig[i] != goodsig[i]) return(0);
 return(1);
}

static RT *find_rt(SOFTC *sc, struct in_addr src)
{
 int i;
 RT *r;

 for (i=0;i<sc->nrt;i++)
  { r = sc->rts[i];
    if ( ((src.s_addr & r->srcmask.s_addr) == r->srcmatch.s_addr) &&
	 !in_nullhost(r->src) ) return(r);
  }
 return(0);
}

static int find_encap(struct in_addr src, struct in_addr dst, unsigned char id, SOFTC **scp, RT **rp)
{
 SOFTC *sc;
 int i;
 int j;
 RT *r;

 for (i=0,sc=softc;i<nencap;i++,sc++)
  { if (sc->ifnet.if_flags & IFF_UP)
     { for (j=0;j<sc->nrt;j++)
	{ r = sc->rts[j];
	  if ( (r->id == id) &&
	       in_hosteq(r->src,dst) &&
	       ( in_nullhost(r->dst) ||
		 in_hosteq(r->dst,src) ) )
	   { *scp = sc;
	     *rp = r;
	     if (flags & ESF_D_PKT)
	      { printf("find_encap: %08lx->%08lx id %d: unit %d rt %d\n",
			(unsigned long int)src.s_addr,(unsigned long int)dst.s_addr,id,i,j);
	      }
	     return(1);
	   }
	}
     }
  }
 if (flags & ESF_D_PKT)
  { printf("find_encap: %08lx->%08lx id %d: not found\n",(unsigned long int)src.s_addr,(unsigned long int)dst.s_addr,id);
  }
 return(0);
}

/*
 * Growl.  Where - besides the tcpdump source - is it documented that
 *  DLT_NULL means a 4-byte header containing the address family?!
 *
 * The kludge used here is shamelessly stolen from if_gif.c, where it
 *  is documented as "safe because bpf will only read from the mbuf
 *  (i.e., it won't try to free it or keep a pointer a to it)".
 */
static void encap_tap(caddr_t handle, struct mbuf *m0)
{
 u_int af;
 struct mbuf m;

 af = AF_INET;
 m.m_next = m0;
 m.m_len = 4;
 m.m_data = (void *) &af;
 bpf_mtap(handle,&m);
}

void encap_input(struct mbuf *m, ...)
{
 SOFTC *sc;
 RT *r;
 int hlen;
 va_list ap;
 int s;
 char *pp;
 struct ip *ip;
 struct in_addr pktsrc;

 va_start(ap,m);
 hlen = va_arg(ap,int);
 va_end(ap);
 m = m_pullup(m,sizeof(struct ip)+SIGLEN+IDLEN+IDPAD);
 if (m == 0) return;
 pp = mtod(m,char *);
 ip = (struct ip *) pp;
 pktsrc = ip->ip_src;
 if (find_encap(pktsrc,ip->ip_dst,pp[sizeof(struct ip)+SIGLEN],&sc,&r))
  { sc->ifnet.if_ipackets ++;
    sc->ifnet.if_ibytes += m->m_pkthdr.len;
    m_adj(m,sizeof(struct ip));
    if (! good_sig(r,m,pktsrc))
     { if (flags & ESF_D_PKT) printf("encap_input: bad sig\n");
       sc->ifnet.if_ierrors ++;
       m_freem(m);
     }
    else
     { if (flags & ESF_D_PKT) printf("encap_input: accepted\n");
       m_adj(m,SIGLEN+IDLEN+IDPAD);
       r->curdst = pktsrc;
       if (! (m->m_flags & M_PKTHDR)) panic("encap_input no HDR");
       if (m->m_pkthdr.len < 5)
	{ m = m_pullup(m,m->m_pkthdr.len);
	  if (m == 0) return;
#ifdef ENCAP_REMOTE_REBOOT
	  if ((m->m_pkthdr.len == 1) && (*mtod(m,char *) == 'r'))
	   { cpu_reboot(RB_NOSYNC,"");
	   }
#endif
	  m_freem(m);
	  return;
	}
#ifdef STONE_ENCAP_HACKS
       m = stone_encap_incoming(sc,m);
       if (m == 0) return;
       if (! (m->m_flags & M_PKTHDR)) panic("encap_input no HDR after hacks");
#endif
       m->m_pkthdr.rcvif = &sc->ifnet;
#if NBPFILTER > 0
       if (sc->ifnet.if_bpf) encap_tap(sc->ifnet.if_bpf,m);
#endif
       s = splimp();
       if (IF_QFULL(&ipintrq))
	{ IF_DROP(&ipintrq);
	  m_freem(m);
	}
       else
	{ IF_ENQUEUE(&ipintrq,m);
	  /* no schednetisr(); ipintr() will continue looping */
	}
       splx(s);
     }
  }
 else
  { m_freem(m);
  }
}

static int encap_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst, struct rtentry *rt)
{
 SOFTC *sc;
 RT *r;
 struct ip *ip;
 char *pp;
 struct ip *newip;

 if (dst->sa_family != AF_INET)
  { if (flags & ESF_D_PKT) printf("encap_output: not inet\n");
    IF_DROP(&ifp->if_snd);
    m_freem(m0);
    return(EAFNOSUPPORT);
  }
 if (! (ifp->if_flags & IFF_UP))
  { if (flags & ESF_D_PKT) printf("encap_output: not up\n");
    m_freem(m0);
    return(ENETDOWN);
  }
#if NBPFILTER > 0
 if (ifp->if_bpf) encap_tap(ifp->if_bpf,m0);
#endif
 sc = ifp->if_softc;
 ip = mtod(m0,struct ip *);
 r = find_rt(sc,ip->ip_src);
 if ((r == 0) || in_nullhost(r->curdst))
  { if (flags & ESF_D_PKT) printf("encap_output: no curdst\n");
    ifp->if_opackets ++;
    ifp->if_oerrors ++;
    m_freem(m0);
    return(0);
  }
 if (! (m0->m_flags & M_PKTHDR)) panic("encap_output no HDR");
#ifdef STONE_ENCAP_HACKS
 m0 = stone_encap_outgoing(sc,m0);
 if (m0 == 0) return(0);
 if (! (m0->m_flags & M_PKTHDR)) panic("encap_output no HDR after hacks");
#endif
 M_PREPEND(m0,sizeof(struct ip)+SIGLEN+IDLEN+IDPAD,M_DONTWAIT);
 if (m0 == 0) return(ENOBUFS);
 pp = mtod(m0,char *);
 newip = (struct ip *) pp;
 memset(newip,0,sizeof(*newip));
 newip->ip_hl = sizeof(*newip) >> 2;
 newip->ip_tos = ip->ip_tos;
 newip->ip_len = m0->m_pkthdr.len;
 newip->ip_ttl = r->ttl;
 newip->ip_p = IPPROTO_ENCAP;
 newip->ip_src = r->src;
 newip->ip_dst = r->curdst;
 pp[sizeof(struct ip)+SIGLEN] = r->id;
#if IDPAD > 0
  { int i;
    for (i=0;i<IDPAD;i++) pp[sizeof(struct ip)+SIGLEN+IDLEN+i] = 0;
  }
#endif
 compute_sig(r,m0,sizeof(struct ip)+SIGLEN,pp+sizeof(struct ip),r->src,r->curdst);
 ifp->if_opackets ++;
 ifp->if_obytes += m0->m_pkthdr.len;
 ip_output(m0,0,0,0,0);
 return(0);
}

static void send_magic_packet(SOFTC *sc, RT *r, const void *contents, int len)
{
 struct ip *ip;
 char *pp;
 struct mbuf *m;

 if (in_nullhost(r->src) || in_nullhost(r->curdst)) return;
 m = m_gethdr(M_WAIT,MT_DATA);
 if (sizeof(*ip)+SIGLEN+IDLEN+IDPAD+len > MHLEN) panic("encap: too-big magic packet");
 m->m_len = sizeof(*ip) + SIGLEN + IDLEN + IDPAD + len;
 pp = mtod(m,char *);
 ip = (struct ip *) pp;
 memset(ip,0,sizeof(*ip));
 ip->ip_hl = sizeof(*ip) >> 2;
 ip->ip_tos = ip->ip_tos;
 ip->ip_len = sizeof(*ip) + SIGLEN + IDLEN + IDPAD + len;
 ip->ip_ttl = r->ttl;
 ip->ip_p = IPPROTO_ENCAP;
 ip->ip_src = r->src;
 ip->ip_dst = r->curdst;
 pp[sizeof(struct ip)+SIGLEN] = r->id;
#if IDPAD > 0
  { int i;
    for (i=0;i<IDPAD;i++) pp[sizeof(struct ip)+SIGLEN+IDLEN+i] = 0;
  }
#endif
 if (len) bcopy(contents,pp+sizeof(*ip)+SIGLEN+IDLEN+IDPAD,len);
 compute_sig(r,m,sizeof(struct ip)+SIGLEN,pp+sizeof(struct ip),r->src,r->curdst);
 ip_output(m,0,0,0,0);
}

static int encap_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
 SOFTC *sc;
 int s;
 struct ifaddr *ifa;
 struct ifreq *ifr;
 int error;

 error = 0;
 sc = ifp->if_softc;
 s = splnet();
 switch (cmd)
  { case SIOCSIFADDR:
    case SIOCSIFDSTADDR:
       ifa = (void *) data;
       if (ifa->ifa_addr->sa_family != AF_INET)
	{ error = EAFNOSUPPORT;
	  break;
	}
       break;
    case SIOCSIFFLAGS:
       if (sc->nrt == 0) ifp->if_flags &= ~IFF_UP;
       break;
    case SIOCSIFMTU:
       ifr = (void *) data;
       ifp->if_mtu = ifr->ifr_mtu;
       break;
    default:
       error = EINVAL;
       break;
  }
 splx(s);
 return(error);
}

void encapattach(int count)
{
 SOFTC *sc;
 int i;

 nencap = count;
 softc = malloc(count*sizeof(SOFTC),M_DEVBUF,M_WAITOK);
 memset(softc,0,count*sizeof(SOFTC)); /* XXX */
 for (i=0;i<count;i++)
  { sc = softc + i;
    sprintf(sc->ifnet.if_xname,"encap%d",i);
    sc->ifnet.if_softc = sc;
    sc->ifnet.if_type = IFT_OTHER;
    sc->ifnet.if_addrlen = sizeof(struct in_addr);
    sc->ifnet.if_hdrlen = sizeof(struct ip) + SIGLEN + 1 + IDPAD;
    sc->ifnet.if_mtu = ENC_DEFAULT_MTU;
    sc->ifnet.if_flags = IFF_POINTOPOINT;
    sc->ifnet.if_output = encap_output;
    sc->ifnet.if_ioctl = encap_ioctl;
    sc->nrt = 0;
    sc->rts = 0;
    sc->flags = 0;
#ifdef STONE_ENCAP_HACKS
    init_list(&sc->block,sc);
    init_list(&sc->poison,sc);
    init_dlist(&sc->poisoned,sc);
    nextstamp = 0;
#endif
    if_attach(&sc->ifnet);
#if NBPFILTER > 0
    bpfattach(&sc->ifnet.if_bpf,&sc->ifnet,DLT_NULL,4);
#endif
  }
}

int encapopen(dev_t dev, int flag, int mode, struct proc *p)
{
 if (minor(dev) >= nencap) return(ENXIO);
 return(0);
}

int encapclose(dev_t dev, int flag, int mode, struct proc *p)
{
 return(0);
}

#ifdef STONE_ENCAP_HACKS
static int sglist_ioctl(LIST *l, void *data)
{
 struct encap_listio *arg;
 int newn;
 LISTENTRY *new;
 LIST old;
 int copyn;
 LISTENTRY *oldfree;
 int s;
 int err;

 arg = data;
 new = 0;
 newn = arg->newn;
 if (newn > 0)
  { new = malloc(newn*sizeof(*new),M_DEVBUF,M_WAITOK);
    err = copyin(arg->newlist,new,newn*sizeof(*new));
    if (err)
     { free(new,M_DEVBUF);
       return(err);
     }
  }
 oldfree = 0;
 s = splhigh();
 if (arg->oldn >= 0) old = *l;
 if (newn >= 0)
  { oldfree = l->list;
    l->len = newn;
    l->list = new;
  }
 splx(s);
 if (arg->oldn >= 0)
  { copyn = (old.len < arg->oldn) ? old.len : arg->oldn;
    err = copyout(old.list,arg->oldlist,copyn*sizeof(LISTENTRY));
    if (err) goto free_ret;
    arg->oldn = old.len;
  }
 err = 0;
free_ret:
 if (oldfree) free(oldfree,M_DEVBUF);
 return(err);
}
#endif

int encapioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 RT *dr;
 RT *scr;
 int err;
 char sbuf[MAX_SECRET_LEN];

 sc = softc + minor(dev);
 switch (cmd)
  { case ENCAP_GETNRT:
       if (! (flag & FREAD)) return(EBADF);
       *(unsigned int *)data = sc->nrt;
       return(0);
       break;
    case ENCAP_GETRT:
       if (! (flag & FREAD)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx >= sc->nrt) return(EDOM);
       scr = sc->rts[dr->inx];
       dr->srcmatch = scr->srcmatch;
       dr->srcmask = scr->srcmask;
       dr->src = scr->src;
       dr->dst = scr->dst;
       dr->curdst = scr->curdst;
       dr->id = scr->id;
       dr->ttl = scr->ttl;
	{ int len;
	  len = (scr->secretlen < dr->secretlen) ? scr->secretlen : dr->secretlen;
	  err = len ? copyout(scr->secret,dr->secret,len) : 0;
	}
       dr->secretlen = scr->secretlen;
       return(err);
       break;
    case ENCAP_SETRT:
       if (! (flag & FWRITE)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx > sc->nrt) return(EDOM);
       if (dr->secretlen > MAX_SECRET_LEN) return(EMSGSIZE);
       err = copyin(dr->secret,&sbuf[0],dr->secretlen);
       if (err) return(err);
       if (dr->inx == sc->nrt)
	{ RT **tmp;
	  tmp = malloc((sc->nrt+1)*sizeof(*tmp),M_DEVBUF,M_WAITOK);
	  if (tmp == 0) return(ENOBUFS);
	  tmp[sc->nrt] = 0;
	  if (sc->nrt > 0)
	   { memcpy(tmp,sc->rts,sc->nrt*sizeof(*tmp));
	     free(sc->rts,M_DEVBUF);
	   }
	  sc->nrt ++;
	  sc->rts = tmp;
	}
       scr = sc->rts[dr->inx];
       if (scr == 0)
	{ scr = malloc(sizeof(RT),M_DEVBUF,M_WAITOK);
	  if (scr == 0) return(ENOBUFS);
	  scr->inx = dr->inx;
	  scr->srcmatch.s_addr = 0;
	  scr->srcmask.s_addr = 0;
	  scr->src.s_addr = INADDR_ANY;
	  scr->dst.s_addr = INADDR_ANY;
	  scr->curdst.s_addr = INADDR_ANY;
	  scr->id = 0;
	  scr->ttl = 0;
	  scr->secret = 0;
	  scr->secretlen = 0;
	  sc->rts[dr->inx] = scr;
	}
       if (dr->secretlen == 0)
	{ if (scr->secret) free(scr->secret,M_DEVBUF);
	  scr->secret = 0;
	  scr->secretlen = 0;
	}
       else
	{ if (dr->secretlen > scr->secretlen)
	   { char *t;
	     t = malloc(dr->secretlen,M_DEVBUF,M_WAITOK);
	     if (t == 0) return(ENOBUFS);
	     if (scr->secret) free(scr->secret,M_DEVBUF);
	     scr->secret = t;
	   }
	  memcpy(scr->secret,&sbuf[0],dr->secretlen);
	  scr->secretlen = dr->secretlen;
	}
       scr->srcmatch = dr->srcmatch;
       scr->srcmask = dr->srcmask;
       scr->src = dr->src;
       scr->dst = dr->dst;
       scr->curdst = dr->dst; /* not dr->curdst */
       scr->id = dr->id;
       scr->ttl = dr->ttl ? : IPDEFTTL;
       return(0);
       break;
    case ENCAP_DELRT:
	{ unsigned int i;
	  if (! (flag & FWRITE)) return(EBADF);
	  i = *(unsigned int *)data;
	  if (i >= sc->nrt) return(EDOM);
	  scr = sc->rts[i];
	  if (scr->secret) free(scr->secret,M_DEVBUF);
	  free(scr,M_DEVBUF);
	  sc->nrt --;
	  if (i < sc->nrt) memcpy(sc->rts+i,sc->rts+i+1,(sc->nrt-i)*sizeof(*sc->rts));
	  if (sc->nrt == 0)
	   { free(sc->rts,M_DEVBUF);
	     sc->rts = 0;
	     sc->ifnet.if_flags &= ~IFF_UP;
	   }
	}
       return(0);
       break;
    case ENCAP_SENDTO:
	{ unsigned int i;
	  i = *(unsigned int *)data;
	  if (i >= sc->nrt) return(EDOM);
	  send_magic_packet(sc,sc->rts[i],0,0);
	}
       return(0);
       break;
    case ENCAP_REBOOT:
	{ unsigned int i;
	  i = *(unsigned int *)data;
	  if (i >= sc->nrt) return(EDOM);
	  send_magic_packet(sc,sc->rts[i],"r",1);
	}
       return(0);
       break;
    case ENCAP_SFLAGS:
	{ unsigned int f;
	  if (! (flag & FWRITE)) return(EBADF);
	  f = *(unsigned int *)data & ESF_UCHG;
	  flags = (flags & ~ESF_UCHG) | (f & ESF_GLOBAL);
	  sc->flags = (sc->flags & ~ESF_UCHG) | (f & ~ESF_GLOBAL);
	}
       return(0);
       break;
    case ENCAP_GFLAGS:
       if (! (flag & FREAD)) return(EBADF);
       *(unsigned int *)data = sc->flags | flags;
       return(0);
       break;
    case ENCAP_SGFLAGS:
	{ unsigned int o;
	  unsigned int n;
	  if ((flag & (FWRITE|FREAD)) != (FWRITE|FREAD)) return(EBADF);
	  o = sc->flags | flags;
	  n = *(unsigned int *)data & ESF_UCHG;
	  flags = (flags & ~ESF_UCHG) | (n & ESF_GLOBAL);
	  sc->flags = (sc->flags & ~ESF_UCHG) | (n & ~ESF_GLOBAL);
	  *(unsigned int *)data = o;
	}
       return(0);
       break;
    case ENCAP_BLOCK:
#ifdef STONE_ENCAP_HACKS
       return(sglist_ioctl(&sc->block,data));
#else
       return(EOPNOTSUPP); /* needs translation */
#endif
       break;
    case ENCAP_DEBUG:
       /* dump out:
		- encap0 if_snd size
		- ppp0 if_snd size
		- ppp0's tty's t_outq size
       */
	{ struct ifnet *i;
	  /* Find encap0 */
	  for (i=ifnet.tqh_first;i;i=i->if_list.tqe_next) if (!strcmp(i->if_xname,"encap0")) break;
	  /* Print encap0 goop */
	  if (i)
	   { printf("encap0 if_snd len=%d maxlen=%d drops=%d\n",i->if_snd.ifq_len,i->if_snd.ifq_maxlen,i->if_snd.ifq_drops);
	   }
	  else
	   { printf("no encap0 found\n");
	   }
	  /* Find ppp0 */
	  for (i=ifnet.tqh_first;i;i=i->if_list.tqe_next) if (!strcmp(i->if_xname,"ppp0")) break;
	  /* Print ppp0 goop */
	  if (i)
	   { struct ppp_softc *psc;
	     printf("ppp0 if_snd len=%d maxlen=%d drops=%d\n",i->if_snd.ifq_len,i->if_snd.ifq_maxlen,i->if_snd.ifq_drops);
	     psc = i->if_softc;
	     printf("ppp0 fastq len=%d maxlen=%d drops=%d\n",psc->sc_fastq.ifq_len,psc->sc_fastq.ifq_maxlen,psc->sc_fastq.ifq_drops);
	     if (psc->sc_devp)
	      { struct tty *tp;
		tp = psc->sc_devp;
		printf("ppp0 tty outq count %d\n",tp->t_outq.c_cc);
	      }
	     else
	      { printf("ppp0 has no device\n");
	      }
	   }
	  else
	   { printf("no ppp0 found\n");
	   }
	}
       return(0);
       break;
    case ENCAP_POISON:
#ifdef STONE_ENCAP_HACKS
       return(sglist_ioctl(&sc->poison,data));
#else
       return(EOPNOTSUPP); /* needs translation */
#endif
       break;
    case ENCAP_POISONED:
#ifdef STONE_ENCAP_HACKS
	{ struct encap_listio *arg;
	  LISTENTRY *oldlist;
	  int oldn;
	  int copyn;
	  int s;
	  int i;
	  arg = (void *) data;
	  if (arg->oldn < 1)
	   { s = splhigh();
	     arg->oldn = sc->poisoned.len;
	     splx(s);
	     return(0);
	   }
	  oldlist = malloc(arg->oldn*sizeof(LISTENTRY),M_DEVBUF,M_WAITOK);
	  s = splhigh();
	  oldn = sc->poisoned.len;
	  copyn = (arg->oldn < oldn) ? arg->oldn : oldn;
	  for (i=copyn-1;i>=0;i--)
	   { oldlist[i].net = sc->poisoned.list[i].net;
	     oldlist[i].mask = sc->poisoned.list[i].mask;
	   }
	  splx(s);
	  if (copyn >= 0)
	   { err = copyout(oldlist,arg->oldlist,copyn*sizeof(LISTENTRY));
	     if (err)
	      { free(oldlist,M_DEVBUF);
		return(err);
	      }
	     arg->oldn = oldn;
	   }
	  free(oldlist,M_DEVBUF);
	  return(0);
	}
#else
       return(EOPNOTSUPP); /* needs translation */
#endif
       break;
  }
 return(ENOTTY);
}

#endif /* NENCAP > 0 */

#if 0
/* Fragments relating to the Holo rewrites:
	rewrite holomuck-mcgill/23 to vger-mcgill/50100
	rewrite holomuck-openface/23 to vger-openface/50100
	rewrite vger-mcgill/50100 to holomuck-mcgill/23
	rewrite vger-openface/50100 to holomuck-openface/23
*/

#define VGER_M 0x84ce4e20
#define VGER_O 0xd82e0508
#define HOLOMUCK_M 0x84ce4e02
#define HOLOMUCK_O 0xd82e0502

incoming:
    case HOLOMUCK_M:
    case HOLOMUCK_O:
	  switch (dstaddr)
	   { case HOLOMUCK_M:
		switch (dstport)
		 { case 23:
		      th->th_dport = htons(50100);
		      ip->ip_dst.s_addr = htonl(VGER_M);
		      recksum = 1;
		      break;
		 }
		break;
	     case HOLOMUCK_O:
		switch (dstport)
		 { case 23:
		      th->th_dport = htons(50100);
		      ip->ip_dst.s_addr = htonl(VGER_O);
		      recksum = 1;
		      break;
		 }
		break;
	   }

outgoing:
    case VGER_M:
    case VGER_O:
	     case VGER_M:
		if (srcport == 50100)
		 { th->th_sport = htons(23);
		   ip->ip_src.s_addr = htonl(HOLOMUCK_M);
		   recksum = 1;
		 }
		break;
	     case VGER_O:
		if (srcport == 50100)
		 { th->th_sport = htons(23);
		   ip->ip_src.s_addr = htonl(HOLOMUCK_O);
		   recksum = 1;
		 }
		break;

#endif
