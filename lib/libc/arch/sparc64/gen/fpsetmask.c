/*	$NetBSD: fpsetmask.c,v 1.1 1998/09/11 04:56:24 eeh Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_except
fpsetmask(mask)
	fp_except mask;
{
	fp_except old;
	fp_except new;

	__asm__("st %%fsr,%0" : "=m" (*&old));

	new = old;
	new &= ~(0x1f << 23);
	new |= ((mask & 0x1f) << 23);

	__asm__("ld %0,%%fsr" : : "m" (*&new));

	return (old >> 23) & 0x1f;
}
