/*	$NetBSD: clock_mc.c,v 1.5 2000/01/23 21:01:50 soda Exp $	*/
/*	$OpenBSD: clock_mc.c,v 1.7 1997/04/19 17:19:40 pefo Exp $	*/
/*	NetBSD: clock_mc.c,v 1.2 1995/06/28 04:30:30 cgd Exp 	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>

#include <arc/arc/clockvar.h>
#include <arc/arc/arctype.h>
#include <arc/pica/pica.h>
#include <arc/algor/algor.h>
#include <machine/isa_machdep.h>
#include <arc/isa/timerreg.h>

extern u_int	cputype;
extern int	cpu_int_mask;

void		mcclock_attach __P((struct device *parent,
		    struct device *self, void *aux));
static void	mcclock_init_pica __P((struct clock_softc *csc));
static void	mcclock_init_tyne __P((struct clock_softc *csc));
static void	mcclock_init_p4032 __P((struct clock_softc *csc));
static void	mcclock_get __P((struct clock_softc *csc, time_t base,
		    struct tod_time *ct));
static void	mcclock_set __P((struct clock_softc *csc,
		    struct tod_time *ct));

struct mcclockdata {
	void	(*mc_write) __P((struct clock_softc *csc, u_int reg,
		    u_int datum));
	u_int	(*mc_read) __P((struct clock_softc *csc, u_int reg));
};

#define	mc146818_write(sc, reg, datum)					\
	    (*((struct mcclockdata *)sc->sc_data)->mc_write)(sc, reg, datum)
#define	mc146818_read(sc, reg)						\
	    (*((struct mcclockdata *)sc->sc_data)->mc_read)(sc, reg)

/* Acer Pica clock access code */
static void	mc_write_pica __P((struct clock_softc *csc, u_int reg,
		    u_int datum));
static u_int	mc_read_pica __P((struct clock_softc *csc, u_int reg));
static struct mcclockdata mcclockdata_pica = { mc_write_pica, mc_read_pica };

/* Deskstation clock access code */
static void	mc_write_tyne __P((struct clock_softc *csc, u_int reg,
		    u_int datum));
static u_int	mc_read_tyne __P((struct clock_softc *csc, u_int reg));
static struct mcclockdata mcclockdata_tyne = { mc_write_tyne, mc_read_tyne };

/* Algorithmics P4032 clock access code */
static void	mc_write_p4032 __P((struct clock_softc *csc, u_int reg,
		    u_int datum));
static u_int	mc_read_p4032 __P((struct clock_softc *csc, u_int reg));
static struct mcclockdata mcclockdata_p4032 = { mc_write_p4032, mc_read_p4032 };

void
mcclock_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct clock_softc *csc = (struct clock_softc *)self;

	printf(": mc146818 or compatible");

	csc->sc_get = mcclock_get;
	csc->sc_set = mcclock_set;

        switch (cputype) {

        case ACER_PICA_61:
	case MAGNUM:
		csc->sc_init = mcclock_init_pica;
		csc->sc_data = &mcclockdata_pica;
		mc146818_write(csc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);
		break;

	case DESKSTATION_RPC44:
	case DESKSTATION_TYNE:
		csc->sc_init = mcclock_init_tyne;
		csc->sc_data = &mcclockdata_tyne;
		mc146818_write(csc, MC_REGB, MC_REGB_BINARY | MC_REGB_24HR);
		break;

	case ALGOR_P4032:
		csc->sc_init = mcclock_init_p4032;
		csc->sc_data = &mcclockdata_p4032;
		mc146818_write(csc, MC_REGB, MC_REGB_BINARY|MC_REGB_24HR|MC_REGB_SQWE);
		break;

	default:
		printf("\n");
		panic("don't know how to set up for other system types.");
	}
}

static void
mcclock_init_pica(csc)
	struct clock_softc *csc;
{
/* XXX Does not really belong here but for the moment we don't care */
	out32(R4030_SYS_IT_VALUE, 9); /* 10ms - 1 */
	/* Enable periodic clock interrupt */
	out32(R4030_SYS_EXT_IMASK, cpu_int_mask);
}

static void
mcclock_init_tyne(csc)
	struct clock_softc *csc;
{
	isa_outb(IO_TIMER1 + TIMER_MODE,
	    TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
	isa_outb(IO_TIMER1 + TIMER_CNTR0, TIMER_DIV(hz) % 256);
	isa_outb(IO_TIMER1 + TIMER_CNTR0, TIMER_DIV(hz) / 256);
}

static void
mcclock_init_p4032(csc)
	struct clock_softc *csc;
{
	int s;
	char cv;

	hz = 256;	/* NOTE! We are going at 256 Hz! */
	s = splclock();
	cv = mc146818_read(csc, MC_REGA) & ~MC_REGA_RSMASK;
	mc146818_write(csc, MC_REGA, cv | MC_RATE_256_Hz);
	cv = mc146818_read(csc, MC_REGB);
	mc146818_write(csc, MC_REGB, cv | MC_REGB_PIE);
	splx(s);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static void
mcclock_get(csc, base, ct)
	struct clock_softc *csc;
	time_t base;
	struct tod_time *ct;
{
	mc_todregs regs;
	int s;

	s = splclock();
	MC146818_GETTOD(csc, &regs)
	splx(s);

	ct->sec = regs[MC_SEC];
	ct->min = regs[MC_MIN];
	ct->hour = regs[MC_HOUR];
	ct->dow = regs[MC_DOW];
	ct->day = regs[MC_DOM];
	ct->mon = regs[MC_MONTH];
	ct->year = regs[MC_YEAR];
	if(cputype == ALGOR_P4032)
		ct->year -= 80;
}

/*
 * Reset the TODR based on the time value.
 */
static void
mcclock_set(csc, ct)
	struct clock_softc *csc;
	struct tod_time *ct;
{
	mc_todregs regs;
	int s;

	s = splclock();
	MC146818_GETTOD(csc, &regs);
printf("%d-%d-%d, %d:%d:%d\n", regs[MC_YEAR], regs[MC_MONTH], regs[MC_DOM], regs[MC_HOUR], regs[MC_MIN], regs[MC_SEC]);

	regs[MC_SEC] = ct->sec;
	regs[MC_MIN] = ct->min;
	regs[MC_HOUR] = ct->hour;
	regs[MC_DOW] = ct->dow;
	regs[MC_DOM] = ct->day;
	regs[MC_MONTH] = ct->mon;
	if(cputype == ALGOR_P4032)
		regs[MC_YEAR] = ct->year + 80;
	else
		regs[MC_YEAR] = ct->year;

	MC146818_PUTTOD(csc, &regs);
	MC146818_GETTOD(csc, &regs);
printf("%d-%d-%d, %d:%d:%d\n", regs[MC_YEAR], regs[MC_MONTH], regs[MC_DOM], regs[MC_HOUR], regs[MC_MIN], regs[MC_SEC]);
	splx(s);
}

static void
mc_write_pica(csc, reg, datum)
	struct clock_softc *csc;
	u_int reg, datum;
{
	int as;

	as = in32(PICA_SYS_ISA_AS) & 0x80;
	out32(PICA_SYS_ISA_AS, as | reg);
	outb(PICA_SYS_CLOCK, datum);
}

static u_int
mc_read_pica(csc, reg)
	struct clock_softc *csc;
	u_int reg;
{
	int i,as;

	as = in32(PICA_SYS_ISA_AS) & 0x80;
	out32(PICA_SYS_ISA_AS, as | reg);
	i = inb(PICA_SYS_CLOCK);
	return(i);
}

static void
mc_write_tyne(csc, reg, datum)
	struct clock_softc *csc;
	u_int reg, datum;
{
	isa_outb(IO_RTC, reg);
	isa_outb(IO_RTC+1, datum);
}

static u_int
mc_read_tyne(csc, reg)
	struct clock_softc *csc;
	u_int reg;
{
	int i;

	isa_outb(IO_RTC, reg);
	i = isa_inb(IO_RTC+1);
	return(i);
}

static void
mc_write_p4032(csc, reg, datum)
	struct clock_softc *csc;
	u_int reg, datum;
{
	outb(P4032_CLOCK, reg);
	outb(P4032_CLOCK+4, datum);
}

static u_int
mc_read_p4032(csc, reg)
	struct clock_softc *csc;
	u_int reg;
{
	int i;

	outb(P4032_CLOCK, reg);
	i = inb(P4032_CLOCK+4) & 0xff;
	return(i);
}
