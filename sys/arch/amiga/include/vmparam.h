/*	$NetBSD: vmparam.h,v 1.26 2000/02/14 20:23:20 is Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vmparam.h 1.16 91/01/18$
 *
 *	@(#)vmparam.h	7.3 (Berkeley) 5/7/91
 */
#ifndef _MACHINE_VMPARAM_H_
#define _MACHINE_VMPARAM_H_

#include <machine/pte.h>

/*
 * Machine dependent constants for HP300
 */
/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the P0 region to the
 * beginning of the text and from the beginning of the P1 region to the
 * beginning of the stack respectively.
 *
 * These are a mixture of i386, sun3 and hp settings.. 
 */

/* Sun settings. Still hope, that I might get sun3 binaries to work... */
#define	USRTEXT		0x2000
#ifndef USRSTACK
#define	USRSTACK	0x0E000000
#endif
#define	LOWPAGES	btoc(USRTEXT)
#define KUSER_AREA	(-UPAGES*NBPG)
/*
 * Virtual memory related constants, all in bytes
 */

#ifndef MAXTSIZ
#define	MAXTSIZ		(6*1024*1024)		/* max text size */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(32*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(128*1024*1024)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(32*1024*1024)		/* max stack size */
#endif

/*
 * Sizes of the system and user portions of the system page table.
 */
/* SYSPTSIZE IS SILLY; IT SHOULD BE COMPUTED AT BOOT TIME */
#define	SYSPTSIZE	(2 * NPTEPG)	/* 16mb */
#define	USRPTSIZE 	(1 * NPTEPG)	/* 16mb */

/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * One page is enough to handle 16Mb of simultaneous raw IO operations.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	(1 * NPTEPG)	/* 16mb */
#endif

/*
 * PTEs for system V style shared memory.
 * This is basically slop for kmempt which we actually allocate (malloc) from.
 */
#ifndef SHMMAXPGS
#define SHMMAXPGS	(1 * NPTEPG)	/* 16mb */
#endif

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * user/kernel map constants
 */
#define VM_MIN_ADDRESS		((vaddr_t)0)		/* user min */
#define VM_MAX_ADDRESS		((vaddr_t)(USRSTACK))	/* user max */
#define VM_MAXUSER_ADDRESS	((vaddr_t)(VM_MAX_ADDRESS))	/* same */
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)0)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)(0-NBPG))

/*
 * virtual sizes (bytes) for various kernel submaps
 */
#define VM_PHYS_SIZE		(USRIOSIZE*NBPG)

/*
 * Our bootloader currently passes up to 16 segments (but this is variable)
 * Normally, the biggest of them is used for the kernel, and the kernel
 * segment is given to VM first.
 */
#define VM_PHYSSEG_MAX		(16)
#define VM_PHYSSEG_STRAT	VM_PSTRAT_RANDOM
#define VM_PHYSSEG_NOADD	/* XXX this should be done right later */

/*
 * Allow supporting Zorro-II memory as lower priority:
 *
 *	- DEFAULT for Zorro-III memory (presumably 32 bit)
 *	- ZORROII for Zorro-II memory (16 bit, Zorro-II DMA)
 */

#define VM_NFREELIST		2
#define VM_FREELIST_DEFAULT	0
#define VM_FREELIST_ZORROII	1

/* 
 * pmap-specific data stored in the vm_physmem[] array.
 */   
struct pmap_physseg { 
	struct pv_entry *pvent;         /* pv table for this seg */
	char *attrs;                    /* page attributes for this seg */
}; 

/*
 * number of kernel PT pages (initial only, can grow dynamically)
 */
#define VM_KERNEL_PT_PAGES	((vm_size_t)8)		/* XXX: SYSPTSIZE */
#endif /* !_MACHINE_VMPARAM_H_ */
