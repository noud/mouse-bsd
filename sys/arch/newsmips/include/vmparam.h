/*	$NetBSD: vmparam.h,v 1.3 1998/07/08 04:43:18 thorpej Exp $	*/

#include <mips/vmparam.h>

/*
 * NEWS has one physical memory segment.
 */
#define	VM_PHYSSEG_MAX		1

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* pcb base */
/*#define	pcbb(p)		((u_int)(p)->p_addr) */
