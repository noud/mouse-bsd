/*	$NetBSD: pte.h,v 1.8 1999/05/28 07:23:38 nisimura Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright 1996 The Board of Trustees of The Leland Stanford
 * Junior University. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  Stanford University
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef  __MIPS_PTE_H__
#define  __MIPS_PTE_H__

#include <mips/mips1_pte.h>
#include <mips/mips3_pte.h>

#if !defined(MIPS1) && !defined(MIPS3)
#error Must include at least one MIPS architecture.
#endif

#define	PG_ASID	0x000000ff	/* Address space ID */

#ifndef _LOCORE
#include <mips/cpu.h>

typedef union pt_entry {
	unsigned int	 pt_entry;	/* for copying, etc. */
	struct mips1_pte pt_mips1_pte;	/* for getting to bits by name */
	struct mips3_pte pt_mips3_pte;
} pt_entry_t;

#define	PT_ENTRY_NULL	((pt_entry_t *) 0)

/*
 * Macros/inline functions to hide PTE format differences.
 */

#define	mips_pg_nv_bit()	(MIPS1_PG_NV)	/* same on mips1 and mips3 */


int pmap_is_page_ro(pmap_t, vaddr_t, int);


/* MIPS1-only */
#if defined(MIPS1) && !defined(MIPS3)
#define	mips_pg_v(entry)	((entry) & MIPS1_PG_V)
#define	mips_pg_wired(entry)	((entry) & MIPS1_PG_WIRED)

#define	mips_pg_m_bit()		(MIPS1_PG_D)
#define	mips_pg_rw_bit()	(MIPS1_PG_RW)	/* no RW bits for mips1 */
#define	mips_pg_ro_bit()	(MIPS1_PG_RO)
#define	mips_pg_ropage_bit()	(MIPS1_PG_RO)	/* XXX not MIPS1_PG_ROPAGE? */
#define	mips_pg_rwpage_bit()	(MIPS1_PG_RWPAGE)
#define	mips_pg_cwpage_bit()	(MIPS1_PG_CWPAGE)
#define	mips_pg_global_bit()	(MIPS1_PG_G)
#define	mips_pg_wired_bit()	(MIPS1_PG_WIRED)

#define	PTE_TO_PADDR(pte)	MIPS1_PTE_TO_PADDR((pte))
#define	PAGE_IS_RDONLY(pte, va)	MIPS1_PAGE_IS_RDONLY((pte), (va))

#define	pfn_to_vad(x)		mips1_pfn_to_vad((vaddr_t)(x))
#define	vad_to_pfn(x)		mips1_vad_to_pfn((x))
#endif /* mips1 */


/* MIPS3-only */
#if !defined(MIPS1) && defined(MIPS3)
#define	mips_pg_v(entry)	((entry) & MIPS3_PG_V)
#define	mips_pg_wired(entry)	((entry) & MIPS3_PG_WIRED)

#define	mips_pg_m_bit()		(MIPS3_PG_D)
#define	mips_pg_rw_bit()	(MIPS3_PG_D)
#define	mips_pg_ro_bit()	(MIPS3_PG_RO)
#define	mips_pg_ropage_bit()	(MIPS3_PG_ROPAGE)
#define	mips_pg_rwpage_bit()	(MIPS3_PG_RWPAGE)
#define	mips_pg_cwpage_bit()	(MIPS3_PG_CWPAGE)
#define	mips_pg_global_bit()	(MIPS3_PG_G)
#define	mips_pg_wired_bit()	(MIPS3_PG_WIRED)

#define	PTE_TO_PADDR(pte)	MIPS3_PTE_TO_PADDR((pte))
#define	PAGE_IS_RDONLY(pte, va)	MIPS3_PAGE_IS_RDONLY((pte), (va))

#define	pfn_to_vad(x)		mips3_pfn_to_vad((vaddr_t)(x))
#define	vad_to_pfn(x)		mips3_vad_to_pfn((x))
#endif /* mips3 */

/* MIPS1 and MIPS3 */
#if defined(MIPS1) && defined(MIPS3)

static __inline int
    mips_pg_v(unsigned int entry),
    mips_pg_wired(unsigned int entry),
    PAGE_IS_RDONLY(unsigned int pte, vaddr_t va);

static __inline unsigned int
    mips_pg_wired_bit(void), mips_pg_m_bit(void),
    mips_pg_ro_bit(void), mips_pg_rw_bit(void),
    mips_pg_ropage_bit(void),
    mips_pg_cwpage_bit(void),
    mips_pg_rwpage_bit(void),
    mips_pg_global_bit(void),
    PTE_TO_PADDR(unsigned int entry);

static __inline vaddr_t pfn_to_vad(unsigned int x);
static __inline int vad_to_pfn(vaddr_t x);


static __inline int
mips_pg_v(entry)
	unsigned int entry;
{
	if (CPUISMIPS3)
		return (entry & MIPS3_PG_V);
	return (entry & MIPS1_PG_V);
}

static __inline int
mips_pg_wired(entry)
	unsigned int entry;
{
	if (CPUISMIPS3)
		return (entry & MIPS3_PG_WIRED);
	return (entry & MIPS1_PG_WIRED);
}

static __inline unsigned int
mips_pg_m_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_D);
	return (MIPS1_PG_D);
}

static __inline unsigned int
mips_pg_ro_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_RO);
	return (MIPS1_PG_RO);
}

static __inline unsigned int
mips_pg_rw_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_D);
	return (MIPS1_PG_RW);
}

static __inline unsigned int
mips_pg_ropage_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_ROPAGE);
	return (MIPS1_PG_RO);
}

static __inline unsigned int
mips_pg_rwpage_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_RWPAGE);
	return (MIPS1_PG_RWPAGE);
}

static __inline unsigned int
mips_pg_cwpage_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_CWPAGE);
	return (MIPS1_PG_CWPAGE);
}


static __inline unsigned int
mips_pg_global_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_G);
	return (MIPS1_PG_G);
}

static __inline unsigned int
mips_pg_wired_bit()
{
	if (CPUISMIPS3)
		return (MIPS3_PG_WIRED);
	return (MIPS1_PG_WIRED);
}

static __inline unsigned int
PTE_TO_PADDR(pte)
	unsigned int pte;
{
	if (CPUISMIPS3)
		return (MIPS3_PTE_TO_PADDR(pte));
	return (MIPS1_PTE_TO_PADDR(pte));
}

static __inline int
PAGE_IS_RDONLY(pte, va)
	unsigned int pte;
	vaddr_t va;
{
	if (CPUISMIPS3)
		return (MIPS3_PAGE_IS_RDONLY(pte, va));
	return (MIPS1_PAGE_IS_RDONLY(pte, va));
}

static __inline vaddr_t
pfn_to_vad(x)
	unsigned int x;
{
	if (CPUISMIPS3)
		return (mips3_pfn_to_vad(x));
	return (mips1_pfn_to_vad(x));
}

static __inline int
vad_to_pfn(x)
	vaddr_t x;
{
	if (CPUISMIPS3)
		return (mips3_vad_to_pfn(x));
	return (mips1_vad_to_pfn(x));
}
#endif

#endif /* ! _LOCORE */

#if defined(_KERNEL) && !defined(_LOCORE)
/*
 * Kernel virtual address to page table entry and visa versa.
 */
#define	kvtopte(va) \
	(Sysmap + (((vaddr_t)(va) - VM_MIN_KERNEL_ADDRESS) >> PGSHIFT))
#define	ptetokv(pte) \
	((((pt_entry_t *)(pte) - Sysmap) << PGSHIFT) + VM_MIN_KERNEL_ADDRESS)

extern	pt_entry_t *Sysmap;		/* kernel pte table */
extern	u_int Sysmapsize;		/* number of pte's in Sysmap */
#endif	/* defined(_KERNEL) && !defined(_LOCORE) */

/*
 * User virtual to pte page entry.  Same on mips1 and mips3.
 */
#define	uvtopte(adr)	(((adr) >> PGSHIFT) & (NPTEPG - 1))


#endif /* __MIPS_PTE_H__ */
