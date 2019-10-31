/*	$NetBSD: ms.c,v 1.17 1999/08/02 01:50:27 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Mouse driver (/dev/mouse)
 */

/*
 * Zilog Z8530 Dual UART driver (mouse interface)
 *
 * This is the "slave" driver that will be attached to
 * the "zsc" driver for a Sun mouse.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/fcntl.h>

#include <machine/vuid_event.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <dev/sun/event_var.h>
#include <dev/sun/msvar.h>

#include "locators.h"

cdev_decl(ms);	/* open, close, read, write, ioctl, stop, ... */

extern struct cfdriver ms_cd;

#define UNIT_UNIT(u) ((u)&0xf)
#define UNIT_FLAGS(u) ((u)&0xf0)
#define UNIT_FLAG_RAW 0x10

/****************************************************************
 *  Entry points for /dev/mouse
 *  (open,close,read,write,...)
 ****************************************************************/

int msopen(dev_t dev, int flags, int mode, struct proc *p)
{
 struct ms_softc *ms;
 int unit;
 unsigned int mflags;

 unit = minor(dev);
 mflags = UNIT_FLAGS(unit);
 unit = UNIT_UNIT(unit);
 if (unit >= ms_cd.cd_ndevs) return(ENXIO);
 ms = ms_cd.cd_devs[unit];
 if (! ms) return(ENXIO);
 // This is an exclusive open device.
 if (ms->flags & MSF_OPEN) return(EBUSY);
 ms->flags |= MSF_OPEN;
 if (mflags & UNIT_FLAG_RAW)
  { if (flags & O_NONBLOCK) ms->flags |= MSF_NBIO; else ms->flags &= ~MSF_NBIO;
    ms->u.raw.h = 0;
    ms->u.raw.t = 0;
    // XXX What's the right way to initialize a struct selinfo?
     { static const struct selinfo selinit;
       ms->u.raw.rsel = selinit;
     }
    ms->flags |= MSF_RAW;
  }
 else
  { ms->flags &= ~MSF_RAW;
    ms->u.ev.ms_events.ev_io = p;
    ev_init(&ms->u.ev.ms_events); // may sleep
  }
 ms->ms_ready = 1;		/* start accepting events */
 return(0);
}

int msclose(dev_t dev, int flags, int mode, struct proc *p)
{
 struct ms_softc *ms;

 ms = ms_cd.cd_devs[UNIT_UNIT(minor(dev))];
 ms->ms_ready = 0;		/* stop accepting events */
 if (ms->flags & MSF_RAW)
  { ms->flags &= ~MSF_RAW;
  }
 else
  { ev_fini(&ms->u.ev.ms_events);
    ms->u.ev.ms_events.ev_io = 0;
  }
 ms->flags &= ~MSF_OPEN;
 return(0);
}

int msread(dev_t dev, struct uio *uio, int flags)
{
 struct ms_softc *ms;
 int s;

 ms = ms_cd.cd_devs[UNIT_UNIT(minor(dev))];
 if (ms->flags & MSF_RAW)
  { unsigned int h;
    unsigned int t;
    int n;
    n = 0;
    while (uio->uio_resid)
     { s = splhigh();
       h = ms->u.raw.h;
       t = ms->u.raw.t;
       splx(s);
       if (h == t)
	{ int e;
	  if ((n > 0) || (ms->flags & MSF_NBIO)) return(0);
	  e = tsleep(ms,PZERO|PCATCH,"msraw",0);
	  if (e) return(e);
	  continue;
	}
       uiomove(&ms->u.raw.ring[t],1,uio);
       n ++;
       s = splhigh();
       ms->u.raw.t = MS_RING_ADV(t);
       splx(s);
     }
    return(0);
  }
 else
  { return(ev_read(&ms->u.ev.ms_events,uio,flags));
  }
}

/* this routine should not exist, but is convenient to write here for now */
int mswrite(dev_t dev, struct uio *uio,int flags)
{
 return(EOPNOTSUPP);
}

int msioctl(dev_t dev,u_long cmd, caddr_t data, int flag,struct proc *p)
{
 struct ms_softc *ms;

 ms = ms_cd.cd_devs[UNIT_UNIT(minor(dev))];
 switch (cmd)
  { case FIONBIO:
       if (ms->flags & MSF_RAW)
	{ if (*(int *)data) ms->flags |= MSF_NBIO; else ms->flags &= ~MSF_NBIO;
	}
       return(0);
       break;
    case FIOASYNC:
       if (ms->flags & MSF_RAW) return(*(int *)data?EOPNOTSUPP:0);
       ms->u.ev.ms_events.ev_async = *(int *)data != 0;
       return(0);
       break;
    case TIOCSPGRP:
       if (ms->flags & MSF_RAW) return(EOPNOTSUPP);
       if (*(int *)data != ms->u.ev.ms_events.ev_io->p_pgid) return(EPERM);
       return(0);
       break;
    case VUIDGFORMAT:
       if (ms->flags & MSF_RAW) return(EOPNOTSUPP);
       // we do only firm_events
       *(int *)data = VUID_FIRM_EVENT;
       return(0);
       break;
    case VUIDSFORMAT:
       if (ms->flags & MSF_RAW) return(EOPNOTSUPP);
       if (*(int *)data != VUID_FIRM_EVENT) return(EINVAL);
       return(0);
  }
 return(ENOTTY);
}

int mspoll(dev_t dev, int events, struct proc *p)
{
 struct ms_softc *ms;
 int revents;
 int s;
 int any;

 ms = ms_cd.cd_devs[UNIT_UNIT(minor(dev))];
 if (ms->flags & MSF_RAW)
  { revents = events & (POLLOUT | POLLWRNORM);
    s = splhigh();
    any = (ms->u.raw.h != ms->u.raw.t);
    splx(s);
    if (any)
     { revents |= events & (POLLIN | POLLRDNORM);
     }
    else
     { selrecord(p,&ms->u.raw.rsel);
     }
    return(revents);
  }
 else
  { return(ev_poll(&ms->u.ev.ms_events,events,p));
  }
}

/****************************************************************
 * Middle layer (translator)
 ****************************************************************/

static void ms_input_event(struct ms_softc *ms, int c)
{
 register struct firm_event *fe;
 register int mb, ub, d, get, put, any;
 static const char to_one[] = { 1, 2, 2, 4, 4, 4, 4 };
 static const int to_id[] = { MS_RIGHT, MS_MIDDLE, 0, MS_LEFT };

 /*
  * Discard input if not ready.  Drop sync on parity or framing error;
  *  gain sync on button byte.
  */
 if (! ms->ms_ready) return;
 if (c == -1)
  { ms->ms_state = -1;
    return;
  }
 if ((c & ~0x0f) == 0x80)	/* if in 0x80..0x8f */
  { if (c & 8)
     { ms->ms_state = 1;	/* short form (3 bytes) */
     }
    else
     { ms->ms_state = 0;	/* long form (5 bytes) */
     }
  }
 /*
  * Run the decode loop, adding to the current information.  We add,
  *  rather than replace, deltas, so that, if the event queue fills, we
  *  accumulate deltas for when it opens up again.
  */
 switch (ms->ms_state)
  { case -1:
       return;
       break;
    case 0:
       // buttons (long form)
       ms->ms_state = 2;
       ms->ms_mb = (~c) & 0x7;
       return;
       break;
    case 1:
       // buttons (short form)
       ms->ms_state = 4;
       ms->ms_mb = (~c) & 0x7;
       return;
       break;
    case 2:
       // first delta-x
       ms->ms_state = 3;
       ms->ms_dx += (signed char) c;
       return;
       break;
    case 3:
       // first delta-y
       ms->ms_state = 4;
       ms->ms_dy += (signed char) c;
       return;
       break;
    case 4:
       // second delta-x
       ms->ms_state = 5;
       ms->ms_dx += (signed char) c;
       return;
       break;
    case 5:
       // second delta-y
       ms->ms_state = -1;	// wait for button-byte again
       ms->ms_dy += (signed char) c;
       break;
    default:
       panic("ms_rint");
       /* NOTREACHED */
  }
 /*
  * We have at least one event (mouse button, delta-X, or delta-Y;
  *  possibly all three, and possibly three separate button events).
  *  Deliver these events until we are out of changes or out of room.
  *  As events get delivered, mark them `unchanged'.
  */
 any = 0;
 get = ms->u.ev.ms_events.ev_get;
 put = ms->u.ev.ms_events.ev_put;
 fe = &ms->u.ev.ms_events.ev_q[put];
 /* NEXT prepares to put the next event, backing off if necessary */
#define	NEXT \
	if ((++put) % EV_QSIZE == get) { \
		put--; \
		break; \
	}
	/* ADVANCE completes the `put' of the event */
#define	ADVANCE \
	fe++; \
	if (put >= EV_QSIZE) { \
		put = 0; \
		fe = &ms->u.ev.ms_events.ev_q[0]; \
	} \
	any = 1
 mb = ms->ms_mb;
 ub = ms->ms_ub;
 do
  { while ((d = mb ^ ub) != 0)
     { /*
	* Mouse button change.  Convert up to three changes to the
	*  `first' change, and drop it into the event queue.
	*/
       NEXT;
       d = to_one[d-1];		/* from 1..7 to {1,2,4} */
       fe->id = to_id[d-1];	/* from {1,2,4} to ID */
       fe->value = (mb & d) ? VKEY_DOWN : VKEY_UP;
       fe->time = time;
       ADVANCE;
       ub ^= d;
     }
    if (ms->ms_dx)
     { NEXT;
       fe->id = LOC_X_DELTA;
       fe->value = ms->ms_dx;
       fe->time = time;
       ADVANCE;
       ms->ms_dx = 0;
     }
    if (ms->ms_dy)
     { NEXT;
       fe->id = LOC_Y_DELTA;
       fe->value = ms->ms_dy;
       fe->time = time;
       ADVANCE;
       ms->ms_dy = 0;
     }
  } while (0);
 if (any)
  { ms->ms_ub = ub;
    ms->u.ev.ms_events.ev_put = put;
    EV_WAKEUP(&ms->u.ev.ms_events);
  }
}

static void ms_input_raw(struct ms_softc *ms, int c)
{
 int h;
 int t;
 int nh;

 if (! ms->ms_ready) return;
 h = ms->u.raw.h;
 t = ms->u.raw.t;
 nh = MS_RING_ADV(h);
 if (nh != t)
  { ms->u.raw.ring[h] = c;
    ms->u.raw.h = nh;
  }
 wakeup(ms);
 selwakeup(&ms->u.raw.rsel);
}

// Called from ms_zs.c on input.
void ms_input(struct ms_softc *ms, int c)
{
 if (ms->flags & MSF_RAW)
  { ms_input_raw(ms,c);
  }
 else
  { ms_input_event(ms,c);
  }
}
