/*	$NetBSD: conf.c,v 1.15 2000/02/01 02:59:30 nisimura Exp $	*/
/*	$OpenBSD: conf.c,v 1.17 1997/05/21 18:31:31 pefo Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	from: @(#)conf.c	8.2 (Berkeley) 11/14/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/conf.h>

/*
 *	Block devices.
 */

#include "vnd.h"
bdev_decl(vnd);
bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "cd.h"
bdev_decl(cd);
#include "fdc.h"
bdev_decl(fd);
#include "wd.h"
bdev_decl(wd);
#if 0 /* XXX - should be fixed */
#include "acd.h"
bdev_decl(acd);
#endif
#include "ccd.h"
#include "md.h"
bdev_decl(md);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NSD,sd),		/* 0: SCSI disk */
	bdev_swap_init(1,sw),		/* 1: should be here swap pseudo-dev */
	bdev_disk_init(NVND,vnd),	/* 2: vnode disk driver */
	bdev_disk_init(NCD,cd),		/* 3: SCSI CD-ROM */
	bdev_disk_init(NWD,wd),		/* 4: ST506/ESDI/IDE disk */
#if 0 /* XXX - should be fixed */
	bdev_disk_init(NACD,acd),	/* 5: ATAPI CD-ROM */
#else
	bdev_notdef(),			/* 5:  */
#endif
	bdev_disk_init(NCCD,ccd),	/* 6: concatenated disk driver */
	bdev_disk_init(NFDC,fd),	/* 7: Floppy disk driver */
	bdev_disk_init(NMD,md),		/* 8: memory disk (for install) */
	bdev_notdef(),			/* 9:  */
	bdev_notdef(),			/* 10:  */
	bdev_notdef(),			/* 11:  */
	bdev_notdef(),			/* 12:  */
	bdev_notdef(),			/* 13:  */
	bdev_notdef(),			/* 14:  */
	bdev_notdef(),			/* 15:  */
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

/*
 *	Character devices.
 */

/* open, close, read, write, ioctl, tty, mmap */
#define cdev_pc_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), \
	dev_init(c,n,tty), ttpoll, dev_init(c,n,mmap), D_TTY }

/* open, close, write, ioctl */
#define	cdev_lpt_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	0, seltrue, (dev_type_mmap((*))) enodev }

/* open, close, write, ioctl */
#define	cdev_spkr_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) enodev, \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, \
	0, seltrue, (dev_type_mmap((*))) enodev }

cdev_decl(cn);
cdev_decl(sw);
cdev_decl(ctty);
#if 0 /* XXX - should be fixed */
cdev_decl(random);
#endif
#define mmread mmrw
#define mmwrite mmrw
dev_type_read(mmrw);
cdev_decl(mm);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
cdev_decl(fd);
#include "st.h"
cdev_decl(st);
#include "fdc.h"
bdev_decl(fd);
cdev_decl(vnd);
cdev_decl(md);
#include "bpfilter.h"
cdev_decl(bpf);
#include "com.h"
cdev_decl(com);
#include "lpt.h"
cdev_decl(lpt);
cdev_decl(sd);
#include "pc.h"
cdev_decl(pc);
cdev_decl(opms);
cdev_decl(cd);
#include "ss.h"
#include "uk.h"
cdev_decl(uk);
cdev_decl(wd);
#if 0 /* XXX - should be fixed */
cdev_decl(acd);
#endif

/* open, close, read, ioctl */
#include "ipfilter.h"
cdev_decl(ipl);

#include "rnd.h"

#include "scsibus.h"
cdev_decl(scsibus);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_swap_init(1,sw),		/* 1: /dev/drum (swap pseudo-device) */
	cdev_ctty_init(1,ctty),		/* 2: controlling terminal */
	cdev_mm_init(1,mm),		/* 3: /dev/{null,mem,kmem,...} */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_fd_init(1,filedesc),	/* 7: file descriptor pseudo-dev */
	cdev_disk_init(NCD,cd),		/* 8: SCSI CD */
	cdev_disk_init(NSD,sd),		/* 9: SCSI disk */
	cdev_tape_init(NST,st),		/* 10: SCSI tape */
	cdev_disk_init(NVND,vnd),	/* 11: vnode disk */
	cdev_bpftun_init(NBPFILTER,bpf),/* 12: berkeley packet filter */
	cdev_disk_init(NFDC,fd),	/* 13: Floppy disk */
	cdev_pc_init(NPC,pc),		/* 14: builtin pc style console dev */
	cdev_mouse_init(NPC,opms),	/* 15: builtin PS2 style mouse */
	cdev_lpt_init(NLPT,lpt),	/* 16: Parallel printer interface */
	cdev_tty_init(NCOM,com),	/* 17: 16C450 serial interface */
	cdev_disk_init(NWD,wd),		/* 18: ST506/ESDI/IDE disk */
#if 0 /* XXX - should be fixed */
	cdev_disk_init(NACD,acd),	/* 19: ATAPI CD-ROM */
#else
	cdev_notdef(),			/* 19: */
#endif
	cdev_tty_init(NPTY,pts),	/* 20: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 21: pseudo-tty master */
	cdev_disk_init(NMD,md),		/* 22: memory disk device */
	cdev_disk_init(NCCD,ccd),       /* 23: concatenated disk driver */
	cdev_notdef(),			/* 24: */
	cdev_notdef(),			/* 25: */
	cdev_notdef(),			/* 26: */
	cdev_notdef(),			/* 27: */
	cdev_notdef(),			/* 28: */
	cdev_notdef(),			/* 29: */
	cdev_notdef(),			/* 30: */
	cdev_ipf_init(NIPFILTER,ipl),	/* 31: IP filter log */
	cdev_uk_init(NUK,uk),		/* 32: unknown SCSI */
	cdev_rnd_init(NRND,rnd),	/* 33: random source pseudo-device */
	cdev_scanner_init(NSS,ss),           /* 34: SCSI scanner */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 35: SCSI bus */
};

int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
iskmemdev(dev)
	dev_t dev;
{

#ifdef COMPAT_BSD44
	if (major(dev) == 2 && (minor(dev) == 0 || minor(dev) == 1))
#else
	if (major(dev) == 3 && (minor(dev) == 0 || minor(dev) == 1))
#endif
		return (1);
	return (0);
}

/*
 * Returns true if def is /dev/zero
 */
int
iszerodev(dev)
	dev_t dev;
{
#ifdef COMPAT_BSD44
	return (major(dev) == 2 && minor(dev) == 12);
#else
	return (major(dev) == 3 && minor(dev) == 12);
#endif
}


#define MAXDEV	36
static int chrtoblktbl[MAXDEV] =  {
      /* VCHR */      /* VBLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		NODEV,
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		NODEV,
	/* 7 */		NODEV,
	/* 8 */		3,		/* cd */
	/* 9 */		0,		/* sd */
	/* 10 */	NODEV,
	/* 11 */	2,		/* vnd */
	/* 12 */	NODEV,
	/* 13 */	7,		/* fd */
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	4,		/* wd */
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	8,		/* md */
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	NODEV,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
};
/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= MAXDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}
