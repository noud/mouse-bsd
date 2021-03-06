/*	$NetBSD: disklabel.h,v 1.4 2000/01/23 21:01:56 soda Exp $	*/
/*	$OpenBSD: disklabel.h,v 1.6 1997/04/10 13:06:25 deraadt Exp $	*/
/*	NetBSD: disklabel.h,v 1.3 1996/03/09 20:52:54 ghudson Exp 	*/

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MACHINE_DISKLABEL_H_
#define _MACHINE_DISKLABEL_H_

#define	LABELSECTOR	1		/* sector containing label */
#define	LABELOFFSET	0		/* offset of label in sector */
#define	MAXPARTITIONS	16		/* number of partitions */
#define	RAW_PART	3		/* raw partition: ie. XX?d (XXX) */

#define	OPENBSD_RAW_PART 2		/* raw partition: XX?c */

/* Pull in MBR partition definitions. */
#include <sys/disklabel_mbr.h>
/* XXX - should move to <sys/disklabel_mbr.h> */
#define	MBR_PTYPE_OPENBSD	0xa6	/* OpenBSD partition type */
#define MBR_PTYPE_ONTRACK	0x54

#include <sys/dkbad.h>
struct cpu_disklabel {
	struct mbr_partition dosparts[NMBRPART];
	struct dkbad bad;
};

#ifdef _KERNEL
struct disklabel;
int	bounds_check_with_label __P((struct buf *, struct disklabel *, int));
#endif

#endif /* _MACHINE_DISKLABEL_H_ */
