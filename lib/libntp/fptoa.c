/*	$NetBSD: fptoa.c,v 1.2 1998/01/09 03:16:10 perry Exp $	*/

/*
 * fptoa - return an asciized representation of an s_fp number
 */
#include "ntp_fp.h"
#include "ntp_stdlib.h"

char *
fptoa(fpv, ndec)
	s_fp fpv;
	int ndec;
{
	u_fp plusfp;
	int neg;

	if (fpv < 0) {
		plusfp = (u_fp)(-fpv);
		neg = 1;
	} else {
		plusfp = (u_fp)fpv;
		neg = 0;
	}

	return dofptoa(plusfp, neg, ndec, 0);
}
