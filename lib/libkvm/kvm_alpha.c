/*	$NetBSD: kvm_alpha.c,v 1.15 1999/07/02 15:28:49 simonb Exp $	*/

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#define	__KVM_ALPHA_PRIVATE		/* see <machine/pte.h> */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/kcore.h>
#include <machine/kcore.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <limits.h>
#include <db.h>
#include <stdlib.h>

#include "kvm_private.h"

struct vmstate {
	vsize_t		page_shift;
};

void
_kvm_freevtop(kd)
	kvm_t *kd;
{

	if (kd->vmst != 0)
		free(kd->vmst);
}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	cpu_kcore_hdr_t *cpu_kh;
	struct vmstate *vm;

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == NULL)
		return (-1);

	cpu_kh = kd->cpu_data;

	/* Compute page_shift. */
	for (vm->page_shift = 0; (1L << vm->page_shift) < cpu_kh->page_size;
	     vm->page_shift++)
		/* nothing */ ;
	if ((1L << vm->page_shift) != cpu_kh->page_size) {
		free(vm);
		return (-1);
	}

	kd->vmst = vm;
	return (0);
}

int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	cpu_kcore_hdr_t *cpu_kh;
	struct vmstate *vm;
	alpha_pt_entry_t pte;
	u_long pteoff, page_off;
	int rv;

        if (ISALIVE(kd)) {
                _kvm_err(kd, 0, "vatop called in live kernel!");
                return(0);
        }

	cpu_kh = kd->cpu_data;
	vm = kd->vmst;
	page_off = va & (cpu_kh->page_size - 1);

#define	PAGE_SHIFT	vm->page_shift

	if (va >= ALPHA_K0SEG_BASE && va <= ALPHA_K0SEG_END) {
		/*
		 * Direct-mapped address: just convert it.
		 */

		*pa = ALPHA_K0SEG_TO_PHYS(va);
		rv = cpu_kh->page_size - page_off;
	} else if (va >= ALPHA_K1SEG_BASE && va <= ALPHA_K1SEG_END) {
		/*
		 * Real kernel virtual address: do the translation.
		 */

		/* Find and read the L1 PTE. */
		pteoff = cpu_kh->lev1map_pa +
		    l1pte_index(va) * sizeof(alpha_pt_entry_t);
		if (pread(kd->pmfd, &pte, sizeof(pte),
		    _kvm_pa2off(kd, pteoff)) != sizeof(pte)) {
			_kvm_syserr(kd, 0, "could not read L1 PTE");
			goto lose;
		}

		/* Find and read the L2 PTE. */
		if ((pte & ALPHA_PTE_VALID) == 0) {
			_kvm_err(kd, 0, "invalid translation (invalid L1 PTE)");
			goto lose;
		}
		pteoff = ALPHA_PTE_TO_PFN(pte) * cpu_kh->page_size +
		    l2pte_index(va) * sizeof(alpha_pt_entry_t);
		if (pread(kd->pmfd, &pte, sizeof(pte),
		    _kvm_pa2off(kd, pteoff)) != sizeof(pte)) {
			_kvm_syserr(kd, 0, "could not read L2 PTE");
			goto lose;
		}

		/* Find and read the L3 PTE. */
		if ((pte & ALPHA_PTE_VALID) == 0) {
			_kvm_err(kd, 0, "invalid translation (invalid L2 PTE)");
			goto lose;
		}
		pteoff = ALPHA_PTE_TO_PFN(pte) * cpu_kh->page_size +
		    l3pte_index(va) * sizeof(alpha_pt_entry_t);
		if (pread(kd->pmfd, &pte, sizeof(pte),
		    _kvm_pa2off(kd, pteoff)) != sizeof(pte)) {
			_kvm_syserr(kd, 0, "could not read L3 PTE");
			goto lose;
		}

		/* Fill in the PA. */
		if ((pte & ALPHA_PTE_VALID) == 0) {
			_kvm_err(kd, 0, "invalid translation (invalid L3 PTE)");
			goto lose;
		}
		*pa = ALPHA_PTE_TO_PFN(pte) * cpu_kh->page_size + page_off;
		rv = cpu_kh->page_size - page_off;
	} else {
		/*
		 * Bogus address (not in KV space): punt.
		 */

		_kvm_err(kd, 0, "invalid kernel virtual address");
lose:
		*pa = -1;
		rv = 0;
	}

#undef PAGE_SHIFT

	return (rv);
}

/*
 * Translate a physical address to a file-offset in the crash-dump.
 */
off_t
_kvm_pa2off(kd, pa)
	kvm_t *kd;
	u_long pa;
{
	cpu_kcore_hdr_t *cpu_kh;
	phys_ram_seg_t *ramsegs;
	off_t off;
	int i;

	cpu_kh = kd->cpu_data;
	ramsegs = (phys_ram_seg_t *)((char *)cpu_kh + ALIGN(sizeof *cpu_kh));

	off = 0;
	for (i = 0; i < cpu_kh->nmemsegs; i++) {
		if (pa >= ramsegs[i].start &&
		    (pa - ramsegs[i].start) < ramsegs[i].size) {
			off += (pa - ramsegs[i].start);
			break;
		}
		off += ramsegs[i].size;
	}

	return (kd->dump_off + off);
}

/*
 * Machine-dependent initialization for ALL open kvm descriptors,
 * not just those for a kernel crash dump.  Some architectures
 * have to deal with these NOT being constants!  (i.e. m68k)
 */
int
_kvm_mdopen(kd)
	kvm_t	*kd;
{

	kd->usrstack = USRSTACK;
	kd->min_uva = VM_MIN_ADDRESS;
	kd->max_uva = VM_MAXUSER_ADDRESS;

	return (0);
}
