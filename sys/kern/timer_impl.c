#include <sys/mbuf.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/timersock.h>
#include <sys/socketvar.h>

typedef struct timerpcb PCB;

struct timerpcb {
  struct socket *socket;
  struct itimerval itv;
  int flags;
#define TPF_RUNNING  0x00000001
#define TPF_NOWRITES 0x00000002
  int hx;
  int pending;
  struct mbuf *emerg;
  } ;

static volatile int ticking = 0;
static volatile int nrunning = 0;
static volatile int heapa = 0;
static PCB ** volatile heap = 0;

volatile int timerdebug = 0;

/* Why they put the comparison out at the end I'll never figure out. */
#define tvcmp(a,c,b) timercmp(a,b,c)

static char dbgbuf[65536];
static int dbgptr;

static void dbgout_char(char c)
{
 if (dbgptr < sizeof(dbgbuf)) dbgbuf[dbgptr++] = c;
}

static void dbgout_str(const char *s)
{
 for (;*s;s++) dbgout_char(*s);
}

static void dbgout_udec(unsigned long int v, int min)
{
 if ((v == 0) && (min < 1)) return;
 dbgout_udec(v/10,min-1);
 dbgout_char('0'+(v%10));
}

static void dbgout_dec(long int v, int min)
{
 if (v < 0)
  { dbgout_char('-');
    dbgout_udec(-v,min);
  }
 else
  { dbgout_udec(v,min);
  }
}

static void dbgout_hex(unsigned long int v, int min)
{
 if ((v == 0) && (min < 1)) return;
 dbgout_hex(v/16,min-1);
 dbgout_char("0123456789abcdef"[v%16]);
}

static void dump_info_to_buf(int ival, int uval)
{
 int i;
 PCB *p;

 dbgptr = 0;
 dbgout_str("i = ");
 dbgout_dec(ival,1);
 dbgout_str(", u = ");
 dbgout_dec(uval,1);
 dbgout_str("\nticking = ");
 dbgout_dec(ticking,1);
 dbgout_str("\nnrunning = ");
 dbgout_dec(nrunning,1);
 dbgout_str("\nheapa = ");
 dbgout_dec(heapa,1);
 dbgout_str("\n");
 for (i=0;i<nrunning;i++)
  { dbgout_str("[");
    dbgout_dec(i,1);
    dbgout_str("] ");
    p = heap[i];
    dbgout_hex((unsigned long int)p,8);
    dbgout_str(" ");
    dbgout_dec(p->itv.it_value.tv_sec,1);
    dbgout_str(".");
    dbgout_dec(p->itv.it_value.tv_usec,6);
    dbgout_str(" ");
    dbgout_dec(p->itv.it_interval.tv_sec,1);
    dbgout_str(".");
    dbgout_dec(p->itv.it_interval.tv_usec,6);
    if (p->flags == 0)
     { dbgout_str("-");
     }
    else
     { if (p->flags & TPF_RUNNING) dbgout_str("R");
       if (p->flags & TPF_NOWRITES) dbgout_str("N");
       if (p->flags & ~(TPF_RUNNING|TPF_NOWRITES)) dbgout_hex(p->flags&~(TPF_RUNNING|TPF_NOWRITES),1);
     }
    dbgout_str(" hx=");
    dbgout_dec(p->hx,1);
    dbgout_str(" pend=");
    dbgout_dec(p->pending,1);
    dbgout_str("\n");
  }
}

#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
static void save_buf_to_file(void)
{
 struct nameidata nd;
 int e;
 struct vnode *vp;
 struct uio uio;
 struct iovec iov;
 struct vattr vattr;

 NDINIT(&nd,LOOKUP,NOFOLLOW,UIO_SYSSPACE,"/heap.debug",initproc);
 e = vn_open(&nd,FREAD|FWRITE,0600);
 if (e)
  { printf("can't save buf: vn_open error %d\n",e);
    return;
  }
 vp = nd.ni_vp;
 iov.iov_base = &dbgbuf[0];
 iov.iov_len = dbgptr;
 uio.uio_iov = &iov;
 uio.uio_offset = 0;
 uio.uio_segflg = UIO_SYSSPACE;
 uio.uio_rw = UIO_WRITE;
 uio.uio_resid = dbgptr;
 uio.uio_iovcnt = 1;
 uio.uio_procp = 0;
 e = VOP_WRITE(vp,&uio,IO_SYNC|IO_DSYNC,initproc->p_ucred);
 if (e)
  { printf("can't save buf: VOP_WRITE error %d\n",e);
  }
 VATTR_NULL(&vattr);
 vattr.va_size = dbgptr;
 e = VOP_SETATTR(vp,&vattr,initproc->p_ucred,initproc);
 if (e)
  { printf("setting size: VOP_SETATTR error %d\n",e);
  }
 VOP_UNLOCK(vp,0);
 vn_close(vp,FREAD|FWRITE,initproc->p_ucred,initproc);
}

/*
 * The heap_xxx routines must all be called at splclock or higher.
 *
 * The interface to heap_add is a bit odd.  It returns 0 on success; if
 *  the heap-as-allocated is too small, it returns the number of
 *  elements it would like the heap to be grown to.  The caller should
 *  malloc that many (with type M_TEMP) and call heap_grow with the
 *  timer-to-be-added (the one that was passed to heap_add) and the new
 *  heap vector.  Because of the waiting implicit in the malloc,
 *  heap_grow may itself find the heap too small; its return value
 *  semantics are the same as heap_add's.  (heap_grow always either
 *  uses or frees the vector passed to it; the caller should never try
 *  to free the vector, even if heap_grow returns nonzero.)
 */

static void heap_sanity(void)
{
 int i;
 int n;

 n = nrunning;
 if (n < 0) panic("heap_sanity nrunning<0");
 if (n > heapa) panic("heap_sanity nrunning>heapa");
 for (i=0;i<n;i++)
  { PCB *t;
    t = heap[i];
    if (t->hx != i) panic("heap_sanity index wrong (heap[%d]->hx = %d)",i,t->hx);
    if (i > 0)
     { int u;
       u = (i-1) >> 1;
       if (tvcmp(&heap[i]->itv.it_value,<,&heap[u]->itv.it_value))
	{ splhigh();
	  dump_info_to_buf(i,u);
	  save_buf_to_file();
	  panic("heap_sanity heap invariant violated: [%d]=%lu.%06lu, [%d]=%lu.%06lu\n",i,(unsigned long int)heap[i]->itv.it_value.tv_sec,(unsigned long int)heap[i]->itv.it_value.tv_usec,u,(unsigned long int)heap[u]->itv.it_value.tv_sec,(unsigned long int)heap[u]->itv.it_value.tv_usec);
	}
     }
  }
}

static void heap_bubble_up(int x, PCB *t)
{
 int u;

 while (x > 0)
  { u = (x-1) >> 1;
    if (tvcmp(&t->itv.it_value,>=,&heap[u]->itv.it_value)) break;
    (heap[x]=heap[u])->hx = x;
    x = u;
  }
 heap[x] = t;
 t->hx = x;
}

/* we need to bubble either up or down, not both, but telling which
   involves an extra test, roughly equivalent to doing both anyway. */
static void heap_remove(PCB *t)
{
 int x;
 int l;
 int r;
 int s;
 PCB *xpcb;
 int n;

 heap_sanity();
 n = --nrunning;
 if (n < 0) panic("timer heap_remove");
 if (n < 1) return;
 x = t->hx;
 xpcb = heap[n];
 while (1)
  { l = x + x + 1;
    r = l + 1;
    if ((l < n) && tvcmp(&heap[l]->itv.it_value,<,&xpcb->itv.it_value))
     { if ((r < n) && tvcmp(&heap[r]->itv.it_value,<,&xpcb->itv.it_value))
	{ if (tvcmp(&heap[l]->itv.it_value,<,&heap[r]->itv.it_value))
	   { s = l;
	   }
	  else
	   { s = r;
	   }
	}
       else
	{ s = l;
	}
     }
    else
     { if ((r < n) && tvcmp(&heap[r]->itv.it_value,<,&xpcb->itv.it_value))
	{ s = r;
	}
       else
	{ heap_bubble_up(x,xpcb);
	  heap_sanity();
	  return;
	}
     }
    (heap[x]=heap[s])->hx = x;
    x = s;
  }
}

static int heap_add(PCB *t)
{
 int n;

 heap_sanity();
 n = nrunning;
 if (n >= heapa) return(n+8);
 nrunning ++;
 heap_bubble_up(n,t);
 heap_sanity();
 return(0);
}

static int heap_grow(PCB *t, PCB **newv, int vlen)
{
 int n;

 heap_sanity();
 n = nrunning;
 if ((n >= vlen) && (n >= heapa))
  { free(newv,M_TEMP);
    return(n+8);
  }
 if (n < heapa)
  { free(newv,M_TEMP);
  }
 else
  { if (heap)
     { bcopy(heap,newv,n*sizeof(PCB *));
       free(heap,M_TEMP);
     }
    heap = newv;
    heapa = vlen;
  }
 nrunning ++;
 heap_bubble_up(n,t);
 heap_sanity();
 return(0);
}

static int timer_gentick(PCB *t)
{
 struct mbuf *m;
 struct timersock_event ev;
 int rv;

 if (! t->emerg) return(0);
 if (sbspace(&t->socket->so_rcv) < sizeof(struct timersock_event))
  { ((volatile PCB *)t)->pending ++;
    if (timerdebug) printf("gentick %p no space\n",(void *)t);
    return(0);
  }
 bzero(&ev,sizeof(ev));
 MGET(m,M_NOWAIT,MT_DATA);
 if (m == 0)
  { m = ((volatile PCB *)t)->emerg;
    ((volatile PCB *)t)->emerg = 0;
    ev.tse_type = TS_EV_ERROR;
    ev.tse_err = ENOBUFS;
    rv = 0;
    if (timerdebug) printf("gentick %p used emerg\n",(void *)t);
  }
 else
  { ev.tse_type = TS_EV_TICK;
    ev.tse_tv = time;
    rv = 1;
    if (timerdebug) printf("gentick %p normal\n",(void *)t);
  }
 *mtod(m,struct timersock_event *) = ev;
 m->m_len = sizeof(struct timersock_event);
 sbappend(&t->socket->so_rcv,m);
 sorwakeup(t->socket);
 return(rv);
}

static void timer_tick(void *arg __attribute__((__unused__)))
{
 int s;
 PCB *t;

 s = splclock();
 while ((nrunning > 0) && tvcmp(&(t=heap[0])->itv.it_value,<=,&time))
  { if (timerdebug) printf("timer_tick %p fired\n",(void *)t);
    heap_remove(t);
    if (timerisset(&t->itv.it_interval))
     { do
	{ timer_gentick(t);
	  timeradd(&t->itv.it_value,&t->itv.it_interval,&t->itv.it_value);
	} while (tvcmp(&t->itv.it_value,<=,&time));
       if (heap_add(t)) panic("timer_tick heap_add");
       if (timerdebug) printf("timer_tick %p reinstalled %llu.%06lu\n",(void *)t,t->itv.it_value.tv_sec,t->itv.it_value.tv_usec);
     }
    else
     { timer_gentick(t);
       timerclear(&t->itv.it_value);
       t->flags &= ~TPF_RUNNING;
       if (timerdebug) printf("timer_tick %p done\n",(void *)t);
     }
  }
 if (nrunning > 0)
  { timeout(timer_tick,0,1);
  }
 else
  { ticking = 0;
    if (timerdebug) printf("timer_tick stopping ticker\n");
  }
 splx(s);
}

static void timer_halt(PCB *pcb)
{
 int s;

 s = splclock();
 if (! (pcb->flags & TPF_RUNNING))
  { /* This can happen; in particular, timer_set does not splclock()
       around its testing TPF_RUNNING and calling timer_halt().  A
       clock interrupt after the test and before the call could cause
       the timer to be not-running by the time we get here. */
    splx(s);
    return;
  }
 pcb->flags &= ~TPF_RUNNING;
 heap_remove(pcb);
 splx(s);
}

/* The use of splclock()/splx() for locking presumes a uniprocessor system;
   see the comments on the heap_* routines for doc on the peculiar
   heap_add/heap_grow interface. */
static void timer_start(PCB *pcb)
{
 int s;
 int i;

 s = splclock();
 if (timerdebug) printf("timer_start %p heap %d/%d\n",(void *)pcb,nrunning,heapa);
 i = heap_add(pcb);
 while (i)
  { PCB **newheap;
    splx(s);
    newheap = malloc(i*sizeof(PCB *),M_TEMP,M_WAITOK);
    s = splclock();
    if (timerdebug) printf("timer_start %p heap_grow %d\n",(void *)pcb,i);
    i = heap_grow(pcb,newheap,i);
  }
 pcb->flags |= TPF_RUNNING;
 if (! ticking)
  { if (timerdebug) printf("timer_start %p starting ticker\n",(void *)pcb);
    timeout(timer_tick,0,1);
    ticking = 1;
  }
 else
  { if (timerdebug) printf("timer_start %p ticker already running\n",(void *)pcb);
  }
 splx(s);
}

static int timer_set(PCB *pcb, struct mbuf *m)
{
 struct itimerval itv;
 int e;

 m = m_pullup(m,sizeof(struct itimerval));
 if (m == 0) return(EINVAL);
 if (pcb->flags & TPF_RUNNING) timer_halt(pcb);
 itv = *mtod(m,struct itimerval *);
 m_freem(m);
 e = itimerfix(&itv.it_value);
 if (e) return(e);
 e = itimerfix(&itv.it_interval);
 if (e) return(e);
 if (timerisset(&itv.it_value))
  { timeradd(&time,&itv.it_value,&pcb->itv.it_value);
    pcb->itv.it_interval = itv.it_interval;
    if (timerdebug) printf("timer_set %p start %llu.%06lu+%llu.%06lu=%llu.%06lu\n",(void *)pcb,time.tv_sec,time.tv_usec,itv.it_value.tv_sec,itv.it_value.tv_usec,pcb->itv.it_value.tv_sec,pcb->itv.it_value.tv_usec);
    timer_start(pcb);
  }
 else
  { pcb->itv = itv;
    if (timerdebug) printf("timer_set %p unset\n",(void *)pcb);
  }
 return(0);
}

static int timer_ctloutput(int op, struct socket *so, int level, int optname, struct mbuf **mp)
{
 if ((op == PRCO_SETOPT) && *mp) m_free(*mp);
 return(EINVAL);
}

static void timer_readsome(PCB *pcb)
{
 int s;
 struct mbuf *m;

 s = splclock();
 while (1)
  { if (pcb->emerg == 0)
     { splx(s);
       MGET(m,M_WAIT,MT_DATA);
       s = splclock();
       if (pcb->emerg)
	{ m_freem(m);
	}
       else
	{ ((volatile PCB *)pcb)->emerg = m;
	}
       continue;
     }
    if (pcb->pending)
     { while (pcb->pending)
	{ pcb->pending --;
	  if (! timer_gentick(pcb)) goto done; /* break 2 */
	}
       continue;
     }
    break;
  }
done:;
 splx(s);
}

static int timer_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *nam, struct mbuf *control, struct proc *p)
{
 PCB *pcb;

 pcb = (void *) so->so_pcb;
 if ((pcb == 0) && (req != PRU_ATTACH)) return(EINVAL);
 switch (req)
  { case PRU_ATTACH:
       if (pcb) return(EISCONN);
       pcb = malloc(sizeof(PCB),M_PCB,M_NOWAIT);
       if (pcb == 0) return(ENOBUFS);
       if ((so->so_snd.sb_hiwat == 0) || (so->so_rcv.sb_hiwat == 0))
	{ int e;
	  e = soreserve(so,256,8192);
	  if (e) return(e);
	}
       pcb->socket = so;
       pcb->flags = 0;
       pcb->pending = 0;
       pcb->emerg = m_get(M_WAIT,MT_DATA);
       so->so_pcb = pcb;
       soisconnected(so);
       if (timerdebug) printf("timer PRU_ATTACH, pcb %p\n",(void *)pcb);
       break;
    case PRU_DETACH:
       if (timerdebug) printf("timer PRU_DETACH, pcb %p\n",(void *)pcb);
       if (pcb->flags & TPF_RUNNING) timer_halt(pcb);
       if (pcb->emerg) m_freem(pcb->emerg);
       free(pcb,M_PCB);
       so->so_pcb = 0;
       break;
    case PRU_CONTROL:
	{ unsigned long int cmd;
	  void *data;
	  if (pcb == 0) return(ENXIO);
	  cmd = (unsigned long int) m;
	  data = nam;
	  printf("timer PRU_CONTROL, pcb %p cmd %08lx data %p\n",(void *)pcb,cmd,data);
	  switch (cmd)
	   { case _IOW('~',0,int):
		timerdebug = *(int *)data;
		break;
	     case _IOR('~',1,int):
		*(int *)data = timerdebug;
		break;
	     default:
		return(ENOTTY);
		break;
	   }
	  return(0);
	}
       break;
    case PRU_BIND:
    case PRU_LISTEN:
    case PRU_CONNECT:
    case PRU_CONNECT2:
    case PRU_ACCEPT:
       return(EOPNOTSUPP);
       break;
    case PRU_DISCONNECT:
       soisdisconnected(so);
       break;
    case PRU_SHUTDOWN:
       socantsendmore(so);
       pcb->flags |= TPF_NOWRITES;
       break;
    case PRU_RCVD:
       timer_readsome(pcb);
       break;
    case PRU_SEND:
       if (nam)
	{ m_freem(m);
	  return(EINVAL);
	}
       return(timer_set(pcb,m));
       break;
    case PRU_ABORT:
       panic("timer_usrreq PRU_ABORT");
       break;
    case PRU_SENSE:
	{ struct stat *stp;
	  static struct timespec zts = { 0, 0 };
	  stp = (void *) m;
	  stp->st_blksize = so->so_rcv.sb_hiwat;
	  stp->st_dev = NODEV;
	  stp->st_atimespec = zts;
	  stp->st_mtimespec = zts;
	  stp->st_ctimespec = zts;
	  stp->st_ino = 0;
	}
       break;
    case PRU_RCVOOB:
    case PRU_SOCKADDR:
    case PRU_PEERADDR:
       return(EOPNOTSUPP);
       break;
    case PRU_SENDOOB:
       m_freem(m);
       return(EOPNOTSUPP);
       break;
    default:
       panic("timer_usrreq");
       break;
  }
 return(0);
}

extern struct domain timerdomain;

static struct protosw timersw[]
 = { { SOCK_STREAM,
       &timerdomain,
       0/*pr_protocol*/,
       PR_ATOMIC|PR_WANTRCVD/*pr_flags*/,
       0/*pr_input*/,
       0/*pr_output*/,
       0/*pr_ctlinput*/,
       timer_ctloutput,
       timer_usrreq,
       0/*pr_init*/,
       0/*pr_fasttimo*/,
       0/*pr_slowtimo*/,
       0/*pr_drain*/,
       0/*pr_sysctl*/ } };

struct domain timerdomain
 = { PF_TIMER,
     "timers",
     0/*dom_init*/,
     0/*dom_externalize*/,
     0/*dom_dispose*/,
     timersw,
     &timersw[sizeof(timersw)/sizeof(timersw[0])],
     0/*dom_next*/,
     0/*dom_rtattach*/,
     0/*dom_rtoffset*/,
     0/*dom_maxrtkey*/ };
