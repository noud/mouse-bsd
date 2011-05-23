/* $NetBSD: pfcreg.h,v 1.2 1999/09/16 12:48:35 msaitoh Exp $ */

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

#ifndef _PFCREG_H_
#define _PFCREG_H_

/* address definitions for pin function controller (PFC)*/

#if !defined(SH4)

/* SH3 definition */

#define SHREG_PCTR	(*(volatile unsigned short *)	0xFFFFFF76)
#define SHREG_PDTR	(*(volatile unsigned char *)	0xFFFFFF78)
#define SHREG_SCSPTR	(*(volatile unsigned char *)	0xFFFFFF7C)

#if defined(SH7709) || defined(SH7709A)
#define SHREG_PACR	(*(volatile unsigned short *)0xa4000100)
#define SHREG_PBCR	(*(volatile unsigned short *)0xa4000102)
#define SHREG_PCCR	(*(volatile unsigned short *)0xa4000104)
#define SHREG_PDCR	(*(volatile unsigned short *)0xa4000106)
#define SHREG_PECR	(*(volatile unsigned short *)0xa4000108)
#define SHREG_PFCR	(*(volatile unsigned short *)0xa400010a)
#define SHREG_PGCR	(*(volatile unsigned short *)0xa400010c)
#define SHREG_PHCR	(*(volatile unsigned short *)0xa400010e)
#define SHREG_PJCR	(*(volatile unsigned short *)0xa4000110)
#define SHREG_PKCR	(*(volatile unsigned short *)0xa4000112)
#define SHREG_PLCR	(*(volatile unsigned short *)0xa4000114)
#define SHREG_SCPCR	(*(volatile unsigned short *)0xa4000116)
#endif

#else

/* SH4 definitions */

#define SHREG_PCTRA	(*(volatile unsigned int *)	0xff80002c)
#define SHREG_PDTRA	(*(volatile unsigned short *)	0xff800030)
#define SHREG_PCTRB	(*(volatile unsigned int *)	0xff800040)
#define SHREG_PDTRB	(*(volatile unsigned short *)	0xff800044)
#define SHREG_GPIOIC	(*(volatile unsigned short *)	0xff800048)
#define SHREG_SCSPTR1	(*(volatile unsigned char *)	0xffe0001c)
#define SHREG_SCSPTR2	(*(volatile unsigned short *)	0xffe80020)

#endif

#endif /* _PFCREG_H_ */
