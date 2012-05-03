/*	$NetBSD: lpt.c,v 1.55 1999/03/29 21:50:06 perry Exp $	*/

/*
 * Copyright (c) 1993, 1994 Charles M. Hannum.
 * Copyright (c) 1990 William F. Jolitz, TeleMuse
 * All rights reserved.
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
 *	This software is a component of "386BSD" developed by
 *	William F. Jolitz, TeleMuse.
 * 4. Neither the name of the developer nor the name "386BSD"
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS A COMPONENT OF 386BSD DEVELOPED BY WILLIAM F. JOLITZ
 * AND IS INTENDED FOR RESEARCH AND EDUCATIONAL PURPOSES ONLY. THIS
 * SOFTWARE SHOULD NOT BE CONSIDERED TO BE A COMMERCIAL PRODUCT.
 * THE DEVELOPER URGES THAT USERS WHO REQUIRE A COMMERCIAL PRODUCT
 * NOT MAKE USE OF THIS WORK.
 *
 * FOR USERS WHO WISH TO UNDERSTAND THE 386BSD SYSTEM DEVELOPED
 * BY WILLIAM F. JOLITZ, WE RECOMMEND THE USER STUDY WRITTEN
 * REFERENCES SUCH AS THE  "PORTING UNIX TO THE 386" SERIES
 * (BEGINNING JANUARY 1991 "DR. DOBBS JOURNAL", USA AND BEGINNING
 * JUNE 1991 "UNIX MAGAZIN", GERMANY) BY WILLIAM F. JOLITZ AND
 * LYNNE GREER JOLITZ, AS WELL AS OTHER BOOKS ON UNIX AND THE
 * ON-LINE 386BSD USER MANUAL BEFORE USE. A BOOK DISCUSSING THE INTERNALS
 * OF 386BSD ENTITLED "386BSD FROM THE INSIDE OUT" WILL BE AVAILABLE LATE 1992.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPER ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device Driver for AT style parallel printer port
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/syslog.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/lptreg.h>
#include <dev/ic/lptvar.h>

#define	TIMEOUT		hz*16	/* wait up to 16 seconds for a ready */
#define	STEP		hz/4

#define	LPTPRI		(PZERO+8)
#ifndef LPT_BSIZE
#define	LPT_BSIZE	1024
#endif

#define LPTDEBUG

#ifndef LPTDEBUG
#define LPRINTF(a)
#else
#define LPRINTF(a)	if (lptdebug) printf a
int lptdebug = 0;
#endif

/* XXX does not belong here */
cdev_decl(lpt);

extern struct cfdriver lpt_cd;

#define	LPTUNIT(s)	(minor(s) & 0x1f)
#define	LPTFLAGS(s)	(minor(s) & ~0x1f)

void
lpt_attach_subr(sc)
	struct lpt_softc *sc;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc->sc_state = 0;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	bus_space_write_1(iot, ioh, lpt_control, LPC_NINIT);

	sc->sc_dev_ok = 1;
}

/*
 * Reset the printer, then wait until it's selected and not busy.
 */
int
lptopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	unsigned short int flags = LPTFLAGS(dev);
	struct lpt_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_char control;
	int error;
	int spin;

	if (unit >= lpt_cd.cd_ndevs)
		return ENXIO;
	sc = lpt_cd.cd_devs[unit];
	if (!sc || !sc->sc_dev_ok)
		return ENXIO;

 if (flags & LPT_RAW)
  { if (sc->sc_state) return(EBUSY);
    sc->sc_state = LPT_OPEN;
    sc->sc_flags = flags;
    bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_control,0);
    return(0);
  }

#if 0	/* XXX what to do? */
	if (sc->sc_irq == IRQUNK && (flags & LPT_NOINTR) == 0)
		return ENXIO;
#endif

#ifdef DIAGNOSTIC
	if (sc->sc_state)
		printf("%s: stat=0x%x not zero\n", sc->sc_dev.dv_xname,
		    sc->sc_state);
#endif

	if (sc->sc_state)
		return EBUSY;

	sc->sc_state = LPT_INIT;
	sc->sc_flags = flags;
	LPRINTF(("%s: open: flags=0x%x\n", sc->sc_dev.dv_xname,
	    (unsigned)flags));
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	if ((flags & LPT_NOPRIME) == 0) {
		/* assert INIT for 100 usec to start up printer */
		bus_space_write_1(iot, ioh, lpt_control, LPC_SELECT);
		delay(100);
	}

	control = LPC_SELECT | LPC_NINIT;
	bus_space_write_1(iot, ioh, lpt_control, control);

	/* wait till ready (printer running diagnostics) */
	for (spin = 0; NOT_READY_ERR(); spin += STEP) {
		if (spin >= TIMEOUT) {
			sc->sc_state = 0;
			return EBUSY;
		}

		/* wait 1/4 second, give up if we get a signal */
		error = tsleep((caddr_t)sc, LPTPRI | PCATCH, "lptopen", STEP);
		if (error != EWOULDBLOCK) {
			sc->sc_state = 0;
			return error;
		}
	}

	if ((flags & LPT_NOINTR) == 0)
		control |= LPC_IENABLE;
	if (flags & LPT_AUTOLF)
		control |= LPC_AUTOLF;
	sc->sc_control = control;
	bus_space_write_1(iot, ioh, lpt_control, control);

	sc->sc_inbuf = malloc(LPT_BSIZE, M_DEVBUF, M_WAITOK);
	sc->sc_count = 0;
	sc->sc_state = LPT_OPEN;

	if ((sc->sc_flags & LPT_NOINTR) == 0)
		lptwakeup(sc);

	LPRINTF(("%s: opened\n", sc->sc_dev.dv_xname));
	return 0;
}

int
lptnotready(status, sc)
	u_char status;
	struct lpt_softc *sc;
{
	u_char new;

	status = (status ^ LPS_INVERT) & LPS_MASK;
	new = status & ~sc->sc_laststatus;
	sc->sc_laststatus = status;

	if (sc->sc_state & LPT_OPEN) {
		if (new & LPS_SELECT)
			log(LOG_NOTICE,
			    "%s: offline\n", sc->sc_dev.dv_xname);
		else if (new & LPS_NOPAPER)
			log(LOG_NOTICE,
			    "%s: out of paper\n", sc->sc_dev.dv_xname);
		else if (new & LPS_NERR)
			log(LOG_NOTICE,
			    "%s: output error\n", sc->sc_dev.dv_xname);
	}

	return status;
}

void
lptwakeup(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	int s;

	s = spllpt();
	lptintr(sc);
	splx(s);

	timeout(lptwakeup, sc, STEP);
}

/*
 * Close the device, and free the local line buffer.
 */
int
lptclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit = LPTUNIT(dev);
	struct lpt_softc *sc = lpt_cd.cd_devs[unit];
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

 if (sc->sc_flags & LPT_RAW)
  { sc->sc_flags = 0;
    sc->sc_state = 0;
    return(0);
  }

	if (sc->sc_count)
		(void) lptpushbytes(sc);

	if ((sc->sc_flags & LPT_NOINTR) == 0)
		untimeout(lptwakeup, sc);

	bus_space_write_1(iot, ioh, lpt_control, LPC_NINIT);
	sc->sc_state = 0;
	bus_space_write_1(iot, ioh, lpt_control, LPC_NINIT);
	free(sc->sc_inbuf, M_DEVBUF);

	LPRINTF(("%s: closed\n", sc->sc_dev.dv_xname));
	return 0;
}

int
lptpushbytes(sc)
	struct lpt_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int error;

	if (sc->sc_flags & LPT_NOINTR) {
		int spin, tic;
		u_char control = sc->sc_control;

		while (sc->sc_count > 0) {
			spin = 0;
			while (NOT_READY()) {
				if (++spin < sc->sc_spinmax)
					continue;
				tic = 0;
				/* adapt busy-wait algorithm */
				sc->sc_spinmax++;
				while (NOT_READY_ERR()) {
					/* exponential backoff */
					tic = tic + tic + 1;
					if (tic > TIMEOUT)
						tic = TIMEOUT;
					error = tsleep((caddr_t)sc,
					    LPTPRI | PCATCH, "lptpsh", tic);
					if (error != EWOULDBLOCK)
						return error;
				}
				break;
			}

			bus_space_write_1(iot, ioh, lpt_data, *sc->sc_cp++);
			bus_space_write_1(iot, ioh, lpt_control,
			    control | LPC_STROBE);
			sc->sc_count--;
			bus_space_write_1(iot, ioh, lpt_control, control);

			/* adapt busy-wait algorithm */
			if (spin*2 + 16 < sc->sc_spinmax)
				sc->sc_spinmax--;
		}
	} else {
		int s;

		while (sc->sc_count > 0) {
			/* if the printer is ready for a char, give it one */
			if ((sc->sc_state & LPT_OBUSY) == 0) {
				LPRINTF(("%s: write %lu\n", sc->sc_dev.dv_xname,
				    (u_long)sc->sc_count));
				s = spllpt();
				(void) lptintr(sc);
				splx(s);
			}
			error = tsleep((caddr_t)sc, LPTPRI | PCATCH,
			    "lptwrite2", 0);
			if (error)
				return error;
		}
	}
	return 0;
}

/*
 * Copy a line from user space to a local buffer, then call putc to get the
 * chars moved to the output queue.
 */
int
lptwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct lpt_softc *sc = lpt_cd.cd_devs[LPTUNIT(dev)];
	size_t n;
	int error = 0;

 if (sc->sc_flags & LPT_RAW)
  { unsigned char c[2];
    if (uio->uio_resid < 2) return(EINVAL);
    uiomove(&c[0],2,uio);
    c[1] &= LPC_STROBE | LPC_AUTOLF | LPC_NINIT | LPC_SELECT;
    bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,c[0]);
    bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_control,c[1]);
    return(0);
  }

	while ((n = min(LPT_BSIZE, uio->uio_resid)) != 0) {
		uiomove(sc->sc_cp = sc->sc_inbuf, n, uio);
		sc->sc_count = n;
		error = lptpushbytes(sc);
		if (error) {
			/*
			 * Return accurate residual if interrupted or timed
			 * out.
			 */
			uio->uio_resid += sc->sc_count;
			sc->sc_count = 0;
			return error;
		}
	}
	return 0;
}

/*
 * Handle printer interrupts which occur when the printer is ready to accept
 * another char.
 */
int
lptintr(arg)
	void *arg;
{
	struct lpt_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

#if 0
	if ((sc->sc_state & LPT_OPEN) == 0)
		return 0;
#endif

 if (sc->sc_flags & LPT_RAW) return(1);

	/* is printer online and ready for output */
	if (NOT_READY() && NOT_READY_ERR())
		return 0;

	if (sc->sc_count) {
		u_char control = sc->sc_control;
		/* send char */
		bus_space_write_1(iot, ioh, lpt_data, *sc->sc_cp++);
		DELAY(1);
		bus_space_write_1(iot, ioh, lpt_control, control | LPC_STROBE);
		DELAY(1);
		sc->sc_count--;
		bus_space_write_1(iot, ioh, lpt_control, control);
		sc->sc_state |= LPT_OBUSY;
	} else
		sc->sc_state &= ~LPT_OBUSY;

	if (sc->sc_count == 0) {
		/* none, wake up the top half to get more */
		wakeup((caddr_t)sc);
	}

	return 1;
}

struct rrblk {
  unsigned int len;
  unsigned char *buf;
  } ;

static int lpt_rawread(dev_t dev, const struct rrblk *rbp)
{
 struct lpt_softc *sc;
 int i;
 int n;
 unsigned char buf[256];

 sc = lpt_cd.cd_devs[LPTUNIT(dev)];
 if (! (sc->sc_flags & LPT_RAW)) return(ENOTTY);
 if (rbp->len < 1) return(0);
 if (rbp->len > 256) return(EMSGSIZE);
 n = rbp->len;
 i = copyin(rbp->buf,&buf[0],n);
 if (i) return(i);
 for (i=0;i<n;i++)
  { bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,buf[i]);
    buf[i] = bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status) ^ LPS_NBSY;
  }
 return(copyout(&buf[0],rbp->buf,n));
}

static void sendrecv(struct lpt_softc *sc, unsigned char s, unsigned char *rp)
{
 unsigned char r;
 int i;

 r = 0;
 bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,0xf3);
 for (i=0;i<8;i++)
  { DELAY(10);
    bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,(s&1)?0xfa:0xf8);
    DELAY(10);
    r >>= 1;
    if (bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status) & LPS_NOPAPER) r |= 0x80;
    bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,(s&1)?0xfb:0xf9);
    s >>= 1;
  }
 DELAY(10);
 if (rp) *rp = r;
}

static int getack(struct lpt_softc *sc)
{
 int i;

 for (i=100;i>0;i--)
  { if (! (bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status) & LPS_NACK)) break;
    DELAY(1);
  }
 if (! i)
  { printf("lpt getack: no response\n");
    return(1);
  }
 bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,0xf3);
 bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,0xfb);
 for (i=100;i>0;i--)
  { if (bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status) & LPS_NACK) break;
    DELAY(1);
  }
 if (! i) printf("lpt getack: NACK stuck low\n");
 return(0);
}

static int lpt_psxread(dev_t dev, char * const *dpp)
{
 struct lpt_softc *sc;
 int n;
 int rx;
 unsigned char rbuf[32];

 sc = lpt_cd.cd_devs[LPTUNIT(dev)];
 if (! (sc->sc_flags & LPT_RAW)) return(ENOTTY);
 bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,0xf3);
 sendrecv(sc,0x01,0);
 if (getack(sc)) return(EHOSTDOWN);
 sendrecv(sc,0x42,&rbuf[1]);
 switch (rbuf[1])
  { case 0x41: n = 3; break;
    case 0x23: n = 7; break;
    case 0x73: n = 7; break;
    case 0x53: n = 7; break;
    default: n = 0; break;
  }
 rbuf[0] = n + 1;
 rx = 2;
 for (;n>0;n--)
  { if (getack(sc)) return(EHOSTDOWN);
    sendrecv(sc,0xff,&rbuf[rx++]);
  }
 bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,0xff);
 return(copyout(&rbuf[0],*dpp,rbuf[0]+1));
}

int
lptioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error = 0;

	switch (cmd) {
	case _IOW('P', 0x3, struct rrblk):
		return(lpt_rawread(dev,(const void *)data));
		break;
	case _IOW('P', 0x4, char *):
		return(lpt_psxread(dev,(const void *)data));
		break;
	default:
		error = ENODEV;
	}

	return error;
}

int lptread(dev_t dev, struct uio *uio, int flags)
{
 unsigned char buf[256];
 struct lpt_softc *sc;
 int i;
 int n;

 sc = lpt_cd.cd_devs[LPTUNIT(dev)];
 if (! (sc->sc_flags & LPT_RAW)) return(0);
 n = uio->uio_resid;
 if (n < 1) return(EINVAL);
 if (sc->sc_flags & LPT_AUTOLF)
  { if (n > 256) n = 256;
    for (i=0;i<n;i++)
     { bus_space_write_1(sc->sc_iot,sc->sc_ioh,lpt_data,i);
       buf[i] = bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status);
     }
  }
 else
  { buf[0] = bus_space_read_1(sc->sc_iot,sc->sc_ioh,lpt_status);
    n = 1;
  }
 uiomove(&buf[0],n,uio);
 return(0);
}
