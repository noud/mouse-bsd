/*	$NetBSD: zs.c,v 1.13 1999/10/11 01:57:43 eeh Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Zilog Z8530 Dual UART driver (machine-dependent part)
 *
 * Runs two serial lines per chip using slave drivers.
 * Plain tty/async lines use the zs_async slave.
 * Sun keyboard/mouse uses the zs_kbd/zs_ms slaves.
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>

#include <machine/autoconf.h>
#include <machine/openfirm.h>
#include <machine/bsd_openprom.h>
#include <machine/conf.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/z8530var.h>

#include <dev/cons.h>
#include <dev/ic/z8530reg.h>

#include <sparc64/sparc64/vaddrs.h>
#include <sparc64/sparc64/auxreg.h>
#include <sparc64/dev/cons.h>

#include "kbd.h"	/* NKBD */
#include "zs.h" 	/* NZS */

/* Make life easier for the initialized arrays here. */
#if NZS < 3
#undef  NZS
#define NZS 3
#endif

/*
 * Some warts needed by z8530tty.c -
 * The default parity REALLY needs to be the same as the PROM uses,
 * or you can not see messages done with printf during boot-up...
 */
int zs_def_cflag = (CREAD | CS8 | HUPCL);
int zs_major = 12;

/*
 * The Sun provides a 4.9152 MHz clock to the ZS chips.
 */
#define PCLK	(9600 * 512)	/* PCLK pin input clock rate */

/*
 * Select software interrupt bit based on TTY ipl.
 */
#if PIL_TTY == 1
# define IE_ZSSOFT IE_L1
#elif PIL_TTY == 4
# define IE_ZSSOFT IE_L4
#elif PIL_TTY == 6
# define IE_ZSSOFT IE_L6
#else
# error "no suitable software interrupt bit"
#endif

#define	ZS_DELAY()

/* The layout of this is hardware-dependent (padding, order). */
struct zschan {
	volatile u_char	zc_csr;		/* ctrl,status, and indirect access */
	u_char		zc_xxx0;
	volatile u_char	zc_data;	/* data */
	u_char		zc_xxx1;
};
struct zsdevice {
	/* Yes, they are backwards. */
	struct	zschan zs_chan_b;
	struct	zschan zs_chan_a;
};

/* Saved PROM mappings */
static struct zsdevice *zsaddr[NZS];

/* Flags from cninit() */
static int zs_hwflags[NZS][2];

/* Default speed for each channel */
static int zs_defspeed[NZS][2] = {
	{ 9600, 	/* ttya */
	  9600 },	/* ttyb */
	{ 1200, 	/* keyboard */
	  1200 },	/* mouse */
	{ 9600, 	/* ttyc */
	  9600 },	/* ttyd */
};

static u_char zs_init_reg[16] = {
	0,	/* 0: CMD (reset, etc.) */
	0,	/* 1: No interrupts yet. */
	0,	/* 2: IVECT */
	ZSWR3_RX_8 | ZSWR3_RX_ENABLE,
	ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP,
	ZSWR5_TX_8 | ZSWR5_TX_ENABLE,
	0,	/* 6: TXSYNC/SYNCLO */
	0,	/* 7: RXSYNC/SYNCHI */
	0,	/* 8: alias for data port */
	ZSWR9_MASTER_IE | ZSWR9_NO_VECTOR,
	0,	/*10: Misc. TX/RX control bits */
	ZSWR11_TXCLK_BAUD | ZSWR11_RXCLK_BAUD,
	((PCLK/32)/9600)-2,	/*12: BAUDLO (default=9600) */
	0,			/*13: BAUDHI (default=9600) */
	ZSWR14_BAUD_ENA | ZSWR14_BAUD_FROM_PCLK,
	ZSWR15_BREAK_IE,
};

struct zschan *
zs_get_chan_addr(zs_unit, channel)
	int zs_unit, channel;
{
	struct zsdevice	*addr;
	struct zschan	*zc;

	if (zs_unit >= NZS)
		return (NULL);
	addr = zsaddr[zs_unit];
#ifdef DEBUG
	if (addr == NULL) {
		db_printf("zs_get_chan_addr(): unit %d channel %d not found\n", zs_unit, channel);
		Debugger();
	}
#endif
	if (addr == NULL)
		return (NULL);
	if (channel == 0) {
		zc = &addr->zs_chan_a;
	} else {
		zc = &addr->zs_chan_b;
	}
	return (zc);
}


/****************************************************************
 * Autoconfig
 ****************************************************************/

/* Definition of the driver for autoconfig. */
static int  zs_match_sbus __P((struct device *, struct cfdata *, void *));
static int  zs_match_mainbus __P((struct device *, struct cfdata *, void *));
static int  zs_match_obio __P((struct device *, struct cfdata *, void *));
static void zs_attach_sbus __P((struct device *, struct device *, void *));
static void zs_attach_mainbus __P((struct device *, struct device *, void *));
static void zs_attach_obio __P((struct device *, struct device *, void *));

static void zs_attach __P((struct zsc_softc *, int));
static int  zs_print __P((void *, const char *name));

struct cfattach zs_ca = {
	sizeof(struct zsc_softc), zs_match_sbus, zs_attach_sbus
};

struct cfattach zs_mainbus_ca = {
	sizeof(struct zsc_softc), zs_match_mainbus, zs_attach_mainbus
};

struct cfattach zs_obio_ca = {
	sizeof(struct zsc_softc), zs_match_obio, zs_attach_obio
};

extern struct cfdriver zs_cd;
extern struct consdev consdev_kd;
extern struct consdev consdev_zs;
extern struct consdev *cn_hw;
extern int stdinnode;
extern int fbnode;

/* Interrupt handlers. */
static int zshard __P((void *));
static int zssoft __P((void *));
static struct intrhand levelsoft = { zssoft };

static int zs_get_speed __P((struct zs_chanstate *));


/*
 * Is the zs chip present?
 */
static int
zs_match_mainbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(cf->cf_driver->cd_name, ma->ma_name) != 0)
		return (0);

	return (getpropint(ma->ma_node, "slave", -2) == cf->cf_unit);
}

static int
zs_match_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	if (strcmp(cf->cf_driver->cd_name, sa->sa_name) != 0)
		return (0);

	return 1;
}

static int
zs_match_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#ifdef SUN4U
	return 0;
#else
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;

		if (strcmp(cf->cf_driver->cd_name, sa->sa_name) != 0)
			return (0);

		return (getpropint(sa->sa_node, "slave", -2) == cf->cf_unit);
	}

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0, oba->oba_paddr,
			        1, 0, 0, NULL, NULL));
#endif
}

static void
zs_attach_mainbus(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
#ifdef SUN4U
	return;
#else
	struct zsc_softc *zsc = (void *) self;
	struct mainbus_attach_args *ma = aux;
	int zs_unit = zsc->zsc_dev.dv_unit;

	zsc->zsc_bustag = ma->ma_bustag;
	zsc->zsc_dmatag = ma->ma_dmatag;

	/* Use the mapping setup by the Sun PROM. */
	if (zsaddr[zs_unit] == NULL)
		zsaddr[zs_unit] = findzs(zs_unit);
	if ((void*)zsaddr[zs_unit] != (void*)(u_long)ma->ma_address[0])
		panic("zsattach_mainbus");
	zs_attach(zsc, ma->ma_pri);
#endif
}


static void
zs_attach_sbus(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct zsc_softc *zsc = (void *) self;
	struct sbus_attach_args *sa = aux;
	int zs_unit = zsc->zsc_dev.dv_unit;
	struct consdev *cn = NULL;

	zsc->zsc_bustag = sa->sa_bustag;
	zsc->zsc_dmatag = sa->sa_dmatag;

	/* Use the mapping setup by the Sun PROM. */
	if (zsaddr[zs_unit] == NULL) {
		if (sa->sa_npromvaddrs) {
			/*
			 * We're converting from a 32-bit pointer to a 64-bit
			 * pointer.  Since the 32-bit entity is negative, but
			 * the kernel is still mapped into the lower 4GB
			 * range, this needs to be zero-extended.
			 *
			 * XXXXX If we map the kernel and devices into the
			 * high 4GB range, this needs to be changed to
			 * sign-extend the address.
			 */
			zsaddr[zs_unit] =
				(struct zsdevice *)
				(unsigned long)sa->sa_promvaddrs[0];
		} else {
			bus_space_handle_t kvaddr;

			if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
					 sa->sa_offset,
					 sa->sa_size,
					 BUS_SPACE_MAP_LINEAR,
					 0, &kvaddr) != 0) {
				printf("%s @ sbus: cannot map registers\n",
				       self->dv_xname);
				return;
			}
			zsaddr[zs_unit] = (struct zsdevice *)
				(long)kvaddr;
		}
	}
	/*
	 * Check to see if we're the console.  We presume the input comes from
	 * the same location as the output, although that may not be true.
	 * To support input from the serial line but output to a display we
	 * would need to generate some really weird consdev vectors.
	 */
	if (sa->sa_node == stdinnode) {
		char buf[256];
		int chan = 0;
		int len;

		if ((len = OF_instance_to_path(sa->sa_node, buf, sizeof(buf))) > 0) {
			/* With zs nodes, the last :a or :b selects the channel */
			if (buf[len] == 0) len--;
			if (buf[len] == 'b') chan = 1;
			/* But keyboards don't have a :a or :b */
		}
		zs_hwflags[zs_unit][chan] = ZS_HWFLAG_CONSOLE;
		zs_conschan = zs_get_chan_addr(zs_unit, chan);
		if (OF_getproplen(sa->sa_node, "keyboard") >= 0) {
			cn_hw = &consdev_zs;
			cn = &consdev_kd;
		} else {
			cn = &consdev_zs;
		}
	}
	zs_attach(zsc, sa->sa_pri);
	if (cn) {
		cn_tab = cn;
		(*cn->cn_init)(cn);
#ifdef	KGDB
		zs_kgdb_init();
#endif
	}
}

static void
zs_attach_obio(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
#ifndef SUN4U
	struct zsc_softc *zsc = (void *) self;
	union obio_attach_args *uoba = aux;
	int zs_unit = zsc->zsc_dev.dv_unit;

	/* Use the mapping setup by the Sun PROM. */
	if (zsaddr[zs_unit] == NULL)
		zsaddr[zs_unit] = findzs(zs_unit);

	if (uoba->uoba_isobio4 == 0) {
		struct sbus_attach_args *sa = &uoba->uoba_sbus;
		zsc->zsc_bustag = sa->sa_bustag;
		zsc->zsc_dmatag = sa->sa_dmatag;
		zs_attach(zsc, sa->sa_pri);
	} else {
		struct obio4_attach_args *oba = &uoba->uoba_oba4;
		zsc->zsc_bustag = oba->oba_bustag;
		zsc->zsc_dmatag = oba->oba_dmatag;
		zs_attach(zsc, oba->oba_pri);
	}
#endif
}
/*
 * Attach a found zs.
 *
 * USE ROM PROPERTIES port-a-ignore-cd AND port-b-ignore-cd FOR
 * SOFT CARRIER, AND keyboard PROPERTY FOR KEYBOARD/MOUSE?
 */
static void
zs_attach(zsc, pri)
	struct zsc_softc *zsc;
	int pri;
{
	struct zsc_attach_args zsc_args;
	volatile struct zschan *zc;
	struct zs_chanstate *cs;
	int s, zs_unit, channel;
	static int didintr, prevpri;

	printf(" softpri %d\n", PIL_TTY);

	/*
	 * Initialize software state for each channel.
	 */
	zs_unit = zsc->zsc_dev.dv_unit;
	for (channel = 0; channel < 2; channel++) {
		zsc_args.channel = channel;
		zsc_args.hwflags = zs_hwflags[zs_unit][channel];
		cs = &zsc->zsc_cs_store[channel];
		zsc->zsc_cs[channel] = cs;

		cs->cs_channel = channel;
		cs->cs_private = NULL;
		cs->cs_ops = &zsops_null;
		cs->cs_brg_clk = PCLK / 16;

		zc = zs_get_chan_addr(zs_unit, channel);
		if (zs_hwflags[zs_unit][channel] == ZS_HWFLAG_CONSOLE) {
			zs_conschan = (struct zschan *)zc;
		}
		cs->cs_reg_csr  = &zc->zc_csr;
		cs->cs_reg_data = &zc->zc_data;

		bcopy(zs_init_reg, cs->cs_creg, 16);
		bcopy(zs_init_reg, cs->cs_preg, 16);

		/* XXX: Get these from the PROM properties! */
		/* XXX: See the mvme167 code.  Better. */
		if (zsc_args.hwflags & ZS_HWFLAG_CONSOLE)
			cs->cs_defspeed = zs_get_speed(cs);
		else
			cs->cs_defspeed = zs_defspeed[zs_unit][channel];
		cs->cs_defcflag = zs_def_cflag;

		/* Make these correspond to cs_defcflag (-crtscts) */
		cs->cs_rr0_dcd = ZSRR0_DCD;
		cs->cs_rr0_cts = 0;
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;

		/*
		 * Clear the master interrupt enable.
		 * The INTENA is common to both channels,
		 * so just do it on the A channel.
		 */
		if (channel == 0) {
			zs_write_reg(cs, 9, 0);
		}

		/*
		 * Look for a child driver for this channel.
		 * The child attach will setup the hardware.
		 */
		if (!config_found(&zsc->zsc_dev, (void *)&zsc_args, zs_print)) {
			/* No sub-driver.  Just reset it. */
			u_char reset = (channel == 0) ?
				ZSWR9_A_RESET : ZSWR9_B_RESET;
			s = splzs();
			zs_write_reg(cs,  9, reset);
			splx(s);
		}
	}

	/*
	 * Now safe to install interrupt handlers.  Note the arguments
	 * to the interrupt handlers aren't used.  Note, we only do this
	 * once since both SCCs interrupt at the same level and vector.
	 */
	if (!didintr) {
		didintr = 1;
		prevpri = pri;
		bus_intr_establish(zsc->zsc_bustag, pri, 0, zshard, NULL);
		intr_establish(PIL_TTY, &levelsoft);
	} else if (pri != prevpri)
		panic("broken zs interrupt scheme");

	evcnt_attach(&zsc->zsc_dev, "intr", &zsc->zsc_intrcnt);

	/*
	 * Set the master interrupt enable and interrupt vector.
	 * (common to both channels, do it on A)
	 */
	cs = zsc->zsc_cs[0];
	s = splhigh();
	/* interrupt vector */
	zs_write_reg(cs, 2, zs_init_reg[2]);
	/* master interrupt control (enable) */
	zs_write_reg(cs, 9, zs_init_reg[9]);
	splx(s);

#if 0
	/*
	 * XXX: L1A hack - We would like to be able to break into
	 * the debugger during the rest of autoconfiguration, so
	 * lower interrupts just enough to let zs interrupts in.
	 * This is done after both zs devices are attached.
	 */
	if (zs_unit == 1) {
		printf("zs1: enabling zs interrupts\n");
		(void)splfd(); /* XXX: splzs - 1 */
	}
#endif
}

static int
zs_print(aux, name)
	void *aux;
	const char *name;
{
	struct zsc_attach_args *args = aux;

	if (name != NULL)
		printf("%s: ", name);

	if (args->channel != -1)
		printf(" channel %d", args->channel);

	return (UNCONF);
}

static volatile int zssoftpending;

/*
 * Our ZS chips all share a common, autovectored interrupt,
 * so we have to look at all of them on each interrupt.
 */
static int
zshard(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int unit, rr3, rval, softreq;

	rval = softreq = 0;
	for (unit = 0; unit < zs_cd.cd_ndevs; unit++) {
		zsc = zs_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		rr3 = zsc_intr_hard(zsc);
		/* Count up the interrupts. */
		if (rr3) {
			rval |= rr3;
			zsc->zsc_intrcnt.ev_count++;
		}
		softreq |= zsc->zsc_cs[0]->cs_softreq;
		softreq |= zsc->zsc_cs[1]->cs_softreq;
	}

	/* We are at splzs here, so no need to lock. */
	if (softreq && (zssoftpending == 0)) {
		zssoftpending = IE_ZSSOFT;
#if defined(SUN4M)
		if (CPU_ISSUN4M)
			raise(0, PIL_TTY);
		else
#endif
			ienab_bis(IE_ZSSOFT);
	}
	return (rval);
}

/*
 * Similar scheme as for zshard (look at all of them)
 */
static int
zssoft(arg)
	void *arg;
{
	register struct zsc_softc *zsc;
	register int s, unit;

	/* This is not the only ISR on this IPL. */
	if (zssoftpending == 0)
		return (0);

	/*
	 * The soft intr. bit will be set by zshard only if
	 * the variable zssoftpending is zero.  The order of
	 * these next two statements prevents our clearing
	 * the soft intr bit just after zshard has set it.
	 */
	/* ienab_bic(IE_ZSSOFT); */
	zssoftpending = 0;

	/* Make sure we call the tty layer at spltty. */
	s = spltty();
	for (unit = 0; unit < zs_cd.cd_ndevs; unit++) {
		zsc = zs_cd.cd_devs[unit];
		if (zsc == NULL)
			continue;
		(void)zsc_intr_soft(zsc);
#ifdef TTY_DEBUG
		{
			struct zstty_softc *zst0 = zsc->zsc_cs[0]->cs_private;
			struct zstty_softc *zst1 = zsc->zsc_cs[1]->cs_private;
			if (zst0->zst_overflows || zst1->zst_overflows ) {
				struct trapframe *frame = (struct trapframe *)arg;

				printf("zs silo overflow from %p\n",
				       (long)frame->tf_pc);
			}
		}
#endif
	}
	splx(s);
	return (1);
}


/*
 * Compute the current baud rate given a ZS channel.
 */
static int
zs_get_speed(cs)
	struct zs_chanstate *cs;
{
	int tconst;

	tconst = zs_read_reg(cs, 12);
	tconst |= zs_read_reg(cs, 13) << 8;
	return (TCONST_TO_BPS(cs->cs_brg_clk, tconst));
}

/*
 * MD functions for setting the baud rate and control modes.
 */
int
zs_set_speed(cs, bps)
	struct zs_chanstate *cs;
	int bps;	/* bits per second */
{
	int tconst, real_bps;

	if (bps == 0)
		return (0);

#ifdef	DIAGNOSTIC
	if (cs->cs_brg_clk == 0)
		panic("zs_set_speed");
#endif

	tconst = BPS_TO_TCONST(cs->cs_brg_clk, bps);
	if (tconst < 0)
		return (EINVAL);

	/* Convert back to make sure we can do it. */
	real_bps = TCONST_TO_BPS(cs->cs_brg_clk, tconst);

	/* XXX - Allow some tolerance here? */
	if (real_bps != bps)
		return (EINVAL);

	cs->cs_preg[12] = tconst;
	cs->cs_preg[13] = tconst >> 8;

	/* Caller will stuff the pending registers. */
	return (0);
}

int
zs_set_modes(cs, cflag)
	struct zs_chanstate *cs;
	int cflag;	/* bits per second */
{
	int s;

	/*
	 * Output hardware flow control on the chip is horrendous:
	 * if carrier detect drops, the receiver is disabled, and if
	 * CTS drops, the transmitter is stoped IN MID CHARACTER!
	 * Therefore, NEVER set the HFC bit, and instead use the
	 * status interrupt to detect CTS changes.
	 */
	s = splzs();
	cs->cs_rr0_pps = 0;
	if ((cflag & (CLOCAL | MDMBUF)) != 0) {
		cs->cs_rr0_dcd = 0;
		if ((cflag & MDMBUF) == 0)
			cs->cs_rr0_pps = ZSRR0_DCD;
	} else
		cs->cs_rr0_dcd = ZSRR0_DCD;
	if ((cflag & CRTSCTS) != 0) {
		cs->cs_wr5_dtr = ZSWR5_DTR;
		cs->cs_wr5_rts = ZSWR5_RTS;
		cs->cs_rr0_cts = ZSRR0_CTS;
	} else if ((cflag & CDTRCTS) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
		cs->cs_rr0_cts = ZSRR0_CTS;
	} else if ((cflag & MDMBUF) != 0) {
		cs->cs_wr5_dtr = 0;
		cs->cs_wr5_rts = ZSWR5_DTR;
		cs->cs_rr0_cts = ZSRR0_DCD;
	} else {
		cs->cs_wr5_dtr = ZSWR5_DTR | ZSWR5_RTS;
		cs->cs_wr5_rts = 0;
		cs->cs_rr0_cts = 0;
	}
	splx(s);

	/* Caller will stuff the pending registers. */
	return (0);
}


/*
 * Read or write the chip with suitable delays.
 */

u_char
zs_read_reg(cs, reg)
	struct zs_chanstate *cs;
	u_char reg;
{
	u_char val;

	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void
zs_write_reg(cs, reg, val)
	struct zs_chanstate *cs;
	u_char reg, val;
{
	*cs->cs_reg_csr = reg;
	ZS_DELAY();
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char
zs_read_csr(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = *cs->cs_reg_csr;
	ZS_DELAY();
	return (val);
}

void  zs_write_csr(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_csr = val;
	ZS_DELAY();
}

u_char zs_read_data(cs)
	struct zs_chanstate *cs;
{
	register u_char val;

	val = *cs->cs_reg_data;
	ZS_DELAY();
	return (val);
}

void  zs_write_data(cs, val)
	struct zs_chanstate *cs;
	u_char val;
{
	*cs->cs_reg_data = val;
	ZS_DELAY();
}

/****************************************************************
 * Console support functions (Sun specific!)
 * Note: this code is allowed to know about the layout of
 * the chip registers, and uses that to keep things simple.
 * XXX - I think I like the mvme167 code better. -gwr
 ****************************************************************/

extern void Debugger __P((void));
void *zs_conschan;

/*
 * Handle user request to enter kernel debugger.
 */
void
zs_abort(cs)
	struct zs_chanstate *cs;
{
	register volatile struct zschan *zc = zs_conschan;
	int rr0;

	/* Wait for end of break to avoid PROM abort. */
	/* XXX - Limit the wait? */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while (rr0 & ZSRR0_BREAK);

#if defined(KGDB)
	zskgdb(cs);
#elif defined(DDB)
	{
		extern int db_active;

		if (!db_active)
			Debugger();
		else
			/* Debugger is probably hozed */
			callrom();
	}
#else
	printf("stopping on keyboard abort\n");
	callrom();
#endif
}

/*
 * Polled input char.
 */
int
zs_getc(arg)
	void *arg;
{
	register volatile struct zschan *zc = arg;
	register int s, c, rr0;

	s = splhigh();
	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();
	splx(s);

	/*
	 * This is used by the kd driver to read scan codes,
	 * so don't translate '\r' ==> '\n' here...
	 */
	return (c);
}

/*
 * Polled output char.
 */
void
zs_putc(arg, c)
	void *arg;
	int c;
{
	register volatile struct zschan *zc = arg;
	register int s, rr0;

	s = splhigh();

	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	/*
	 * Send the next character.
	 * Now you'd think that this could be followed by a ZS_DELAY()
	 * just like all the other chip accesses, but it turns out that
	 * the `transmit-ready' interrupt isn't de-asserted until
	 * some period of time after the register write completes
	 * (more than a couple instructions).  So to avoid stray
	 * interrupts we put in the 2us delay regardless of cpu model.
	 */
	zc->zc_data = c;
	delay(2);

	splx(s);
}

/*****************************************************************/

static void zscninit __P((struct consdev *));
static int  zscngetc __P((dev_t));
static void zscnputc __P((dev_t, int));
static void zscnpollc __P((dev_t, int));
/*
 * Console table shared by ttya, ttyb
 */
struct consdev consdev_zs = {
	nullcnprobe,
	zscninit,
	zscngetc,
	zscnputc,
	zscnpollc,
};

static void
zscninit(cn)
	struct consdev *cn;
{
}

/*
 * Polled console input putchar.
 */
static int
zscngetc(dev)
	dev_t dev;
{
	return (zs_getc(zs_conschan));
}

/*
 * Polled console output putchar.
 */
static void
zscnputc(dev, c)
	dev_t dev;
	int c;
{
	zs_putc(zs_conschan, c);
}

int swallow_zsintrs;

static void
zscnpollc(dev, on)
	dev_t dev;
	int on;
{
	/*
	 * Need to tell zs driver to acknowledge all interrupts or we get
	 * annoying spurious interrupt messages.  This is because mucking
	 * with spl() levels during polling does not prevent interrupts from
	 * being generated.
	 */

	if (on) swallow_zsintrs++;
	else swallow_zsintrs--;
}
