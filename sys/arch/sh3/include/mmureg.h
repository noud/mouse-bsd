/* $NetBSD: mmureg.h,v 1.2 1999/09/16 12:48:35 msaitoh Exp $ */

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_MMUREG_H__
#define _SH3_MMUREG_H__

/*
 * MMU
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xFFFFFFF0)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xFFFFFFF4)
#define SHREG_TTB	(*(volatile unsigned long *)	0xFFFFFFF8)
#define SHREG_TEA	(*(volatile unsigned long *)	0xFFFFFFFC)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xFFFFFFE0)

#else

/* SH4 definitions */

#define SHREG_PTEH	(*(volatile unsigned long *)	0xff000000)
#define SHREG_PTEL	(*(volatile unsigned long *)	0xff000004)
#define SHREG_PTEA	(*(volatile unsigned long *)	0xff000034)
#define SHREG_TTB	(*(volatile unsigned long *)	0xff000008)
#define SHREG_TEA	(*(volatile unsigned long *)	0xff00000c)
#define SHREG_MMUCR	(*(volatile unsigned long *)	0xff000010)

#endif

#ifdef _KERNEL
extern int PageDirReg;
#endif

#endif /* !_SH3_MMUREG_H__ */
