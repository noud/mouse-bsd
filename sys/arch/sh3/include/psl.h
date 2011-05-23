/*	$NetBSD: psl.h,v 1.1 1999/09/13 10:31:21 itojun Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)psl.h	5.2 (Berkeley) 1/18/91
 */

#ifndef _SH3_PSL_H_
#define _SH3_PSL_H_

/*
 * SH3 processor status longword.
 */
#define PSL_TBIT	0x00000001	/* T bit */
#define PSL_SBIT	0x00000002	/* S bit */
#define PSL_IMASK	0x000000f0	/* Interrupt Mask bit */
#define	PSL_QBIT	0x00000100	/* Q bit */
#define	PSL_MBIT	0x00000200	/* M bit */
#define	PSL_BL		0x10000000	/* Block bit */
#define	PSL_RB		0x20000000	/* Register Bank bit */
#define	PSL_MD		0x40000000	/* Processor Mode bit */
                                        /* 1 = supervisor,0=user */

#define	PSL_MBO		0x00000000	/* must be one bits */
#define	PSL_MBZ		0x8ffffc0c	/* must be zero bits */

#define PSL_USERSET    0
#define PSL_USERSTATIC    (PSL_BL|PSL_RB|PSL_MD|PSL_IMASK)

#ifdef _KERNEL
#include <machine/intr.h>
#endif

#endif /* !_SH3_PSL_H_ */
