#include "srt.h"

#if NSRT > 0

#include "opt_inet.h"
#include "opt_pfil_hooks.h"
#if !defined(INET) && !defined(INET6)
#error SRT with neither INET nor INET6?
#endif

#include <net/if.h>
#include <sys/mbuf.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if_types.h>
#include <dev/pseudo/if_srt.h>

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <sys/time.h>
#endif

#ifdef PFIL_HOOKS
#include <net/pfil.h>
#include <sys/protosw.h>
#endif

typedef struct srt_softc SOFTC;
typedef struct srt_rt RT;

static SOFTC *softc;
static int nsrt;
static unsigned int flags;

static RT *find_rt(SOFTC *sc, struct in_addr src)
{
 int i;
 RT *r;

 for (i=0;i<sc->nrt;i++)
  { r = sc->rts[i];
    if ((src.s_addr & r->srcmask.s_addr) == r->srcmatch.s_addr) return(r);
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
static void srt_tap(caddr_t handle, struct mbuf *m0)
{
 u_int af;
 struct mbuf m;

 af = AF_INET;
 m.m_next = m0;
 m.m_len = 4;
 m.m_data = (void *)&af;
 bpf_mtap(handle,&m);
}

static int srt_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst, struct rtentry *rt)
{
 SOFTC *sc;
 RT *r;
 struct ip *ip;

 if (dst->sa_family != AF_INET)
  { IF_DROP(&ifp->if_snd);
    m_freem(m0);
    return(EAFNOSUPPORT);
  }
 if (! (ifp->if_flags & IFF_UP))
  { m_freem(m0);
    return(ENETDOWN);
  }
#if NBPFILTER > 0
 if (ifp->if_bpf) srt_tap(ifp->if_bpf,m0);
#endif
 sc = ifp->if_softc;
 ip = mtod(m0,struct ip *);
 r = find_rt(sc,ip->ip_src);
 if (r == 0)
  { ifp->if_opackets ++;
    ifp->if_oerrors ++;
    m_freem(m0);
    return(0);
  }
 if (! (m0->m_flags & M_PKTHDR)) panic("srt_output no HDR");
 ifp->if_opackets ++;
 ifp->if_obytes += m0->m_pkthdr.len;
 if (! (r->u.dstifp->if_flags & IFF_UP))
  { m_freem(m0);
    return(0); /* XXX ENETDOWN? */
  }
#ifdef PFIL_HOOKS
  { struct packet_filter_hook *pfh;
    int rv;
    for ( pfh = pfil_hook_get(PFIL_OUT,&inetsw[ip_protox[IPPROTO_IP]]);
	  pfh;
	  pfh = pfh->pfil_link.tqe_next )
     { if (pfh->pfil_func)
	{ rv = (*pfh->pfil_func)(ip,ip->ip_hl<<2,r->u.dstifp,1,&m0);
	  if (rv) return(EHOSTUNREACH);
	  ip = mtod(m0,struct ip *);
	  ip->ip_sum = 0;
	  ip->ip_sum = in_cksum(m0,ip->ip_hl<<2);
	}
     }
  }
#endif
 /* XXX is 0 ok as the last arg here? */
 return((*r->u.dstifp->if_output)(r->u.dstifp,m0,&r->dst.sa,0));
}

static int srt_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
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

static void update_mtu(SOFTC *sc)
{
 int mtu;
 int i;
 RT *r;

 if (sc->flags & SSF_MTULOCK) return;
 mtu = 65535;
 for (i=0;i<sc->nrt;i++)
  { r = sc->rts[i];
    if (r->u.dstifp->if_mtu < mtu) mtu = r->u.dstifp->if_mtu;
  }
 sc->ifnet.if_mtu = mtu;
}

void srtattach(int count)
{
 SOFTC *sc;
 int i;

 nsrt = count;
 softc = malloc(count*sizeof(SOFTC),M_DEVBUF,M_WAITOK);
 memset(softc,0,count*sizeof(SOFTC)); /* XXX */
 for (i=0;i<count;i++)
  { sc = softc + i;
    sprintf(sc->ifnet.if_xname,"srt%d",i);
    sc->ifnet.if_softc = sc;
    sc->ifnet.if_type = IFT_OTHER;
    sc->ifnet.if_addrlen = sizeof(struct in_addr);
    sc->ifnet.if_hdrlen = 0;
    sc->ifnet.if_mtu = 65536;
    sc->ifnet.if_flags = IFF_POINTOPOINT;
    sc->ifnet.if_output = srt_output;
    sc->ifnet.if_ioctl = srt_ioctl;
    sc->nrt = 0;
    sc->rts = 0;
    if_attach(&sc->ifnet);
#if NBPFILTER > 0
    bpfattach(&sc->ifnet.if_bpf,&sc->ifnet,DLT_NULL,4);
#endif
  }
}

int srtopen(dev_t dev, int flag, int mode, struct proc *p)
{
 if (minor(dev) >= nsrt) return(ENXIO);
 return(0);
}

int srtclose(dev_t dev, int flag, int mode, struct proc *p)
{
 return(0);
}

int srtioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 RT *dr;
 RT *scr;
 struct ifnet *ifp;
 char nbuf[IFNAMSIZ];

 sc = softc + minor(dev);
 switch (cmd)
  { case SRT_GETNRT:
       if (! (flag & FREAD)) return(EBADF);
       *(unsigned int *)data = sc->nrt;
       return(0);
       break;
    case SRT_GETRT:
       if (! (flag & FREAD)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx >= sc->nrt) return(EDOM);
       scr = sc->rts[dr->inx];
       dr->srcmatch = scr->srcmatch;
       dr->srcmask = scr->srcmask;
       strncpy(&dr->u.dstifn[0],&scr->u.dstifp->if_xname[0],IFNAMSIZ);
       dr->dst = scr->dst;
       return(0);
       break;
    case SRT_SETRT:
       if (! (flag & FWRITE)) return(EBADF);
       dr = (RT *) data;
       if (dr->inx > sc->nrt) return(EDOM);
       strncpy(&nbuf[0],&dr->u.dstifn[0],IFNAMSIZ);
       nbuf[IFNAMSIZ-1] = '\0';
       ifp = ifunit(&nbuf[0]);
       if (ifp == 0) return(ENXIO); /* needs translation */
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
	  scr->u.dstifp = 0;
	  bzero(&scr->dst,sizeof(scr->dst));
	  scr->dst.sin.sin_len = sizeof(scr->dst);
	  scr->dst.sin.sin_family = AF_INET;
	  sc->rts[dr->inx] = scr;
	}
       scr->srcmatch = dr->srcmatch;
       scr->srcmask = dr->srcmask;
       scr->u.dstifp = ifp;
       scr->dst.sin.sin_addr = dr->dst.sin.sin_addr;
       update_mtu(sc);
       return(0);
       break;
    case SRT_DELRT:
	{ unsigned int i;
	  if (! (flag & FWRITE)) return(EBADF);
	  i = *(unsigned int *)data;
	  if (i >= sc->nrt) return(EDOM);
	  scr = sc->rts[i];
	  free(scr,M_DEVBUF);
	  sc->nrt --;
	  if (i < sc->nrt) memcpy(sc->rts+i,sc->rts+i+1,(sc->nrt-i)*sizeof(*sc->rts));
	  if (sc->nrt == 0)
	   { free(sc->rts,M_DEVBUF);
	     sc->rts = 0;
	     sc->ifnet.if_flags &= ~IFF_UP;
	   }
	}
       update_mtu(sc);
       return(0);
       break;
    case SRT_SFLAGS:
	{ unsigned int f;
	  if (! (flag & FWRITE)) return(EBADF);
	  f = *(unsigned int *)data & SSF_UCHG;
	  flags = (flags & ~SSF_UCHG) | (f & SSF_GLOBAL);
	  sc->flags = (sc->flags & ~SSF_UCHG) | (f & ~SSF_GLOBAL);
	}
       return(0);
       break;
    case SRT_GFLAGS:
       if (! (flag & FREAD)) return(EBADF);
       *(unsigned int *)data = sc->flags | flags;
       return(0);
       break;
    case SRT_SGFLAGS:
	{ unsigned int o;
	  unsigned int n;
	  if ((flag & (FWRITE|FREAD)) != (FWRITE|FREAD)) return(EBADF);
	  o = sc->flags | flags;
	  n = *(unsigned int *)data & SSF_UCHG;
	  flags = (flags & ~SSF_UCHG) | (n & SSF_GLOBAL);
	  sc->flags = (sc->flags & ~SSF_UCHG) | (n & ~SSF_GLOBAL);
	  *(unsigned int *)data = o;
	}
       return(0);
       break;
    case SRT_DEBUG:
       return(0);
       break;
  }
 return(ENOTTY);
}

#endif /* NSRT > 0 */
