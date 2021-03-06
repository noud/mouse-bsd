/*	$NetBSD: locore.c,v 1.42 2000/01/17 04:55:26 matt Exp $	*/
/*
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

 /* All bugs are subject to removal without further notice */


#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/user.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/nexus.h>

#include "opt_vax780.h"
#include "opt_vax750.h"
#include "opt_vax8600.h"
#include "opt_vax8200.h"
#include "opt_vax410.h"
#include "opt_vax43.h"
#include "opt_vax46.h"
#include "opt_vax48.h"
#include "opt_vax49.h"
#include "opt_vax630.h"
#include "opt_vax650.h"
#include "opt_vax660.h"
#include "opt_vax670.h"

void	start __P((void));
void	main __P((void));

extern	paddr_t avail_end;
paddr_t	esym;
u_int	proc0paddr;

/*
 * The strict cpu-dependent information is set up here, in
 * form of a pointer to a struct that is specific for each cpu.
 */
extern struct cpu_dep ka780_calls;
extern struct cpu_dep ka750_calls;
extern struct cpu_dep ka860_calls;
extern struct cpu_dep ka820_calls;
extern struct cpu_dep ka43_calls;
extern struct cpu_dep ka46_calls;
extern struct cpu_dep ka48_calls;
extern struct cpu_dep ka49_calls;
extern struct cpu_dep ka410_calls;
extern struct cpu_dep ka630_calls;
extern struct cpu_dep ka650_calls;
extern struct cpu_dep ka660_calls;
extern struct cpu_dep ka670_calls;

/*
 * Start is called from boot; the first routine that is called
 * in kernel. Kernel stack is setup somewhere in a safe place;
 * but we need to move it to a better known place. Memory
 * management is disabled, and no interrupt system is active.
 */
void
start()
{
	extern char cpu_model[];
	extern void *scratch;
	struct pte *pt;

	mtpr(AST_NO, PR_ASTLVL); /* Turn off ASTs */

	findcpu(); /* Set up the CPU identifying variables */

	if (vax_confdata & 0x80)
		strcpy(cpu_model, "MicroVAX ");
	else
		strcpy(cpu_model, "VAXstation ");

	switch (vax_boardtype) {
#if VAX780
	case VAX_BTYP_780:
		dep_call = &ka780_calls;
		strcpy(cpu_model,"VAX 11/780");
		if (vax_cpudata & 0x100)
			cpu_model[9] = '5';
		break;
#endif
#if VAX750
	case VAX_BTYP_750:
		dep_call = &ka750_calls;
		strcpy(cpu_model, "VAX 11/750");
		break;
#endif
#if VAX8600
	case VAX_BTYP_790:
		dep_call = &ka860_calls;
		strcpy(cpu_model,"VAX 8600");
		if (vax_cpudata & 0x100)
			cpu_model[6] = '5';
		break;
#endif
#if VAX410
	case VAX_BTYP_420: /* They are very similar */
		dep_call = &ka410_calls;
		if (((vax_siedata >> 8) & 0xff) == 1)
			strcat(cpu_model, "/m{38,48}");
		else if (((vax_siedata >> 8) & 0xff) == 0)
			strcat(cpu_model, "/m{30,40}");
		break;

	case VAX_BTYP_410:
		dep_call = &ka410_calls;
		strcat(cpu_model, "2000");
		break;
#endif
#if VAX43
	case VAX_BTYP_43:
		dep_call = &ka43_calls;
		strcat(cpu_model, "3100/m76");
		break;
#endif
#if VAX46
	case VAX_BTYP_46:
		dep_call = &ka46_calls;
		strcat(cpu_model, "4000/60");
		break;
#endif
#if VAX48
	case VAX_BTYP_48:
		dep_call = &ka48_calls;
		strcat(cpu_model, "4000 VLC");
		break;
#endif
#if VAX49
	case VAX_BTYP_49:
		dep_call = &ka49_calls;
		strcat(cpu_model, "4000/90");
		break;
#endif
#if VAX630
	case VAX_BTYP_630:
		dep_call = &ka630_calls;
		strcpy(cpu_model,"MicroVAX II");
		break;
#endif
#if VAX650
	case VAX_BTYP_650:
		dep_call = &ka650_calls;
		strcpy(cpu_model,"MicroVAX ");
		switch ((vax_siedata >> 8) & 255) {
		case VAX_SIE_KA640:
			strcat(cpu_model, "3300/3400");
			break;

		case VAX_SIE_KA650:
			strcat(cpu_model, "3500/3600");
			break;

		case VAX_SIE_KA655:
			strcat(cpu_model, "3800/3900");
			break;

		default:
			strcat(cpu_model, "III");
			break;
		}
		break;
#endif
#if VAX660
	case VAX_BTYP_660:
		dep_call = &ka660_calls;
		strcpy(cpu_model,"VAX 4000/200");
		break;
#endif
#if VAX670
	case VAX_BTYP_670:
		dep_call = &ka670_calls;
		strcpy(cpu_model,"VAX 4000/300");
		break;
#endif
#if VAX8200
	case VAX_BTYP_8000:
		mastercpu = mfpr(PR_BINID);
		dep_call = &ka820_calls;
		break;
#endif
	default:
		/* CPU not supported, just give up */
		asm("halt");
	}

        /*
         * Machines older than MicroVAX II have their boot blocks
         * loaded directly or the boot program loaded from console
         * media, so we need to figure out their memory size.
         * This is not easily done on MicroVAXen, so we get it from
         * VMB instead.
         */
        if (avail_end == 0)
                while (badaddr((caddr_t)avail_end, 4) == 0)
                        avail_end += VAX_NBPG * 128;

        avail_end = TRUNC_PAGE(avail_end); /* be sure */

	proc0.p_addr = (void *)proc0paddr; /* XXX */

	/* Clear the used parts of the uarea except for the pcb */
	bzero(&proc0.p_addr->u_stats, sizeof(struct user) - sizeof(struct pcb));

	pmap_bootstrap();

	/* Now running virtual. set red zone for proc0 */
	pt = kvtopte((u_int)proc0.p_addr + REDZONEADDR);
        pt->pg_v = 0;

	((struct pcb *)proc0paddr)->framep = scratch;

	/*
	 * Change mode down to userspace is done by faking a stack
	 * frame that is setup in cpu_set_kpc(). Not done by returning
	 * from main anymore.
	 */
	main();
	/* NOTREACHED */
}
