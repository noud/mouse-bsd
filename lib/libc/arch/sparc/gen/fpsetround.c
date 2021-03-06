/*	$NetBSD: fpsetround.c,v 1.2 1998/01/09 03:15:18 perry Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_rnd
fpsetround(rnd_dir)
	fp_rnd rnd_dir;
{
	fp_rnd old;
	fp_rnd new;

	__asm__("st %%fsr,%0" : "=m" (*&old));

	new = old;
	new &= ~(0x03 << 30);
	new |= ((rnd_dir & 0x03) << 30);

	__asm__("ld %0,%%fsr" : : "m" (*&new));

	return (old >> 30) & 0x03;
}
