/* $NetBSD: dec_3min.c,v 1.34 2000/02/03 04:09:02 nisimura Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: dec_3min.c,v 1.34 2000/02/03 04:09:02 nisimura Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <mips/mips/mips_mcclock.h>	/* mcclock CPUspeed estimation */

/* all these to get ioasic_base */
#include <dev/tc/tcvar.h>		/* tc type definitions for.. */
#include <dev/tc/ioasicreg.h>		/* ioasic interrrupt masks */
#include <dev/tc/ioasicvar.h>		/* ioasic_base */

#include <pmax/pmax/machdep.h>
#include <pmax/pmax/kmin.h>		/* 3min baseboard addresses */
#include <pmax/pmax/memc.h>		/* 3min/maxine memory errors */
#include <pmax/tc/sccvar.h>

#include "rasterconsole.h"


/*
 * forward declarations
 */
void		dec_3min_init __P((void));		/* XXX */
static void	dec_3min_bus_reset __P((void));
static void	dec_3min_cons_init __P((void));
static void	dec_3min_device_register __P((struct device *, void *));
static int	dec_3min_intr __P((unsigned, unsigned, unsigned, unsigned));
static void	dec_3min_intr_establish __P((struct device *, void *,
		    int, int (*)(void *), void *));
static void	dec_3min_intr_disestablish __P((struct device *, void *));

static void	kn02ba_wbflush __P((void));
static unsigned	kn02ba_clkread __P((void));


/*
 * Local declarations.
 */
static u_int32_t kmin_tc3_imask;

#ifdef MIPS3
static unsigned latched_cycle_cnt;
#endif


void
dec_3min_init()
{
	int physmem_boardmax;

	platform.iobus = "tcbus";
	platform.bus_reset = dec_3min_bus_reset;
	platform.cons_init = dec_3min_cons_init;
	platform.device_register = dec_3min_device_register;
	platform.iointr = dec_3min_intr;
	platform.intr_establish = dec_3min_intr_establish;
	platform.intr_disestablish = dec_3min_intr_disestablish;
	platform.memsize = memsize_scan;
	platform.clkread = kn02ba_clkread;

	/* clear any memory errors */
	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	ioasic_base = MIPS_PHYS_TO_KSEG1(KMIN_SYS_ASIC);
	mips_hardware_intr = dec_3min_intr;

	/*
	 * Since all the motherboard interrupts come through the
	 * IOASIC, it has to be turned off for all the spls and
	 * since we don't know what kinds of devices are in the
	 * TURBOchannel option slots, just splhigh().
	 */
	splvec.splbio = MIPS_SPL_0_1_2_3;
	splvec.splnet = MIPS_SPL_0_1_2_3;
	splvec.spltty = MIPS_SPL_0_1_2_3;
	splvec.splimp = MIPS_SPL_0_1_2_3;
	splvec.splclock = MIPS_SPL_0_1_2_3;
	splvec.splstatclock = MIPS_SPL_0_1_2_3;

	/* enable posting of MIPS_INT_MASK_3 to CAUSE register */
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = KMIN_INTR_CLOCK;
	/* calibrate cpu_mhz value */
	mc_cpuspeed(ioasic_base+IOASIC_SLOT_8_START, MIPS_INT_MASK_3);

	*(u_int32_t *)(ioasic_base + IOASIC_LANCE_DECODE) = 0x3;
	*(u_int32_t *)(ioasic_base + IOASIC_SCSI_DECODE) = 0xe;
#if 0
	*(u_int32_t *)(ioasic_base + IOASIC_SCC0_DECODE) = (0x10|4);
	*(u_int32_t *)(ioasic_base + IOASIC_SCC1_DECODE) = (0x10|6);
	*(u_int32_t *)(ioasic_base + IOASIC_CSR) = 0x00000f00;
#endif

	/* sanitize interrupt mask */
	kmin_tc3_imask = (KMIN_INTR_CLOCK|KMIN_INTR_PSWARN|KMIN_INTR_TIMEOUT);
	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = kmin_tc3_imask;

	/*
	 * The kmin memory hardware seems to wrap memory addresses
	 * with 4Mbyte SIMMs, which causes the physmem computation
	 * to lose.  Find out how big the SIMMS are and set
	 * max_ physmem accordingly.
	 * XXX Do MAXINEs lose the same way?
	 */
	physmem_boardmax = KMIN_PHYS_MEMORY_END + 1;
	if ((KMIN_MSR_SIZE_16Mb & *(int *)MIPS_PHYS_TO_KSEG1(KMIN_REG_MSR))
			== 0)
		physmem_boardmax = physmem_boardmax >> 2;
	physmem_boardmax = MIPS_PHYS_TO_KSEG1(physmem_boardmax);

	sprintf(cpu_model, "DECstation 5000/1%d (3MIN)", cpu_mhz);
}

/*
 * Initalize the memory system and I/O buses.
 */
static void
dec_3min_bus_reset()
{

	/*
	 * Reset interrupts, clear any errors from newconf probes
	 */

	*(u_int32_t *)MIPS_PHYS_TO_KSEG1(KMIN_REG_TIMEOUT) = 0;
	kn02ba_wbflush();

	*(u_int32_t *)(ioasic_base + IOASIC_INTR) = 0;
	kn02ba_wbflush();
}

static void
dec_3min_cons_init()
{
	int kbd, crt, screen;
	extern int tcfb_cnattach __P((int));		/* XXX */

	kbd = crt = screen = 0;
	prom_findcons(&kbd, &crt, &screen);

	if (screen > 0) {
#if NRASTERCONSOLE > 0
		if (tcfb_cnattach(crt) > 0) {
			scc_lk201_cnattach(ioasic_base, 0x180000);
			return;
		}
#endif
		printf("No framebuffer device configured for slot %d: ", crt);
		printf("using serial console\n");
	}
	/*
	 * Delay to allow PROM putchars to complete.
	 * FIFO depth * character time,
	 * character time = (1000000 / (defaultrate / 10))
	 */
	DELAY(160000000 / 9600);	/* XXX */

	scc_cnattach(ioasic_base, 0x180000);
}


static void
dec_3min_device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	panic("dec_3min_device_register unimplemented");
}


static void
dec_3min_intr_establish(dev, cookie, level, handler, arg)
	struct device *dev;
	void *cookie;
	int level;
	int (*handler) __P((void *));
	void *arg;
{
	int slotno = (int)cookie;
	unsigned mask;

	switch (slotno) {
		/* slots 0-2 don't interrupt through the IOASIC. */
	  case 0:
		mask = MIPS_INT_MASK_0;
		break;
	  case 1:
		mask = MIPS_INT_MASK_1;
		break;
	  case 2:
		mask = MIPS_INT_MASK_2;
		break;

	  case KMIN_SCSI_SLOT:
		mask = (IOASIC_INTR_SCSI | IOASIC_INTR_SCSI_PTR_LOAD |
			IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E);
		break;
	  case KMIN_LANCE_SLOT:
		mask = KMIN_INTR_LANCE;
		break;
	  case KMIN_SCC0_SLOT:
		mask = KMIN_INTR_SCC_0;
		break;
	  case KMIN_SCC1_SLOT:
		mask = KMIN_INTR_SCC_1;
		break;
	  case KMIN_ASIC_SLOT:
		mask = KMIN_INTR_ASIC;
		break;
	  default:
#ifdef DIAGNOSTIC
		printf("warning: enabling unknown intr %x\n", slotno);
#endif
		return;
	}

#if defined(DEBUG) || defined(DIAGNOSTIC)
	printf("3MIN: imask %x, %sabling slot %d, sc %p handler %p\n",
	    kmin_tc3_imask, (on? "en" : "dis"), slotno, sc, handler);
#endif

	/*
	 * Enable the interrupt  handler, and if it's an IOASIC
	 * slot, set the IOASIC interrupt mask.
	 * Otherwise, set the appropriate spl level in the R3000
	 * register.
	 * Be careful to set handlers  before enabling, and disable
	 * interrupts before clearing handlers.
	 */

	/* Set the interrupt handler and argument ... */
	intrtab[slotno].ih_func = handler;
	intrtab[slotno].ih_arg = arg;
	/* ... and set the relevant mask */
	if (slotno <= 2) {
		/* it's an option slot */
		int s = splhigh();
		s |= mask;
		splx(s);
	} else {
		/* it's a baseboard device going via the ASIC */
		kmin_tc3_imask |= mask;
	}

	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = kmin_tc3_imask;
	kn02ba_wbflush();
}


static void
dec_3min_intr_disestablish(dev, arg)
	struct device *dev;
	void *arg;
{
	printf("dec_3min_intr_distestablish: not implemented\n");
}


/*
 * 3min hardware interrupts. (DECstation 5000/1xx)
 */
static int
dec_3min_intr(cpumask, pc, status, cause)
	unsigned cpumask;
	unsigned pc;
	unsigned status;
	unsigned cause;
{
	static int user_warned = 0;
	static int intr_depth = 0;
	u_int32_t old_mask;

	intr_depth++;
	old_mask = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);

	if (cpumask & MIPS_INT_MASK_4)
		prom_haltbutton();

	if (cpumask & MIPS_INT_MASK_3) {
		/* NB: status & MIPS_INT_MASK3 must also be set */
		/* masked interrupts are still observable */
		u_int32_t intr, imsk, turnoff;

		turnoff = 0;
		intr = *(u_int32_t *)(ioasic_base + IOASIC_INTR);
		imsk = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
		intr &= imsk;

		if (intr & IOASIC_INTR_SCSI_PTR_LOAD) {
			turnoff |= IOASIC_INTR_SCSI_PTR_LOAD;
#ifdef notdef
			asc_dma_intr();
#endif
		}

		if (intr & (IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E))
			turnoff |= IOASIC_INTR_SCSI_OVRUN | IOASIC_INTR_SCSI_READ_E;

		if (intr & IOASIC_INTR_LANCE_READ_E)
			turnoff |= IOASIC_INTR_LANCE_READ_E;

		if (turnoff)
			*(u_int32_t *)(ioasic_base + IOASIC_INTR) = ~turnoff;

		if (intr & KMIN_INTR_TIMEOUT)
			kn02ba_errintr();

		if (intr & KMIN_INTR_CLOCK) {
			struct clockframe cf;

			__asm __volatile("lbu $0,48(%0)" ::
				"r"(ioasic_base + IOASIC_SLOT_8_START));
#ifdef MIPS3
			if (CPUISMIPS3) {
				latched_cycle_cnt = mips3_cycle_count();
			}
#endif
			cf.pc = pc;
			cf.sr = status;
			hardclock(&cf);
			intrcnt[HARDCLOCK]++;
		}

		/* If clock interrups were enabled, re-enable them ASAP. */
		if (old_mask & KMIN_INTR_CLOCK) {
			/* ioctl interrupt mask to splclock and higher */
			*(u_int32_t *)(ioasic_base + IOASIC_IMSK)
				= old_mask &
					~(KMIN_INTR_SCC_0|KMIN_INTR_SCC_1 |
					  IOASIC_INTR_LANCE|IOASIC_INTR_SCSI);
			kn02ba_wbflush();
			_splset(MIPS_SR_INT_IE | (status & MIPS_INT_MASK_3));
		}

		if (intr_depth > 1)
			 goto done;

		if ((intr & KMIN_INTR_SCC_0) &&
		    intrtab[KMIN_SCC0_SLOT].ih_func) {
			(*(intrtab[KMIN_SCC0_SLOT].ih_func))
			  (intrtab[KMIN_SCC0_SLOT].ih_arg);
			intrcnt[SERIAL0_INTR]++;
		}

		if ((intr & KMIN_INTR_SCC_1) &&
		    intrtab[KMIN_SCC1_SLOT].ih_func) {
			(*(intrtab[KMIN_SCC1_SLOT].ih_func))
			  (intrtab[KMIN_SCC1_SLOT].ih_arg);
			intrcnt[SERIAL1_INTR]++;
		}

#ifdef notyet /* untested */
		/* If tty interrupts were enabled, re-enable them ASAP. */
		if ((old_mask & (KMIN_INTR_SCC_1|KMIN_INTR_SCC_0)) ==
		     (KMIN_INTR_SCC_1|KMIN_INTR_SCC_0)) {
			*imaskp = old_mask &
			  ~(KMIN_INTR_SCC_0|KMIN_INTR_SCC_1 |
			  IOASIC_INTR_LANCE|IOASIC_INTR_SCSI);
			kn02ba_wbflush();
		}

		/* XXX until we know about SPLs of TC options. */
		if (intr_depth > 1)
			 goto done;
#endif
		if ((intr & IOASIC_INTR_LANCE) &&
		    intrtab[KMIN_LANCE_SLOT].ih_func) {
			(*(intrtab[KMIN_LANCE_SLOT].ih_func))
			  (intrtab[KMIN_LANCE_SLOT].ih_arg);
			intrcnt[LANCE_INTR]++;
		}

		if ((intr & IOASIC_INTR_SCSI) &&
		    intrtab[KMIN_SCSI_SLOT].ih_func) {
			(*(intrtab[KMIN_SCSI_SLOT].ih_func))
			  (intrtab[KMIN_SCSI_SLOT].ih_arg);
			intrcnt[SCSI_INTR]++;
		}

		if (user_warned && ((intr & KMIN_INTR_PSWARN) == 0)) {
			printf("%s\n", "Power supply ok now.");
			user_warned = 0;
		}
		if ((intr & KMIN_INTR_PSWARN) && (user_warned < 3)) {
			user_warned++;
			printf("%s\n", "Power supply overheating");
		}
	}
	if ((cpumask & MIPS_INT_MASK_0) && intrtab[0].ih_func) {
		(*intrtab[0].ih_func)(intrtab[0].ih_arg);
		intrcnt[SLOT0_INTR]++;
 	}

	if ((cpumask & MIPS_INT_MASK_1) && intrtab[1].ih_func) {
		(*intrtab[1].ih_func)(intrtab[1].ih_arg);
		intrcnt[SLOT1_INTR]++;
	}
	if ((cpumask & MIPS_INT_MASK_2) && intrtab[2].ih_func) {
		(*intrtab[2].ih_func)(intrtab[2].ih_arg);
		intrcnt[SLOT2_INTR]++;
	}

done:
	/* restore entry state */
	splhigh();
	intr_depth--;
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = old_mask;


	return (MIPS_SR_INT_IE | (status & ~cause & MIPS_HARD_INT_MASK));
}



/*
 ************************************************************************
 * Extra functions
 ************************************************************************
 */

static void
kn02ba_wbflush()
{
	/* read twice IOASIC_IMSK */
	__asm __volatile("lw $0,%0; lw $0,%0" ::
	    "i"(MIPS_PHYS_TO_KSEG1(KMIN_REG_IMSK)));
}

static unsigned
kn02ba_clkread()
{
#ifdef MIPS3
	if (CPUISMIPS3) {
		u_int32_t mips3_cycles;

		mips3_cycles = mips3_cycle_count() - latched_cycle_cnt;
		/* XXX divides take 78 cycles: approximate with * 41/2048 */
		return((mips3_cycles >> 6) + (mips3_cycles >> 8) +
		       (mips3_cycles >> 11));
	}
#endif
	return 0;
}
