/*	$NetBSD: nanf.c,v 1.1 1999/12/23 10:15:06 kleink Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: nanf.c,v 1.1 1999/12/23 10:15:06 kleink Exp $");
#endif /* not lint */

/* nanf.c */

#include <math.h>

/* bytes for quiet NaN on an arm32 (IEEE single precision) */
const char __nanf[] = { 0, 0, (char)0xc0, 0x7f };
