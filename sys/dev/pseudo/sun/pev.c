/*
 * Pseudo-event driver.  Provides the same interface to userland as the
 *  keyboard/mouse drivers, but is driven by a master half rather than
 *  a serial line.
 *
 * We'd like to be a pseudo-device which can have a kbd/ms instance
 *  attach to it, but pseudo-devices don't exist as far as the device
 *  attachment machinery is concerned.
 */

#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/filio.h>
#include <sys/vnode.h>
#include <sys/ttycom.h>
#include <sys/kernel.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <machine/kbd.h>
#include <machine/intr.h>
#include <machine/kbio.h>
#include <sys/signalvar.h>
#include <dev/sun/event_var.h>
#include <machine/vuid_event.h>

/* cdevsw glue */
cdev_decl(pevm);
cdev_decl(pevs);

/* pseudo-device attach glue */
void pevattach(int);

typedef struct pev_softc SOFTC;

#define PKOBSIZE 8

struct pev_softc {
  int unit;
  unsigned int flags;
#define PEF_SOPEN 0x00000001
#define PEF_MOPEN 0x00000002
#define PEF_OWANT 0x00000004
#define PEF_OWAIT 0x00000008
  unsigned int ocbhead;
  unsigned int ocbtail;
  unsigned int ocbn;
  unsigned char ocbuf[PKOBSIZE];
  struct selinfo msel;
  struct evvar events;
  } ;

static int npev;
static SOFTC *pev;

void pevattach(int n)
{
 void *mem;
 int i;
 SOFTC *sc;

 npev = n;
 mem = malloc(npev*sizeof(SOFTC),M_DEVBUF,M_NOWAIT);
 if (mem == 0)
  { printf("pev: attach malloc failed\n");
    return;
  }
 pev = mem;
 for (i=0;i<npev;i++)
  { sc = &pev[i];
    sc->unit = i;
    sc->flags = 0;
    sc->ocbhead = 0;
    sc->ocbtail = 0;
    sc->ocbn = 0;
  }
}

int pevsopen(dev_t dev, int flags, int mode, struct proc *p)
{
 unsigned int u;
 SOFTC *sc;
 int s;

 u = minor(dev);
 if (u >= npev) return(ENXIO);
 sc = &pev[u];
 s = splhigh();
 if (sc->flags & PEF_SOPEN)
  { splx(s);
    return(EBUSY);
  }
 sc->flags |= PEF_SOPEN;
 splx(s);
 ev_init(&sc->events);
 /* XXX ev_init() is broken; it doesn't initialize most of the struct!
    It sets ev_get, ev_put, and ev_q, but none of the rest.  Worse,
    there is no documentedly-correct way to initialize a struct selinfo,
    making it difficult to do it ourselves; using a static struct like
    this is ugly, but it works.... */
  { static const struct selinfo si_init;
    sc->events.ev_sel = si_init;
    sc->events.ev_io = p;
    sc->events.ev_wanted = 0;
    sc->events.ev_async = 0;
  }
 return(0);
}

int pevsclose(dev_t dev, int flags, int mode, struct proc *p)
{
 unsigned int u;
 SOFTC *sc;
 int s;

 u = minor(dev);
 if (u >= npev) panic("pevsclose: closing impossible pev");
 sc = &pev[u];
 ev_fini(&sc->events);
 s = splhigh();
 sc->flags &= ~PEF_SOPEN;
 sc->events.ev_io = 0;
 splx(s);
 return(0);
}

int pevsread(dev_t dev, struct uio *uio, int flags)
{
 return(ev_read(&pev[minor(dev)].events,uio,flags));
}

int pevswrite(dev_t dev, struct uio *uio, int flags)
{
 SOFTC *sc;
 int e;
 unsigned char c;

 sc = &pev[minor(dev)];
 while (uio->uio_resid)
  { e = uiomove(&c,1,uio);
    if (e) return(e);
    while (1)
     { if (! (sc->flags & PEF_MOPEN)) return(EIO);
       if (sc->ocbn < PKOBSIZE) break;
       sc->flags |= PEF_OWANT;
       e = tsleep(sc,PZERO|PCATCH,"pevswr",0);
       if (e) return(e);
     }
    sc->ocbuf[sc->ocbhead] = c;
    if (sc->ocbhead >= PKOBSIZE-1) sc->ocbhead = 0; else sc->ocbhead ++;
    sc->ocbn ++;
    if (sc->flags & PEF_OWAIT)
     { sc->flags &= ~PEF_OWAIT;
       wakeup(sc);
     }
  }
 return(0);
}

int pevsioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;

 sc = &pev[minor(dev)];
 switch (cmd)
  { case KIOCTRANS:
       if (*(int *)data != TR_UNTRANS_EVENT) return(EINVAL);
       break;
    case KIOCGTRANS:
       *(int *)data = TR_UNTRANS_EVENT;
       break;
    case KIOCSDIRECT:
       if (! *(int *)data) return(EINVAL);
       break;
    case KIOCTYPE:
       *(int *)data = KB_SUN3;
       break;
    case FIONBIO:
       break;
    case FIOASYNC:
       sc->events.ev_async = *(int *)data != 0;
       break;
    case TIOCSPGRP:
       if (! sc->events.ev_io)
	{ printf("pevsioctl: TIOCSPGRP: nil ev_io\n");
	  return(EIO);
	}
       if (*(int *)data != sc->events.ev_io->p_pgid) return(EPERM);
       break;
    case VUIDGFORMAT:
       *(int *)data = VUID_FIRM_EVENT;
       break;
    case VUIDSFORMAT:
       if (*(int *)data != VUID_FIRM_EVENT) return(EINVAL);
       break;
    default:
       return(ENOTTY);
       break;
  }
 return(0);
}

int pevspoll(dev_t dev, int events, struct proc *p)
{
 return(ev_poll(&pev[minor(dev)].events,events,p));
}

int pevmopen(dev_t dev, int flags, int mode, struct proc *p)
{
 unsigned int u;
 SOFTC *sc;
 int s;

 u = minor(dev);
 if (u >= npev) return(ENXIO);
 sc = &pev[u];
 s = splhigh();
 if (sc->flags & PEF_MOPEN)
  { splx(s);
    return(EBUSY);
  }
 sc->flags |= PEF_MOPEN;
 splx(s);
 return(0);
}

int pevmclose(dev_t dev, int flags, int mode, struct proc *p)
{
 unsigned int u;
 SOFTC *sc;
 int s;

 u = minor(dev);
 if (u >= npev) panic("pevmclose: closing impossible pev");
 sc = &pev[u];
 s = splhigh();
 sc->flags &= ~PEF_MOPEN;
 splx(s);
 wakeup(sc);
 return(0);
}

int pevmread(dev_t dev, struct uio *uio, int flags)
{
 SOFTC *sc;
 int e;

 if (uio->uio_resid < 1) return(0);
 sc = &pev[minor(dev)];
 while (1)
  { if (sc->ocbn > 0) break;
    if (flags & IO_NDELAY) return(EWOULDBLOCK);
    sc->flags |= PEF_OWAIT;
    e = tsleep(sc,PZERO|PCATCH,"pevmrd",0);
    if (e) return(e);
  }
 while (sc->ocbn && uio->uio_resid)
  { e = uiomove(&sc->ocbuf[sc->ocbtail],1,uio);
    if (e) return(e);
    if (sc->ocbtail >= PKOBSIZE-1) sc->ocbtail = 0; else sc->ocbtail ++;
    sc->ocbn --;
  }
 if (sc->flags & PEF_OWANT)
  { sc->flags &= ~PEF_OWANT;
    wakeup(sc);
  }
 return(0);
}

int pevmwrite(dev_t dev, struct uio *uio, int flags)
{
 SOFTC *sc;
 int e;
 struct firm_event ev;
 int put;
 struct firm_event *ep;

 sc = &pev[minor(dev)];
 while (uio->uio_resid >= sizeof(ev))
  { e = uiomove(&ev,sizeof(ev),uio);
    if (e) return(e);
    if (sc->flags & PEF_SOPEN)
     { put = sc->events.ev_put;
       ep = &sc->events.ev_q[put];
       if (put >= EV_QSIZE-1) put = 0; else put ++;
       if (put != sc->events.ev_get)
	{ *ep = ev;
	  ep->time = time;
	  sc->events.ev_put = put;
	  EV_WAKEUP(&sc->events);
	}
     }
  }
 return(0);
}

int pevmioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;

 sc = &pev[minor(dev)];
 switch (cmd)
  { case FIONBIO:
       break;
    case FIOASYNC:
       sc->events.ev_async = (*(int *)data != 0);
       break;
    default:
       return(ENOTTY);
       break;
  }
 return(0);
}

int pevmpoll(dev_t dev, int events, struct proc *p)
{
 SOFTC *sc;
 int revents;

 sc = &pev[minor(dev)];
 revents = events & (POLLOUT | POLLWRNORM);
 if (events & (POLLIN|POLLRDNORM))
  { if (sc->ocbn == 0)
     { selrecord(p,&sc->msel);
     }
    else
     { revents |= events & (POLLIN | POLLRDNORM);
     }
  }
 return(revents);
}
