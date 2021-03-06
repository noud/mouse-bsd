/*	$NetBSD: zsms.c,v 1.6 2000/01/08 02:57:22 takemura Exp $	*/

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
 * VSXXX mice attached with channel A of the 1st SCC
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>

#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>

#include <dev/dec/lk201.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include "locators.h"

/*
 * How many input characters we can buffer.
 * The port-specific var.h may override this.
 * Note: must be a power of two!
 */
#define	ZSMS_RX_RING_SIZE	256
#define ZSMS_RX_RING_MASK (ZSMS_RX_RING_SIZE-1)
/*
 * Output buffer.  Only need a few chars.
 */
#define	ZSMS_TX_RING_SIZE	16
#define ZSMS_TX_RING_MASK (ZSMS_TX_RING_SIZE-1)

#define ZSMS_BPS 4800

struct zsms_softc {		/* driver status information */
	struct	device zsms_dev;	/* required first: base device */
	struct	zs_chanstate *zsms_cs;

	/* Flags to communicate with zsms_softintr() */
	volatile int zsms_intr_flags;
#define	INTR_RX_OVERRUN 1
#define INTR_TX_EMPTY   2
#define INTR_ST_CHECK   4

	/*
	 * The receive ring buffer.
	 */
	u_int	zsms_rbget;	/* ring buffer `get' index */
	volatile u_int	zsms_rbput;	/* ring buffer `put' index */
	u_short	zsms_rbuf[ZSMS_RX_RING_SIZE]; /* rr1, data pairs */

	int sc_enabled;		/* input enabled? */
	int self_test;

	int inputstate;
	u_int buttons;
	signed char dx;
	signed char dy;

	struct device *sc_wsmousedev;
};

struct zsops zsops_zsms;

static int  zsms_match __P((struct device *, struct cfdata *, void *));
static void zsms_attach __P((struct device *, struct device *, void *));
static void zsms_input __P((void *, int));

struct cfattach zsms_ca = {
	sizeof(struct zsms_softc), zsms_match, zsms_attach,
};

static int  zsms_enable __P((void *));
static int  zsms_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void zsms_disable __P((void *));

const struct wsmouse_accessops zsms_accessops = {
	zsms_enable,
	zsms_ioctl,
	zsms_disable,
};

static int
zsms_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct zsc_attach_args *args = aux;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[ZSCCF_CHANNEL] == args->channel)
		return 2;

	/* This driver accepts wildcard. */
	if (cf->cf_loc[ZSCCF_CHANNEL] == ZSCCF_CHANNEL_DEFAULT)
		return 1;

	return 0;
}

static void
zsms_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *)parent;
	struct zsms_softc *zsms = (void *)self;
	struct zsc_attach_args *args = aux;
	struct zs_chanstate *cs;
	struct wsmousedev_attach_args a;
	int s;

	cs = zsc->zsc_cs[args->channel];
	cs->cs_private = zsms;
	cs->cs_ops = &zsops_zsms;
	zsms->zsms_cs = cs;

	printf("\n");

	/* Initialize the speed, etc. */
	s = splzs();
	/* May need reset... */
	zs_write_reg(cs, 9, ZSWR9_A_RESET);
	/* These are OK as set by zscc: WR3, WR5 */
	/* We don't care about status or tx interrupts. */
	cs->cs_preg[1] = ZSWR1_RIE;
	(void) zs_set_speed(cs, ZSMS_BPS);

	/* mouse wants odd parity */
	cs->cs_preg[4] |= ZSWR4_PARENB;
	/* cs->cs_preg[4] &= ~ZSWR4_EVENP; (no-op) */

	zs_loadchannelregs(cs);
	splx(s);

	a.accessops = &zsms_accessops;
	a.accesscookie = zsms;

	/* XXX correct following comment block XXX
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in pmsintr, because if this fails pms_enable() will
	 * never be called, so pmsintr() will never be called.
	 */
	zsms->sc_enabled = 0;
	zsms->self_test = 0;
	zsms->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

static int
zsms_enable(v)
	void *v;
{
	struct zsms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	/* XXX mice presence test should be done in match/attach context XXX */
	sc->self_test = 1;
	zs_write_data(sc->zsms_cs, MOUSE_SELF_TEST);
	DELAY(100000);
	if (sc->self_test < 0) {
		sc->self_test = 0;
		return EBUSY;
	} else if (sc->self_test == 5) {
		sc->self_test = 0;
		sc->sc_enabled = 1;
	}
	sc->inputstate = 0;

	zs_write_data(sc->zsms_cs, MOUSE_INCREMENTAL);

	return 0;
}

static void
zsms_disable(v)
	void *v;
{
	struct zsms_softc *sc = v;

	sc->sc_enabled = 0;
}

static int
zsms_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = WSMOUSE_TYPE_VSXXX;
		return 0;
	}
	return -1;
}

static void
zsms_input(vsc, data)
	void *vsc;
	int data;
{
	struct zsms_softc *sc = vsc;

	/* XXX mice presence test should be done in match/attach context XXX */
	if (!sc->sc_enabled) {
		if (sc->self_test > 0) {
			if (data < 0) {
				printf("Timeout on 1st byte of mouse self-test report\n");
				sc->self_test = -1;
			} else {
				sc->self_test++;
			}
		}
		if (sc->self_test == 3) {
			if ((data & 0x0f) != 0x2) {
				printf("We don't have a mouse!!!\n");
				sc->self_test = -1;
			}
		}
		/* Interrupts are not expected.  Discard the byte. */
		return;
	}

#define WSMS_BUTTON1    0x01
#define WSMS_BUTTON2    0x02
#define WSMS_BUTTON3    0x04

	if ((data & MOUSE_START_FRAME) != 0)
		sc->inputstate = 1;
	else
		sc->inputstate++;

	if (sc->inputstate == 1) {
		sc->buttons = 0;
		if ((data & LEFT_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON1;
		if ((data & MIDDLE_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON2;
		if ((data & RIGHT_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON3;

		sc->dx = data & MOUSE_X_SIGN;
		sc->dy = data & MOUSE_Y_SIGN;
	} else if (sc->inputstate == 2) {
		if (sc->dx == 0)
			sc->dx = -data;
		else
			sc->dx = data;
	} else if (sc->inputstate == 3) {
		sc->inputstate = 0;
		if (sc->dy == 0)
			sc->dy = -data;
		else
			sc->dy = data;
		wsmouse_input(sc->sc_wsmousedev, sc->buttons,
		    sc->dx, sc->dy, 0, WSMOUSE_INPUT_DELTA);
	}

	return;
}

/****************************************************************
 * Interface to the lower layer (zscc)
 ****************************************************************/

static void zsms_rxint __P((struct zs_chanstate *));
static void zsms_stint __P((struct zs_chanstate *, int));
static void zsms_txint __P((struct zs_chanstate *));
static void zsms_softint __P((struct zs_chanstate *));

static void
zsms_rxint(cs)
	struct zs_chanstate *cs;
{
	struct zsms_softc *zsms;
	int put, put_next;
	u_char c, rr1;

	zsms = cs->cs_private;
	put = zsms->zsms_rbput;

	/*
	 * First read the status, because reading the received char
	 * destroys the status of this char.
	 */
	rr1 = zs_read_reg(cs, 1);
	c = zs_read_data(cs);
	if (rr1 & (ZSRR1_FE | ZSRR1_DO | ZSRR1_PE)) {
		/* Clear the receive error. */
		zs_write_csr(cs, ZSWR0_RESET_ERRORS);
	}

	zsms->zsms_rbuf[put] = (c << 8) | rr1;
	put_next = (put + 1) & ZSMS_RX_RING_MASK;

	/* Would overrun if increment makes (put==get). */
	if (put_next == zsms->zsms_rbget) {
		zsms->zsms_intr_flags |= INTR_RX_OVERRUN;
	} else {
		/* OK, really increment. */
		put = put_next;
	}

	/* Done reading. */
	zsms->zsms_rbput = put;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zsms_txint(cs)
	struct zs_chanstate *cs;
{
	struct zsms_softc *zsms;

	zsms = cs->cs_private;
	zs_write_csr(cs, ZSWR0_RESET_TXINT);
	zsms->zsms_intr_flags |= INTR_TX_EMPTY;
	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zsms_stint(cs, force)
	struct zs_chanstate *cs;
	int force;
{
	struct zsms_softc *zsms;
	int rr0;

	zsms = cs->cs_private;

	rr0 = zs_read_csr(cs);
	zs_write_csr(cs, ZSWR0_RESET_STATUS);

	/*
	 * We have to accumulate status line changes here.
	 * Otherwise, if we get multiple status interrupts
	 * before the softint runs, we could fail to notice
	 * some status line changes in the softint routine.
	 * Fix from Bill Studenmund, October 1996.
	 */
	cs->cs_rr0_delta |= (cs->cs_rr0 ^ rr0);
	cs->cs_rr0 = rr0;
	zsms->zsms_intr_flags |= INTR_ST_CHECK;

	/* Ask for softint() call. */
	cs->cs_softreq = 1;
}


static void
zsms_softint(cs)
	struct zs_chanstate *cs;
{
	struct zsms_softc *zsms;
	int get, c, s;
	int intr_flags;
	u_short ring_data;

	zsms = cs->cs_private;

	/* Atomically get and clear flags. */
	s = splzs();
	intr_flags = zsms->zsms_intr_flags;
	zsms->zsms_intr_flags = 0;

	/* Now lower to spltty for the rest. */
	(void) spltty();

	/*
	 * Copy data from the receive ring to the event layer.
	 */
	get = zsms->zsms_rbget;
	while (get != zsms->zsms_rbput) {
		ring_data = zsms->zsms_rbuf[get];
		get = (get + 1) & ZSMS_RX_RING_MASK;

		/* low byte of ring_data is rr1 */
		c = (ring_data >> 8) & 0xff;

		if (ring_data & ZSRR1_DO)
			intr_flags |= INTR_RX_OVERRUN;
		if (ring_data & (ZSRR1_FE | ZSRR1_PE)) {
			log(LOG_ERR, "%s: input error (0x%x)\n",
				zsms->zsms_dev.dv_xname, ring_data);
			c = -1;	/* signal input error */
		}

		/* Pass this up to the "middle" layer. */
		zsms_input(zsms, c);
	}
	if (intr_flags & INTR_RX_OVERRUN) {
		log(LOG_ERR, "%s: input overrun\n",
		    zsms->zsms_dev.dv_xname);
	}
	zsms->zsms_rbget = get;

	if (intr_flags & INTR_TX_EMPTY) {
		/*
		 * Transmit done.  (Not expected.)
		 */
		log(LOG_ERR, "%s: transmit interrupt?\n",
		    zsms->zsms_dev.dv_xname);
	}

	if (intr_flags & INTR_ST_CHECK) {
		/*
		 * Status line change.  (Not expected.)
		 */
		log(LOG_ERR, "%s: status interrupt?\n",
		    zsms->zsms_dev.dv_xname);
		cs->cs_rr0_delta = 0;
	}

	splx(s);
}

struct zsops zsops_zsms = {
	zsms_rxint,	/* receive char available */
	zsms_stint,	/* external/status */
	zsms_txint,	/* xmit buffer empty */
	zsms_softint,	/* process software interrupt */
};
