/*	$NetBSD: fpsetsticky.c,v 1.2 1996/09/16 18:10:52 jonathan Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;
	fp_except new;

	__asm__("cfc1 %0,$31" : "=r" (old));

	new = old;
	new &= ~(0x1f << 2);
	new |= ((sticky & 0x1f) << 2);

	__asm__("ctc1 %0,$31" : : "r" (new));

	return (old >> 2) & 0x1f;
}
