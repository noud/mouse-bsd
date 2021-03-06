/*	$NetBSD: bt478var.h,v 1.7 2000/02/03 04:09:12 nisimura Exp $	*/

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

/*
 * External declarations exported from the bt478 low-level
 * chipset driver.
 */

int	bt478init __P((struct fbinfo *fi));
void	bt478BlankCursor __P((struct fbinfo *fi));
void	bt478RestoreCursorColor __P((struct fbinfo *fi));
void	bt478InitColorMap __P((struct fbinfo *fi));
int	bt478LoadColorMap __P((struct fbinfo *fi, const u_char *bits,
	    int index, int count));
int	bt478GetColorMap __P((struct fbinfo *fi, u_char *bits,
	    int index, int count));
void bt478CursorColor __P((struct fbinfo *fi, u_int *color));
void	bt478CursorColor __P((struct fbinfo *fi, unsigned int color[]));
