/*
 * Watcher notify messagse:
 *
 *	key	data and meaning
 *	z	none
 *		all blocking was cleared
 *	d	address
 *		address was deleted
 *	m	address
 *		memory shortage trying to list address
 *	a	address, int=N, N bytes of packet data
 *		address was listed; packet causing listing follows
 *		if listing was from ioctl, packet is zero-length
 *	f	address, int=N, N bytes of packet data
 *		just like a except address was already listed ("freshen")
 *
 * "address" means a 4-byte IP address in network byte order.
 */

#include "pfw.h"
#if NPFW > 0

#include <net/if.h>
#include <sys/mbuf.h>
#include <net/pfil.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/systm.h>
#include <netinet/ip.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/kthread.h>
#include <sys/protosw.h>
#include <sys/filedesc.h>
#include <sys/socketvar.h>
#include <machine/stdarg.h>

#include <dev/pseudo/pfw-kern.h>

typedef __typeof__(time.tv_sec) TVSEC;
typedef struct ftn FTN;
typedef struct watch WATCH;

struct watch {
  WATCH *link;
  struct file *f;
  int errored;
  } ;

/*
 * FTNs are kept simultaneously in two data structures: (a) a heap, in
 *  ftnv[] (with aftn the allocated size and nftn the live size), with
 *  the comparison criterion such that the top of the heap has minimum
 *  exp value, and (b) a balanced binary tree, with u, l, and r
 *  pointers, such that it's a binary search tree on the addr values,
 *  with bal representing the imbalance figure (+ve means the right
 *  subtree is deeper, -ve, the left).
 */
struct ftn {
  FTN *u;
  FTN *l;
  FTN *r;
  int bal;
  u_int32_t addr;
  TVSEC exp;
  TVSEC upd;
  int hx;
  } ;

static struct ifnet *fifp;
static int nftn;
static int aftn;
static FTN **ftnv;
static FTN *ftnr;
static int running;
static int attached;
static unsigned long long int serial;
static WATCH *watchers;
static struct proc *watchproc;

/*
 * Rebalance the binary tree after an insertion or deletion.  *up is
 *  the root pointer of the (sub)tree we want to maybe rebalance; uptr
 *  is the correct thing to put in the u field of an element replacing
 *  *up.  The tree must be self-consistent (all u, l, r pointers right,
 *  all bal values correct given the structure).
 *
 * Return value is 1 if the result is unbalanced (it will never be
 *  unbalanced by more than 1) or 0 if it is now balanced.
 */
static int rebalance(FTN **up, FTN *uptr)
{
 FTN *u;
 FTN *f;
 FTN *b;
 FTN *c;

 u = *up;
 if (uptr != u->u) printf("pfw rebalance: uptr wrong\n");
 switch (u->bal)
  { case 0:
       return(0);
       break;
    case -1: case 1:
       return(1);
       break;
    case -2:
       if (u->l->bal <= 0)
	{ /*
	   * Pivot:
	   *
	   *					u (-2)
	   *			A (-1)				B
	   *		C		f		       / \
	   *	       / \	       / \		      /   \
	   *	      /   \           /   \		     -------
	   *	     /     \	     -------
	   *	    ---------
	   * becomes
	   *				A (0)
	   *		C				u (0)
	   *	       / \			f		B
	   *	      /   \		       / \	       / \
	   *	     /     \		      /   \	      /   \
	   *	    ---------		     -------	     -------
	   *
	   * and
	   *
	   *					u (-2)
	   *			A (0)				B
	   *		C		f		       / \
	   *	       / \	       / \		      /   \
	   *	      /   \	      /   \		     -------
	   *	     /     \	     /     \
	   *	    ---------	    ---------
	   * becomes
	   *				A (1)
	   *		C				u (-1)
	   *	       / \			f		B
	   *	      /   \		       / \	       / \
	   *	     /     \		      /   \	      /   \
	   *	    ---------		     /     \	     -------
	   *				    ---------
	   */
	  u->bal = -1 - u->l->bal;
	  u->l->bal ++;
	  *up = u->l;
	  u->l->u = uptr;
	  f = u->l->r;
	  u->l->r = u;
	  u->u = u->l;
	  u->l = f;
	  if (f) f->u = u;
	  if (u->bal) return(1);
	}
       else if (u->l->bal > 0)
	{ /*
	   * Pivot:
	   *
	   *					u (-2)
	   *			A (1)				B
	   *		C		f (?)		       / \
	   *	       / \	   b	     c		      /   \
	   *	      /   \	  / \	    / \		     /     \
	   *	     /     \	 /   \	   /   \	    ---------
	   *	    ---------	----  \	  ----  \
	   *			    ----      ----
	   * becomes
	   *				    f (0)
	   *		    A (-1,0)			    u (0,1)
	   *	    C		    b		    c		    D
	   *	   / \		   / \		   / \		   / \
	   *	  /   \		  /   \		  /   \		  /   \
	   *	 /     \	 ----  \	 ----  \	 /     \
	   *    ---------	     ----	     ----	---------
	   */
	  f = u->l->r;
	  b = f->l;
	  c = f->r;
	  *up = f;
	  f->u = uptr;
	  f->l = u->l;
	  f->l->u = f;
	  f->r = u;
	  u->u = f;
	  f->l->r = b;
	  if (b) b->u = f->l;
	  u->l = c;
	  if (c) c->u = u;
	  f->l->bal = (f->bal > 0) ? -1 : 0;
	  f->r->bal = (f->bal < 0) ?  1 : 0;
	  f->bal = 0;
	}
       return(0);
       break;
    case 2:
       /* See case -2, above, for diagrams, but reverse left/right */
       if (u->r->bal >= 0)
	{ u->bal = 1 - u->r->bal;
	  u->r->bal --;
	  *up = u->r;
	  u->r->u = uptr;
	  f = u->r->l;
	  u->r->l = u;
	  u->u = u->r;
	  u->r = f;
	  if (f) f->u = u;
	  if (u->bal) return(1);
	}
       else if (u->r->bal < 0)
	{ f = u->r->l;
	  b = f->r;
	  c = f->l;
	  *up = f;
	  f->u = uptr;
	  f->r = u->r;
	  f->r->u = f;
	  f->l = u;
	  u->u = f;
	  f->r->l = b;
	  if (b) b->u = f->r;
	  u->r = c;
	  if (c) c->u = u;
	  f->r->bal = (f->bal < 0) ?  1 : 0;
	  f->l->bal = (f->bal > 0) ? -1 : 0;
	  f->bal = 0;
	}
       return(0);
       break;
  }
 panic("pfw: impossible balance");
}

/*
 * Insert f into the binary tree whose root pointer is *up.  uptr is
 *  the correct thing to put in the u field of an element replacing
 *  *up.
 *
 * Return value is:
 *
 *	INSERT_DROP
 *		The element is a duplicate and should be dropped; the
 *		binary tree has not been modified.
 *
 *	INSERT_SAME
 *		The element has been absorbed without changing the
 *		depth of the tree.
 *
 *	INSERT_DEEPEN
 *		The element has been inserted; the resulting tree is
 *		one level deeper than before.
 */
#define INSERT_DROP   1
#define INSERT_SAME   2
#define INSERT_DEEPEN 3
static int insert(FTN *f, FTN **up, FTN *uptr)
{
 FTN *u;

 u = *up;
 if (! u)
  { *up = f;
    f->u = uptr;
    return(INSERT_DEEPEN);
  }
 if (f->addr < u->addr)
  { switch (insert(f,&u->l,u))
     { default:
	  panic("impossible < sub-insert");
	  break;
       case INSERT_DROP:
	  return(INSERT_DROP);
	  break;
       case INSERT_SAME:
	  return(INSERT_SAME);
	  break;
       case INSERT_DEEPEN:
	  u->bal --;
	  return(rebalance(up,uptr)?INSERT_DEEPEN:INSERT_SAME);
	  break;
     }
  }
 else if (f->addr > u->addr)
  { switch (insert(f,&u->r,u))
     { default:
	  panic("impossible > sub-insert");
	  break;
       case INSERT_DROP:
	  return(INSERT_DROP);
	  break;
       case INSERT_SAME:
	  return(INSERT_SAME);
	  break;
       case INSERT_DEEPEN:
	  u->bal ++;
	  return(rebalance(up,uptr)?INSERT_DEEPEN:INSERT_SAME);
	  break;
     }
  }
 else
  { /* This can happen if, eg, the add device is used to
       add an address that's already present. */
    return(INSERT_DROP);
  }
}

/*
 * Search for the correct place and insert f, rebalancing as necessary.
 *  Return value is as for insert(), above.
 */
static int search_insert(FTN *f)
{
 f->l = 0;
 f->r = 0;
 f->bal = 0;
 return(insert(f,&ftnr,0));
}

/*
 * Delete f from the binary tree, rebalancing as necessary.  Blindly
 *  assumes that (a) f is already part of the tree and (b) the tree is
 *  self-consistent.
 */
static void search_delete(FTN *f)
{
 FTN *u;
 FTN *l;
 FTN *r;
 FTN **up;
 int dr;
 FTN *s;

 u = f->u;
 l = f->l;
 r = f->r;
 up = u ? (u->l == f) ? &u->l : &u->r : &ftnr;
 dr = u ? (u->l == f) ? 1 : -1 : 0;
 if (! f->r)
  { if (! f->l)
     { *up = 0;
     }
    else
     { f->l->u = u;
       *up = f->l;
     }
  }
 else if (! f->l)
  { f->r->u = u;
    *up = f->r;
  }
 else if (! f->r->l)
  { f->r->l = f->l;
    f->l->u = f->r;
    f->r->u = u;
    *up = f->r;
    u = f->r;
    u->bal = f->bal;
    dr = -1;
  }
 else
  { s = f->r;
    while (s->l) s = s->l;
    s->u->l = s->r;
    if (s->r) s->r->u = s->u;
    s->l = f->l;
    f->l->u = s;
    s->r = f->r;
    f->r->u = s;
    s->bal = f->bal;
    f = s->u;
    s->u = u;
    *up = s;
    u = f;
    dr = 1;
  }
 if (u)
  { u->bal += dr;
    while <"delrebal"> (1)
     { switch (u->bal)
	{ case 0:
	     if (u->u)
	      { u->u->bal += (u == u->u->l) ? 1 : -1;
		u = u->u;
		continue;
	      }
	     break <"delrebal">;
	     break;
	  case -1:
	  case 1:
	     break <"delrebal">;
	     break;
	  case -2:
	  case 2:
	      { FTN *v;
		v = u->u;
		if (v)
		 { int ob;
		   ob = v->bal;
		   v->bal += (u == v->l) ? 1 : -1;
		   if (rebalance((u==v->l)?&v->l:&v->r,v))
		    { v->bal = ob;
		      break <"delrebal">;
		    }
		   u = v;
		   continue;
		 }
		rebalance(&ftnr,0);
		break <"delrebal">;
	      }
	     break;
	  default:
	     panic("pfw: impossible delete rebalance");
	     break;
	}
     }
  }
}

/*
 * Look up the FTN for an address; return nil if not found.
 */
static FTN *search_find(u_int32_t a)
{
 FTN *f;

 f = ftnr;
 while (1)
  { if (! f) return(0);
    if (f->addr == a) return(f);
    f = (a < f->addr) ? f->l : f->r;
  }
}

static void addr_to_buf(u_int32_t addr, unsigned char *buf)
{
 buf[0] = (addr >> 24) & 0xff;
 buf[1] = (addr >> 16) & 0xff;
 buf[2] = (addr >>  8) & 0xff;
 buf[3] = (addr      ) & 0xff;
}

static __inline__ u_int32_t nw__cvt_uint32t(u_int32_t v) { return(v); }
static __inline__ struct mbuf *nw__cvt_mbuf_p(struct mbuf *v) { return(v); }
static __inline__ int nw__cvt_int(int v) { return(v); }

static void notify_watchers(char flgc, ...)
#define NW__END  1
#define NW_END NW__END
#define NW__ADDR 2
#define NW_ADDR(a) NW__ADDR, nw__cvt_uint32t((a))
#define NW__MBUF 3
#define NW_MBUF(m) NW__MBUF, nw__cvt_mbuf_p((m))
#define NW__INT  4
#define NW_INT(i) NW__INT, nw__cvt_int((i))
{
 WATCH **wp;
 WATCH *w;
 struct socket *so;
 struct mbuf *m0;

 wp = &watchers;
 while <"nextwatcher"> ((w = *wp))
  { do <"closethis">
     { so = w->f->f_data;
       if ((so->so_state & (SS_ISCONNECTED|SS_CANTSENDMORE)) != SS_ISCONNECTED) break <"closethis">;
       do <"keepthis">
	{ if (so->so_snd.sb_mbcnt > 64)
	   { if (w->errored) break <"keepthis">;
	     MGETHDR(m0,M_NOWAIT,MT_DATA);
	     if (m0 == 0) break <"keepthis">;
	     m0->m_pkthdr.len = 1;
	     mtod(m0,char *)[0] = 'e';
	   }
	  else
	   { va_list ap;
	     int key;
	     struct mbuf *m;
	     struct mbuf **tail;
	     MGETHDR(m0,M_DONTWAIT,MT_DATA);
	     if (m0 == 0) break <"keepthis">;
	     m0->m_len = 1;
	     m0->m_pkthdr.len = 1;
	     *mtod(m0,unsigned char *) = flgc;
	     tail = &m0->m_next;
	     va_start(ap,flgc);
	     while <"argloop"> (1)
	      { key = va_arg(ap,int);
		switch (key)
		 { case NW__END:
		      *tail = 0;
		      break <"argloop">;
		   case NW__ADDR:
		       { u_int32_t a;
			 a = va_arg(ap,u_int32_t);
			 MGET(m,M_DONTWAIT,MT_DATA);
			 m->m_len = 4;
			 addr_to_buf(a,mtod(m,unsigned char *));
			 *tail = m;
			 tail = &m->m_next;
		       }
		      break;
#if 0
		   case NW__BLK:
		       { int len;
			 const void *data;
			 len = va_arg(ap,int);
			 data = va_arg(ap,const void *);
			 if (len > MCLBYTES) panic("notify_watchers: too long");
			 MGET(m,M_DONTWAIT,MT_DATA);
			 if (len > MHLEN)
			  { MCLGET(m,M_DONTWAIT);
			    if (! (m->m_flags & M_EXT))
			     { m_freem(m);
			       m_freem(m0);
			       break <"keepthis">;
			     }
			  }
			 m->m_len = len;
			 bcopy(data,mtod(m,void *),len);
			 m0->m_pkthdr.len += len;
			 *tail = m;
			 tail = &m->m_next;
		       }
		      break;
#endif
		   case NW__MBUF:
		      m = va_arg(ap,struct mbuf *);
		      if (m)
		       { m = m_dup(m,0,M_COPYALL,M_DONTWAIT);
			 if (m == 0)
			  { m_freem(m0);
			    break <"keepthis">;
			  }
			 *tail = m;
			 while (m->m_next)
			  { m0->m_pkthdr.len += m->m_len;
			    m = m->m_next;
			  }
			 m0->m_pkthdr.len += m->m_len;
			 tail = &m->m_next;
		       }
		      break;
		   case NW__INT:
		       { int v;
			 v = va_arg(ap,int);
			 MGET(m,M_DONTWAIT,MT_DATA);
			 if (m == 0)
			  { m_freem(m0);
			    break <"keepthis">;
			  }
			 m->m_len = sizeof(int);
			 *mtod(m,int *) = v;
			 *tail = m;
			 tail = &m->m_next;
			 m0->m_pkthdr.len += sizeof(int);
		       }
		      break;
		   default:
		      panic("notify-watchers: key %d",key);
		      break;
		 }
	      }
	     va_end(ap);
	     w->errored = 0;
	   }
	  m0->m_pkthdr.rcvif = 0;
	  if ((*so->so_proto->pr_usrreq)(so,PRU_SEND,m0,0,0,watchproc)) break <"closethis">;
	} while (0);
       wp = &w->link;
       continue <"nextwatcher">;
     } while (0);
    FILE_USE(w->f);
    soshutdown(so,SHUT_RDWR);
    closef(w->f,0);
    *wp = w->link;
    free(w,M_DEVBUF);
    continue <"nextwatcher">;
  }
}

static void ftn_clear(void)
{
 int s;
 int n;
 FTN **v;

 s = splnet();
 n = nftn;
 v = ftnv;
 nftn = 0;
 aftn = 0;
 ftnv = 0;
 ftnr = 0;
 notify_watchers('z',NW_END);
 splx(s);
 for (n--;n>=0;n--) free(v[n],M_DEVBUF);
 if (v) free(v,M_DEVBUF);
}

static void heap_down(FTN *f)
{
 int x;
 int l;
 int r;
 int s;

 x = f->hx;
 while (1)
  { l = x + x + 1;
    r = l + 1;
    if ((l < nftn) && (ftnv[l]->exp < f->exp))
     { if ((r < nftn) && (ftnv[r]->exp < f->exp))
	{ s = (ftnv[l]->exp < ftnv[r]->exp) ? l : r;
	}
       else
	{ s = l;
	}
     }
    else
     { if ((r < nftn) && (ftnv[r]->exp < f->exp))
	{ s = r;
	}
       else
	{ break;
	}
     }
    ftnv[s]->hx = x;
    ftnv[x] = ftnv[s];
    x = s;
  }
 ftnv[x] = f;
 f->hx = x;
}

static int verify_walk(FTN **pp, FTN *f, FTN *u)
{
 int ld;
 int rd;

 if (f == 0) return(0);
 if ((f->hx < 0) || (f->hx >= nftn)) panic("pfw verify_walk: hx %d",f->hx);
 if (f != ftnv[f->hx]) panic("pfw verify_walk: %p != ftnv[%d]=%p",(void *)f,f->hx,(void *)ftnv[f->hx]);
 if (f->u != u) panic("pfw verify_walk: %p->u=%p != %p",(void *)f,(void *)f->u,(void *)u);
 ld = verify_walk(pp,f->l,f);
 if (*pp && ((*pp)->addr >= f->addr)) panic("pfw verify_walk: %p->addr (%08lx) >= %p->addr (%08lx)",(void *)*pp,(unsigned long int)(*pp)->addr,(void *)f,(unsigned long int)f->addr);
 *pp = f;
 rd = verify_walk(pp,f->r,f);
 if ( ((ld == rd+1) && (f->bal == -1)) ||
      ((ld == rd) && (f->bal == 0)) ||
      ((ld == rd-1) && (f->bal == 1)) ) return(((ld>rd)?ld:rd)+1);
 panic("pfw verify_walk: %p ld=%d rd=%d bal=%d",(void *)f,ld,rd,f->bal);
}

static void verify(void)
{
 int s;
 int i;
 FTN *f;
 FTN *p;

 s = splnet();
 for (i=0;i<nftn;i++)
  { f = ftnv[i];
    if (! f) panic("pfw verify: !ftnv[%d<%d]",i,nftn);
    if (f->hx != i) panic("pfw verify: ftnv[%d] hx %d",i,f->hx);
    if (i && (f->exp < ftnv[(f->hx-1)>>1]->exp))
     { panic("pfw verify: ftnv[%d]->exp (%lu) < ftnv[%d]->exp (%lu)",
		f->hx,(unsigned long int)f->exp,
		(f->hx-1)>>1,(unsigned long int)ftnv[(f->hx-1)>>1]->exp);
     }
  }
 p = 0;
 verify_walk(&p,ftnr,0);
 splx(s);
}

static void ftn_freshen(FTN *f)
{
 f->exp = mono_time.tv_sec + 86400;
 f->upd = mono_time.tv_sec + 3600;
 heap_down(f);
 serial ++;
 verify();
}

static void del_ftn(FTN *f)
{
 int x;

 search_delete(f);
 serial ++;
 x = f->hx;
 free(f,M_DEVBUF);
 nftn --;
 if ((nftn > 0) && (x < nftn))
  { f = ftnv[nftn];
    f->hx = x;
    heap_down(f);
  }
 verify();
}

static void ticker(void *arg __attribute__((__unused__)))
{
 int s;
 FTN *f;

 s = splnet();
 while (nftn > 0)
  { f = ftnv[0];
    if (f->exp > mono_time.tv_sec) break;
    notify_watchers('d',NW_ADDR(f->addr),NW_END);
    del_ftn(f);
  }
 if (nftn > 0) timeout(ticker,0,hz); else running = 0;
 splx(s);
}

static int total_mbuf_len(struct mbuf *m)
{
 int len;

 for (len=0;m;m=m->m_next) len += m->m_len;
 return(len);
}

static void add_block(u_int32_t addr, struct mbuf *pkt)
{
 FTN *f;

 f = malloc(sizeof(FTN),M_DEVBUF,M_NOWAIT);
 if (f == 0)
  { notify_watchers('m',NW_ADDR(addr),NW_END);
    return;
  }
 if (nftn >= aftn)
  { FTN **nv;
    int a;
    a = aftn + 64;
    nv = malloc(a*sizeof(FTN *),M_DEVBUF,M_NOWAIT);
    if (nv == 0)
     { free(f,M_DEVBUF);
       notify_watchers('m',NW_ADDR(addr),NW_END);
       return;
     }
    bcopy(ftnv,nv,nftn*sizeof(FTN *));
    if (ftnv) free(ftnv,M_DEVBUF);
    ftnv = nv;
    aftn = a;
  }
 f->addr = addr;
 if (search_insert(f) == INSERT_DROP)
  { free(f,M_DEVBUF);
    f = search_find(addr);
    if (! f) panic("can't find duplicate");
    ftn_freshen(f);
    notify_watchers('f',NW_ADDR(addr),NW_INT(total_mbuf_len(pkt)),NW_MBUF(pkt),NW_END);
    return;
  }
 /* we know the new FTN belongs at the bottom of the heap */
 f->hx = nftn++;
 ftn_freshen(f); /* also stores into ftnv[], and calls verify() */
 notify_watchers('a',NW_ADDR(f->addr),NW_INT(total_mbuf_len(pkt)),NW_MBUF(pkt),NW_END);
 if (! running)
  { timeout(ticker,0,hz);
    running = 1;
  }
}

static void del_block(u_int32_t addr)
{
 FTN *f;

 f = search_find(addr);
 if (f == 0) return;
 notify_watchers('d',NW_ADDR(f->addr),NW_END);
 del_ftn(f);
}

static int pfw_hook(void *data, int hlen __attribute__((__unused__)), struct ifnet *ifp, int dir __attribute__((__unused__)), struct mbuf **m)
{
 struct ip *ip;
 struct tcphdr *th;
 struct udphdr *uh;
 FTN *ftn;
 int s;

 s = splnet();
 if (ifp != fifp)
  { splx(s);
    return(0);
  }
 ip = data;
 ftn = search_find(ntohl(ip->ip_src.s_addr));
 if (ftn)
  { if (mono_time.tv_sec > ftn->upd)
     { ftn_freshen(ftn);
       notify_watchers('f',NW_ADDR(ftn->addr),NW_INT(total_mbuf_len(*m)),NW_MBUF(*m),NW_END);
     }
    m_freem(*m);
    splx(s);
    return(1);
  }
 if (ip->ip_v == 4)
  { if (ip->ip_off & IP_OFFMASK)
     { splx(s);
       return(0);
     }
    switch (ip->ip_p)
     { case IPPROTO_TCP:
	  *m = m_pullup(*m,hlen+sizeof(struct tcphdr));
	  if (! *m)
	   { splx(s);
	     return(1);
	   }
	  th = (struct tcphdr *) (mtod(*m,char *) + hlen);
	  switch (ntohs(th->th_dport))
	   { case 137:
	     case 138:
	     case 139:
		add_block(ntohl(ip->ip_src.s_addr),*m);
		m_freem(*m);
		splx(s);
		return(1);
		break;
	   }
	  break;
       case IPPROTO_UDP:
	  *m = m_pullup(*m,hlen+sizeof(struct udphdr));
	  if (! *m)
	   { splx(s);
	     return(1);
	   }
	  uh = (struct udphdr *) (mtod(*m,char *) + hlen);
	  switch (ntohs(uh->uh_dport))
	   { case 137:
	     case 138:
	     case 139:
		add_block(ntohl(ip->ip_src.s_addr),*m);
		m_freem(*m);
		splx(s);
		return(1);
		break;
	   }
	  break;
     }
  }
 switch (ntohl(ip->ip_dst.s_addr))
  { case 0xd82e0500:
    case 0xd82e050f:
       add_block(ntohl(ip->ip_src.s_addr),*m);
       m_freem(*m);
       splx(s);
       return(1);
       break;
    case 0xd82e0509:
       if ((ip->ip_v == 4) && (ip->ip_p == IPPROTO_TCP))
	{ *m = m_pullup(*m,hlen+sizeof(struct tcphdr));
	  if (! *m)
	   { splx(s);
	     return(1);
	   }
	  th = (struct tcphdr *) (mtod(*m,char *) + hlen);
	  if (ntohs(th->th_dport) == 25)
	   { add_block(ntohl(ip->ip_src.s_addr),*m);
	     m_freem(*m);
	     splx(s);
	     return(1);
	   }
	}
       break;
  }
 splx(s);
 return(0);
}

void pfwattach(int arg __attribute__((__unused__)))
{
 /* delay until first open so we know inetsw and ip_protox are set up */
 attached = 0;
}

int pfwopen(dev_t dev, int flag, int mode, struct proc *p)
{
 if (! attached)
  { nftn = 0;
    aftn = 0;
    ftnv = 0;
    ftnr = 0;
    running = 0;
    serial = 0;
    watchers = 0;
    watchproc = 0;
    pfil_add_hook(pfw_hook,PFIL_IN,&inetsw[ip_protox[IPPROTO_IP]]);
    attached = 1;
  }
 return(0);
}

int pfwclose(dev_t dev, int flag, int mode, struct proc *p)
{
 return(0);
}

int pfwread(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 off_t o;
 int owi;
 int e;
 int s;
 char *d;
 char t;
 int l;
 unsigned int val_ui;
 u_int32_t val_32;
 unsigned char buf_32[4];
 unsigned long long int val_ulli;

 unit = minor(dev);
 if (uio->uio_offset < 0) return(EINVAL);
 switch (unit)
  { case 0:
       if (fifp)
	{ d = &fifp->if_xname[0];
	  l = strlen(d);
	}
       else
	{ t = '-';
	  d = &t;
	  l = 1;
	}
       break;
    case 1:
       return(0);
       break;
    case 2:
       s = splnet();
       val_ulli = serial;
       splx(s);
       d = (void *) &val_ulli;
       l = sizeof(val_ulli);
       break;
    case 3:
       s = splnet();
       val_ui = nftn;
       splx(s);
       d = (void *) &val_ui;
       l = sizeof(val_ui);
       break;
    case 4:
       while (uio->uio_resid)
	{ o = uio->uio_offset / 4;
	  owi = uio->uio_offset % 4;
	  if (o < 0) return(EINVAL);
	  s = splnet();
	  if (o >= nftn)
	   { splx(s);
	     break;
	   }
	  val_32 = htonl(ftnv[o]->addr);
	  buf_32[0] = (val_32 >> 24) & 0xff;
	  buf_32[1] = (val_32 >> 16) & 0xff;
	  buf_32[2] = (val_32 >>  8) & 0xff;
	  buf_32[3] = (val_32      ) & 0xff;
	  splx(s);
	  e = uiomove(owi+(char *)&buf_32[0],4-owi,uio);
	  if (e) return(e);
	}
       return(0);
       break;
    case 5:
    case 6:
    case 7:
       return(0);
       break;
  }
 if (uio->uio_offset >= l) return(0);
 return(uiomove(d+uio->uio_offset,l-uio->uio_offset,uio));
 return(0);
}

static void watchproc_main(void *arg __attribute__((__unused__)))
{
 while (1) tsleep(&watchers,PUSER,"forever",0);
 /*kthread_exit(0);*/
}

/* curproc is an implicit additional argument to addwatch() */
static int addwatch(int fd)
{
 struct file *fp;
 int e;
 struct socket *so;
 int s;
 WATCH *w;

 s = splnet();
 if (watchproc == 0)
  { e = kthread_create1(watchproc_main,0,&watchproc,"pfw-watcher");
    if (e)
     { splx(s);
       return(e);
     }
  }
 splx(s);
 e = getsock(curproc->p_fd,fd,&fp);
 if (e) return(e);
 if (fp->f_type != DTYPE_SOCKET)
  { FILE_UNUSE(fp,0);
    return(ENOTSOCK);
  }
 so = fp->f_data;
 FILE_UNUSE(fp,0);
 if (so->so_proto->pr_type != SOCK_STREAM) return(ESOCKTNOSUPPORT);
 if (! (so->so_state & SS_ISCONNECTED)) return(ENOTCONN);
 fp->f_count ++;
 fdrelease(curproc,fd);
 w = malloc(sizeof(WATCH),M_DEVBUF,M_WAITOK);
 w->f = fp;
 w->errored = 0;
 s = splnet();
 w->link = watchers;
 watchers = w;
 splx(s);
 return(0);
}

int pfwwrite(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 int e;
 int s;

 unit = minor(dev);
 switch (unit)
  { case 0:
	{ char xnbuf[64];
	  int l;
	  struct ifnet *i;
	  if (uio->uio_resid < 0) return(EINVAL);
	  if (uio->uio_resid > sizeof(xnbuf)-1) return(EMSGSIZE);
	  l = uio->uio_resid;
	  e = uiomove(&xnbuf[0],l,uio);
	  if (e) return(e);
	  xnbuf[l] = '\0';
	  if ((l == 1) && (xnbuf[0] == '-'))
	   { fifp = 0;
	   }
	  else
	   { i = ifunit(&xnbuf[0]);
	     if (i == 0) return(ENXIO);
	     fifp = i;
	   }
	}
       break;
    case 1:
       uio->uio_resid = 0;
       ftn_clear();
       break;
    case 2:
    case 3:
    case 4:
       return(EPERM);
       break;
    case 5:
	{ int fd;
	  switch (uio->uio_resid)
	   { case 1:
		 { unsigned char c;
		   e = uiomove(&c,1,uio);
		   fd = c;
		 }
		break;
	     case sizeof(int):
		e = uiomove(&fd,sizeof(int),uio);
		break;
	     default:
		return(EINVAL);
		break;
	   }
	  if (e) return(e);
	  return(addwatch(fd));
	}
       break;
    case 6:
	{ unsigned char a[4];
	  u_int32_t v;
	  FTN *f;
	  if (uio->uio_resid != 4) return(EINVAL);
	  e = uiomove(&a[0],4,uio);
	  if (e) return(e);
	  s = splnet();
	  v = (a[0]*0x1000000) + (a[1]*0x10000) + (a[2]*0x100) + a[3];
	  f = search_find(v);
	  if (f)
	   { ftn_freshen(f);
	     notify_watchers('f',NW_ADDR(v),NW_INT(0),NW_END);
	   }
	  else
	   { add_block(v,0);
	   }
	  splx(s);
	}
       break;
    case 7:
	{ unsigned char a[4];
	  if (uio->uio_resid != 4) return(EINVAL);
	  e = uiomove(&a[0],4,uio);
	  if (e) return(e);
	  s = splnet();
	  del_block((a[0]*0x1000000)+(a[1]*0x10000)+(a[2]*0x100)+a[3]);
	  splx(s);
	}
       break;
  }
 return(0);
}

int pfwioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 return(ENOTTY);
}

int pfwpoll(dev_t dev, int events, struct proc *p)
{
 unsigned int unit;

 unit = minor(dev);
 return(events&(POLLIN|POLLRDNORM));
}

#endif
