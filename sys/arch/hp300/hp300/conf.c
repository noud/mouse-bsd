/*	$NetBSD: conf.c,v 1.45 1999/04/19 21:22:58 kleink Exp $	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *      @(#)conf.c	7.9 (Berkeley) 5/28/91
 */

#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include "ct.h"
bdev_decl(ct);
#include "mt.h"
bdev_decl(mt);
#include "rd.h"
bdev_decl(rd);
bdev_decl(sw);
#include "sd.h"
bdev_decl(sd);
#include "ccd.h"
bdev_decl(ccd);
#include "vnd.h"
bdev_decl(vnd);
#include "st.h"
bdev_decl(st);
#include "md.h"
bdev_decl(md);
#include "raid.h"
bdev_decl(raid);
#include "pdisk.h"
bdev_decl(pdisks);
#include "ed.h"
bdev_decl(ed);

struct bdevsw	bdevsw[] =
{
	bdev_tape_init(NCT,ct),		/* 0: cs80 cartridge tape */
	bdev_tape_init(NMT,mt),		/* 1: magnetic reel tape */
	bdev_disk_init(NRD,rd),		/* 2: HPIB disk */
	bdev_swap_init(1,sw),		/* 3: swap pseudo-device */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_disk_init(NCCD,ccd),	/* 5: concatenated disk driver */
	bdev_disk_init(NVND,vnd),	/* 6: vnode disk driver */
	bdev_tape_init(NST,st),		/* 7: SCSI tape */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_disk_init(NMD,md),		/* 14: memory disk */
	bdev_disk_init(NRAID,raid),	/* 15: RAIDframe disk driver */
	bdev_notdef(),			/* 16 */
	bdev_notdef(),			/* 17 */
	bdev_notdef(),			/* 18 */
	bdev_notdef(),			/* 19 */
	bdev_notdef(),			/* 20 */
	bdev_notdef(),			/* 21 */
	bdev_notdef(),			/* 22 */
	bdev_notdef(),			/* 23 */
	bdev_notdef(),			/* 24 */
	bdev_disk_init(NPDISK,pdisks),	/* 25: pseudo disk */
	bdev_disk_init(NED,ed),		/* 26: encrypted disk */
	bdev_notdef(),			/* 27 */
	bdev_notdef(),			/* 28 */
	bdev_notdef(),			/* 29 */
	bdev_notdef(),			/* 30 */
	bdev_notdef(),			/* 31 */
	bdev_notdef(),			/* 32 */
	bdev_notdef(),			/* 33 */
	bdev_notdef(),			/* 34 */
	bdev_notdef(),			/* 35 */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

/* open, close, ioctl, poll, mmap -- XXX should be a map device */
#define	cdev_grf_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), (dev_type_read((*))) nullop, \
	(dev_type_write((*))) nullop, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, dev_init(c,n,poll), \
	dev_init(c,n,mmap) }

/* open, close, read, write, ioctl -- XXX should be a generic device */
#define	cdev_ppi_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, \
	0, (dev_type_poll((*))) enodev, (dev_type_mmap((*))) enodev }

/* open, close, read, ioctl, poll, mmap -- XXX should be a map device */
#define	cdev_hil_init(c,n) { \
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
	(dev_type_write((*))) nullop, dev_init(c,n,ioctl), \
	(dev_type_stop((*))) enodev, 0, dev_init(c,n,poll), \
	dev_init(c,n,mmap) }

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
cdev_decl(sw);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
cdev_decl(ct);
cdev_decl(sd);
cdev_decl(rd);
#include "grf.h"
cdev_decl(grf);
#include "ppi.h"
cdev_decl(ppi);
#include "dca.h"
cdev_decl(dca);
#include "apci.h"
cdev_decl(apci);
#include "ite.h"
cdev_decl(ite);
/* XXX shouldn't this be optional? */
cdev_decl(hil);
#include "dcm.h"
cdev_decl(dcm);
cdev_decl(mt);
cdev_decl(ccd);
cdev_decl(raid);
cdev_decl(vnd);
cdev_decl(st);
cdev_decl(fd);
cdev_decl(md);
cdev_decl(pdisks);
cdev_decl(pdiskm);
cdev_decl(ed);
cdev_decl(edctl);
dev_decl(filedesc,open);
#include "bpfilter.h"
cdev_decl(bpf);
#include "tun.h"
cdev_decl(tun);
#include "ipfilter.h"
#include "rnd.h"

#if 0	/* XXX XXX XXX */
#include "scsibus.h"
#else
#define	NSCSIBUS	0
#endif
cdev_decl(scsibus);

#include "diskwatch.h"
cdev_decl(diskwatch);

#include "encap.h"
cdev_decl(encap);

#include "vlan.h"
cdev_decl(vlan);

#include "srt.h"
cdev_decl(srt);

#include "ethc.h"
cdev_decl(ethc);

#include "pfw.h"
cdev_decl(pfw);

#include "ptape.h"
cdev_decl(ptapes);
cdev_decl(ptapem);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_swap_init(1,sw),		/* 3: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 4: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 5: pseudo-tty master */
	cdev_log_init(1,log),		/* 6: /dev/klog */
	cdev_tape_init(NCT,ct),		/* 7: cs80 cartridge tape */
	cdev_disk_init(NSD,sd),		/* 8: SCSI disk */
	cdev_disk_init(NRD,rd),		/* 9: HPIB disk */
	cdev_grf_init(NGRF,grf),	/* 10: frame buffer */
	cdev_ppi_init(NPPI,ppi),	/* 11: printer/plotter interface */
	cdev_tty_init(NDCA,dca),	/* 12: built-in single-port serial */
	cdev_tty_init(NITE,ite),	/* 13: console terminal emulator */
	cdev_hil_init(1,hil),		/* 14: human interface loop */
	cdev_tty_init(NDCM,dcm),	/* 15: 4-port serial */
	cdev_tape_init(NMT,mt),		/* 16: magnetic reel tape */
	cdev_disk_init(NCCD,ccd),	/* 17: concatenated disk */
	cdev_ipf_init(NIPFILTER,ipl),	/* 18: ip-filter device */
	cdev_disk_init(NVND,vnd),	/* 19: vnode disk driver */
	cdev_tape_init(NST,st),		/* 20: SCSI tape */
	cdev_fd_init(1,filedesc),	/* 21: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 22: Berkeley packet filter */
	cdev_bpftun_init(NTUN,tun),	/* 23: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 24: loadable module driver */
	cdev_lkm_dummy(),		/* 25 */
	cdev_lkm_dummy(),		/* 26 */
	cdev_lkm_dummy(),		/* 27 */
	cdev_lkm_dummy(),		/* 28 */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_tty_init(NAPCI,apci),	/* 31: Apollo APCI UARTs */
	cdev_disk_init(NMD,md),		/* 32: memory disk */
	cdev_rnd_init(NRND,rnd),	/* 33: random source pseudo-device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 34: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 35: RAIDframe disk driver */
	cdev_svr4_net_init(NSVR4_NET,svr4_net), /* 36: svr4 net pseudo-device */
	cdev_notdef(),			/* 37 */
	cdev_notdef(),			/* 38 */
	cdev_notdef(),			/* 39 */
	cdev_notdef(),			/* 40 */
	cdev_notdef(),			/* 41 */
	cdev_notdef(),			/* 42 */
	cdev_notdef(),			/* 43 */
	cdev_notdef(),			/* 44 */
	cdev_notdef(),			/* 45 */
	cdev_notdef(),			/* 46 */
	cdev_notdef(),			/* 47 */
	cdev__oci_init(NENCAP,encap),	/* 48: encap interfaces */
	cdev__ocrwip_init(NDISKWATCH,diskwatch),/* 49: disk watching */
	cdev__oci_init(NVLAN,vlan),	/* 50: vlan interfaces */
	cdev_tape_init(NPTAPE,ptapes),  /* 51: pseudo tape */
	cdev__ocrwip_init(NPTAPE,ptapem),/* 52: pseudo tape controller */
	cdev_disk_init(NPDISK,pdisks),	/* 53: pseudo disk */
	cdev__ocrwip_init(NPDISK,pdiskm),/* 54: pseudo disk controller */
	cdev__oci_init(NETHC,ethc),	/* 55: ethc interfaces */
	cdev__ocrwip_init(NPFW,pfw),	/* 56: reflex packet filtering */
	cdev__oci_init(NSRT,srt),	/* 57: srt interfaces */
	cdev_disk_init(NED,ed),		/* 58: encrypted disk */
	cdev__ocrwip_init(NED,edctl),	/* 59: encrypted disk control */
	cdev_notdef(),			/* 60 */
	cdev_notdef(),			/* 61 */
	cdev_notdef(),			/* 62 */
	cdev_notdef(),			/* 63 */
	cdev_notdef(),			/* 64 */
	cdev_notdef(),			/* 65 */
	cdev_notdef(),			/* 66 */
	cdev_notdef(),			/* 67 */
	cdev_notdef(),			/* 68 */
	cdev_notdef(),			/* 69 */
	cdev_notdef(),			/* 70 */
	cdev_notdef(),			/* 71 */
	cdev_notdef(),			/* 72 */
	cdev_notdef(),			/* 73 */
	cdev_notdef(),			/* 74 */
	cdev_notdef(),			/* 75 */
	cdev_notdef(),			/* 76 */
	cdev_notdef(),			/* 77 */
	cdev_notdef(),			/* 78 */
	cdev_notdef(),			/* 79 */
	cdev_notdef(),			/* 80 */
	cdev_notdef(),			/* 81 */
	cdev_notdef(),			/* 82 */
	cdev_notdef(),			/* 83 */
	cdev_notdef(),			/* 84 */
	cdev_notdef(),			/* 85 */
	cdev_notdef(),			/* 86 */
	cdev_notdef(),			/* 87 */
	cdev_notdef(),			/* 88 */
	cdev_notdef(),			/* 89 */
	cdev_notdef(),			/* 90 */
	cdev_notdef(),			/* 91 */
	cdev_notdef(),			/* 92 */
	cdev_notdef(),			/* 93 */
	cdev_notdef(),			/* 94 */
	cdev_notdef(),			/* 95 */
	cdev_notdef(),			/* 96 */
	cdev_notdef(),			/* 97 */
	cdev_notdef(),			/* 98 */
	cdev_notdef(),			/* 99 */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

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
dev_t	swapdev = makedev(3, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktbl[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	NODEV,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	0,
	/*  8 */	4,
	/*  9 */	2,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	5,
	/* 18 */	NODEV,
	/* 19 */	6,
	/* 20 */	7,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	NODEV,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	14,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	15,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	NODEV,
	/* 40 */	NODEV,
	/* 41 */	NODEV,
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	NODEV,
	/* 47 */	NODEV,
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	NODEV,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
	/* 53 */	25,
	/* 54 */	NODEV,
	/* 55 */	NODEV,
	/* 56 */	NODEV,
	/* 57 */	NODEV,
	/* 58 */	26,
	/* 59 */	NODEV,
	/* 60 */	NODEV,
	/* 61 */	NODEV,
	/* 62 */	NODEV,
	/* 63 */	NODEV,
	/* 64 */	NODEV,
	/* 65 */	NODEV,
	/* 66 */	NODEV,
	/* 67 */	NODEV,
	/* 68 */	NODEV,
	/* 69 */	NODEV,
	/* 70 */	NODEV,
	/* 71 */	NODEV,
	/* 72 */	NODEV,
	/* 73 */	NODEV,
	/* 74 */	NODEV,
	/* 75 */	NODEV,
	/* 76 */	NODEV,
	/* 77 */	NODEV,
	/* 78 */	NODEV,
	/* 79 */	NODEV,
	/* 80 */	NODEV,
	/* 81 */	NODEV,
	/* 82 */	NODEV,
	/* 83 */	NODEV,
	/* 84 */	NODEV,
	/* 85 */	NODEV,
	/* 86 */	NODEV,
	/* 87 */	NODEV,
	/* 88 */	NODEV,
	/* 89 */	NODEV,
	/* 90 */	NODEV,
	/* 91 */	NODEV,
	/* 92 */	NODEV,
	/* 93 */	NODEV,
	/* 94 */	NODEV,
	/* 95 */	NODEV,
	/* 96 */	NODEV,
	/* 97 */	NODEV,
	/* 98 */	NODEV,
	/* 99 */	NODEV,
};

/*
 * Convert a character device number to a block device number.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if ((sizeof(chrtoblktbl)/sizeof(chrtoblktbl[0])) < nchrdev)
		panic("chrtoblktbl too small for cdevsw");
	if (major(dev) >= nchrdev)
		return (NODEV);
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This entire table could be autoconfig()ed but that would mean that
 * the kernel's idea of the console would be out of sync with that of
 * the standalone boot.  I think it best that they both use the same
 * known algorithm unless we see a pressing need otherwise.
 */
#include <dev/cons.h>

#define dvboxcngetc		itecngetc
#define dvboxcnputc		itecnputc
#define dvboxcnpollc		nullcnpollc
cons_decl(dvbox);

#define gboxcngetc		itecngetc
#define gboxcnputc		itecnputc
#define gboxcnpollc		nullcnpollc
cons_decl(gbox);

#define hypercngetc		itecngetc
#define hypercnputc		itecnputc
#define hypercnpollc		nullcnpollc
cons_decl(hyper);

#define rboxcngetc		itecngetc
#define rboxcnputc		itecnputc
#define rboxcnpollc		nullcnpollc
cons_decl(rbox);

#define topcatcngetc		itecngetc
#define topcatcnputc		itecnputc
#define topcatcnpollc		nullcnpollc
cons_decl(topcat);

#define	dcacnpollc		nullcnpollc
cons_decl(dca);

#define	apcicnpollc		nullcnpollc
cons_decl(apci);

#define	dcmcnpollc		nullcnpollc
cons_decl(dcm);

#include "dvbox.h"
#include "gbox.h"
#include "hyper.h"
#include "rbox.h"
#include "topcat.h"

struct	consdev constab[] = {
#if NITE > 0
#if NDVBOX > 0
	cons_init(dvbox),
#endif
#if NGBOX > 0
	cons_init(gbox),
#endif
#if NHYPER > 0
	cons_init(hyper),
#endif
#if NRBOX > 0
	cons_init(rbox),
#endif
#if NTOPCAT > 0
	cons_init(topcat),
#endif
#endif /* NITE > 0 */
#if NDCA > 0
	cons_init(dca),
#endif
#if NAPCI > 0
	cons_init(apci),
#endif
#if NDCM > 0
	cons_init(dcm),
#endif
	{ 0 },
};
