/*	$NetBSD: fpsetround.S,v 1.2 1998/01/09 03:45:06 perry Exp $	*/

/*
 * Written by Charles M. Hannum, Apr 9, 1995
 * Public domain.
 */

#include <machine/asm.h>

ENTRY(fpsetround)
	subl $4,%esp

	fnstcw (%esp)
	movl (%esp),%eax

	rorl $10,%eax
	movl %eax,%edx
	andl $3,%eax

	subl %eax,%edx
	movl 8(%esp),%ecx
	andl $3,%ecx
	orl %ecx,%edx
	roll $10,%edx
	movl %edx,(%esp)
	fldcw (%esp)

	addl $4,%esp
	ret
