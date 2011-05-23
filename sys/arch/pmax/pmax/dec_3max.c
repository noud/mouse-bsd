/* $NetBSD: dec_3max.c,v 1.24 2000/02/03 04:09:01 nisimura Exp $ */

/*
 * Copyright (c) 1998 Jonathan Stone.  All rights reserved.
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
 *	This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
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
 *	@(#)machdep.c	8.3 (Berkeley) 1/12/94
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dec_3max.c,v 1.24 2000/02/03 04:09:01 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/locore.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kn02.h>
#include <pmax/pmax/memc.h>
#include <pmax/dev/dcvar.h>

#include "rasterconsole.h"

/*
 * forward declarations
 */
void		dec_3max_init __P((void));		/* XXX */
static void	dec_3max_bus_reset __P((void));

static void	dec_3max_cons_init __P((void));
static void	dec_3max_device_register __P((struct device *, void *));
static void	dec_3max_errintr __P((void));
static int	dec_3max_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_3max_intr_establish __P((struct device *, void *,
		    int, int (*)(void *), void *));
static void	dec_3max_intr_disestablish __P((struct device *, void *));


#define	kn02_wbflush()	mips1_wbflush()	/* XXX to be corrected XXX */

void
dec_3max_init()
{
	u_int32_t csr;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3max_bus_reset;
	platform.cons_init = dec_3max_cons_init;
	platform.device_register = dec_3max_device_register;
	platform.iointr = dec_3max_intr;
	platform.intr_establish = dec_3max_intr_establish;
	platform.intr_disestablish = dec_3max_intr_disestablish;
	platform.memsize = memsize_scan;
	/* no high resolution timer available */

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	mips_hardware_intr = dec_3max_intr;

	splvec.splbio = MIPS_SPL0;
	splvec.splnet = MIPS_SPL0;
	splvec.spltty = MIPS_SPL0;
	splvec.splimp = MIPS_SPL0;
	splvec.splclock = MIPS_SPL_0_1;
	splvec.splstatclock = MIPS_SPL_0_1;

	/* calibrate cpu_mhz value */
	mc_cpuspeed(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK), MIPS_INT_MASK_1);

	/*
	 * Enable ECC memory correction, turn off LEDs, and
	 * disable all TURBOchannel interrupts.
	 */
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	csr &= ~(KN02_CSR_WRESERVED|KN02_CSR_IOINTEN|KN02_CSR_CORRECT|0xff);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) = csr;
	kn02_wbflush();

	strcpy(cpu_model, "DECstation 5000/200 (3MAX)");
}

/*
 * Initalize the memory system and I/O buses.
 */
static void
dec_3max_bus_reset()
{
	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(volatile u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();

	*(u_int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN) = 0;
	kn02_wbflush();
}

static void
dec_3max_cons_init()
{
 	int kbd, crt, screen;
 	extern int tcfb_cnattach __P((int));		/* XXX */
 
 	kbd = crt = screen = 0;
 	prom_findcons(&kbd, &crt, &screen);
 
 	if (screen > 0) {
#if NRASTERCONSOLE > 0
 		if (kbd == 7 && tcfb_cnattach(crt) > 0) {
 			dckbd_cnattach(KN02_SYS_DZ);
 			return;
 		}
#else
 		printf("No framebuffer device configured for slot %d: ", crt);
 		printf("using serial console\n");
#endif
 	}
 	/*
 	 * Delay to allow PROM putchars to complete.
 	 * FIFO depth * character time,
 	 * character time = (1000000 / (defaultrate / 10))
 	 */
 	DELAY(160000000 / 9600);	/* XXX */
 
 	dc_cnattach(KN02_SYS_DZ, kbd);
}

static void
dec_3max_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3max_device_register unimplemented");
}


static void
dec_3max_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	int slotno = (int)cookie;
	volatile int *p_csr =
	    (volatile int *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
	int csr;
	int s;

#if 0
	printf("3MAX enable_intr: %sabling slot %d, sc %p\n",
	    (on? "en" : "dis"), slotno, sc);
#endif 
	if (slotno > MAX_INTR_COOKIES)
		panic("dec_3max_intr_establish: bogus slot %d\n", slotno);

	intrtab[slotno].ih_func = handler;
	intrtab[slotno].ih_arg = arg;

	slotno = 1 << (slotno + KN02_CSR_IOINTEN_SHIFT);
	s = splhigh();
	csr = *p_csr & ~(KN02_CSR_WRESERVED | 0xFF);
	*p_csr = csr | slotno;
	splx(s);
}

static void
dec_3max_intr_disestablish(dev, arg)
	struct device *dev;
	void *arg;
{
	printf("dec_3max_intr_distestablish: not implemented\n");
}


/*
 * Handle hardware interrupts for the KN02. (DECstation 5000/200)
 * Returns spl value.
 */
static int
dec_3max_intr(mask, pc, status, cause)
	unsigned mask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int warned = 0;
	unsigned i, m;
	unsigned csr;

	/* handle clock interrupts ASAP */
	if (mask & MIPS_INT_MASK_1) {
		struct clockframe cf;

		csr = *(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		if ((csr & KN02_CSR_PSWARN) && !warned) {
			warned = 1;
			printf("WARNING: power supply is overheating!\n");
		} else if (warned && !(csr & KN02_CSR_PSWARN)) {
			warned = 0;
			printf("WARNING: power supply is OK again\n");
		}

		__asm __volatile("lbu $0,48(%0)" ::
			"r"(MIPS_PHYS_TO_KSEG1(KN02_SYS_CLOCK)));
		cf.pc = pc;
		cf.sr = status;
		hardclock(&cf);
		intrcnt[HARDCLOCK]++;

		/* keep clock interrupts enabled when we return */
		cause &= ~MIPS_INT_MASK_1;
	}

	/* If clock interrups were enabled, re-enable them ASAP. */
	_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_1));

	if (mask & MIPS_INT_MASK_0) {
		static int intr_map[8] = { SLOT0_INTR, SLOT1_INTR, SLOT2_INTR,
					   /* these two bits reserved */
					   STRAY_INTR,  STRAY_INTR,
					   SCSI_INTR, LANCE_INTR,
					   SERIAL0_INTR };

		csr = *(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);
		m = csr & (csr >> KN02_CSR_IOINTEN_SHIFT) & KN02_CSR_IOINT;
#if 0
		*(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) =
			(csr & ~(KN02_CSR_WRESERVED | 0xFF)) |
			(m << KN02_CSR_IOINTEN_SHIFT);
#endif
		for (i = 0; m; i++, m >>= 1) {
			if (!(m & 1))
				continue;
			intrcnt[intr_map[i]]++;
			if (intrtab[i].ih_func)
				(*intrtab[i].ih_func)(intrtab[i].ih_arg);
			else
				printf("spurious interrupt %d\n", i);
		}
#if 0
		*(unsigned *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR) =
			csr & ~(KN02_CSR_WRESERVED | 0xFF);
#endif
	}
	if (mask & MIPS_INT_MASK_3) {
		intrcnt[ERROR_INTR]++;
		dec_3max_errintr();
	}

	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}


/*
 * Handle Memory error.   3max, 3maxplus has ECC.
 * Correct single-bit error, panic on  double-bit error.
 * XXX on double-error on clean user page, mark bad and reload frame?
 */
static void
dec_3max_errintr()
{
	u_int32_t erradr, errsyn, csr;

	/* Fetch error address, ECC chk/syn bits, clear interrupt */
	erradr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR);
	errsyn = MIPS_PHYS_TO_KSEG1(KN02_SYS_CHKSYN);
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_ERRADR) = 0;
	kn02_wbflush();
	csr = *(u_int32_t *)MIPS_PHYS_TO_KSEG1(KN02_SYS_CSR);

	/* Send to kn02/kn03 memory subsystem handler */
	dec_mtasic_err(erradr, errsyn, csr & KN02_CSR_BNK32M);
}
