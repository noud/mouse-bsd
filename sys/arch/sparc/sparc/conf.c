/*	$NetBSD: conf.c,v 1.62 1999/12/15 08:03:43 garbled Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)conf.c	8.3 (Berkeley) 11/14/93
 */

#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/vnode.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/conf.h>

#include "pty.h"
#include "bpfilter.h"
#include "tun.h"
#include "audio.h"
#include "vnd.h"
#include "raid.h"
#include "ccd.h"
#include "ch.h"
#include "ss.h"
#include "sd.h"
#include "st.h"
#include "cd.h"
#include "uk.h"

#include "kbd.h"
#include "ms.h"
#include "com.h"
#include "zstty.h"
#include "bpp.h"
#include "magma.h"		/* has NMTTY and NMBPP */

#include "fdc.h"		/* has NFDC and NFD; see files.sparc */

#include "bwtwo.h"
#include "cgtwo.h"
#include "cgthree.h"
#include "cgfour.h"
#include "cgsix.h"
#include "cgeight.h"
#include "tcx.h"
#include "cgfourteen.h"
#include "zx.h"
#include "pnozz.h"
#include "fb.h"
#include "tctrl.h"

#include "xd.h"
#include "xy.h"
#include "wd.h"
#include "md.h"
#include "ipfilter.h"
#include "rnd.h"
#include "scsibus.h"
#include "diskwatch.h"
#include "ptape.h"
#include "pdisk.h"
#include "ed.h"
#include "encap.h"
#include "vlan.h"
#include "srt.h"
#include "ethc.h"
#include "pfw.h"
#include "lpvi.h"
#include "sbiti.h"
#include "pev.h"
#include "spif.h"
#include "rtvc.h"

#include "vcoda.h"
cdev_decl(vc_nb_);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NWD,wd),		/* 0: ATA disk */
	bdev_notdef(),			/* 1 */
	bdev_notdef(),			/* 2 */
	bdev_disk_init(NXY,xy),		/* 3: SMD disk */
	bdev_swap_init(1,sw),		/* 4 */
	bdev_disk_init(NMD,md),		/* 5: memory disk */
	bdev_notdef(),			/* 6 */
	bdev_disk_init(NSD,sd),		/* 7: SCSI disk */
	bdev_disk_init(NVND,vnd),	/* 8: vnode disk driver */
	bdev_disk_init(NCCD,ccd),	/* 9: concatenated disk driver */
	bdev_disk_init(NXD,xd),		/* 10: SMD disk */
	bdev_tape_init(NST,st),		/* 11: SCSI tape */
	bdev_notdef(),			/* 12 */
	bdev_notdef(),			/* 13 */
	bdev_notdef(),			/* 14 */
	bdev_notdef(),			/* 15 */
	bdev_disk_init(NFD,fd),		/* 16: floppy disk */
	bdev_notdef(),			/* 17 */
	bdev_disk_init(NCD,cd),		/* 18: SCSI CD-ROM */
	bdev_lkm_dummy(),		/* 19 */
	bdev_lkm_dummy(),		/* 20 */
	bdev_lkm_dummy(),		/* 21 */
	bdev_lkm_dummy(),		/* 22 */
	bdev_lkm_dummy(),		/* 23 */
	bdev_lkm_dummy(),		/* 24 */
	bdev_disk_init(NRAID,raid),	/* 25: RAIDframe disk driver */
	bdev_disk_init(NPDISK,pdisks),	/* 26: pseudo disk */
	bdev_disk_init(NED,ed),		/* 27: encrypted disk */
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

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_tty_init(NKBD,kd), 	/* 1: Sun keyboard/display */
	cdev_ctty_init(1,ctty),		/* 2: controlling terminal */
	cdev_mm_init(1,mm),		/* 3: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),		/* 4: ATA disk */
	cdev_notdef(),			/* 5: tapemaster tape */
	cdev_notdef(),			/* 6: systech/versatec */
	cdev_swap_init(1,sw),		/* 7: /dev/drum (swap pseudo-device) */
	cdev_notdef(),			/* 8: Archive QIC-11 tape */
	cdev_disk_init(NXY,xy),		/* 9: SMD disk on Xylogics 450/451 */
	cdev_notdef(),			/* 10: systech multi-terminal board */
	cdev_notdef(),			/* 11: DES encryption chip */
	cdev_tty_init(NZSTTY,zs),	/* 12: Zilog 8350 serial port */
	cdev_mouse_init(NMS,ms),	/* 13: /dev/mouse */
	cdev_notdef(),			/* 14: cgone */
	cdev_notdef(),			/* 15: sun /dev/winNNN */
	cdev_log_init(1,log),		/* 16: /dev/klog */
	cdev_disk_init(NSD,sd),		/* 17: SCSI disk */
	cdev_tape_init(NST,st),		/* 18: SCSI tape */
	cdev_ch_init(NCH,ch),		/* 19: SCSI autochanger */
	cdev_tty_init(NPTY,pts),	/* 20: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 21: pseudo-tty master */
	cdev_fb_init(NFB,fb),		/* 22: /dev/fb indirect driver */
	cdev_disk_init(NCCD,ccd),	/* 23: concatenated disk driver */
	cdev_fd_init(1,filedesc),	/* 24: file descriptor pseudo-device */
	cdev_ipf_init(NIPFILTER,ipl),	/* 25: ip-filter device */
	cdev_notdef(),			/* 26 */
	cdev_fb_init(NBWTWO,bwtwo),	/* 27: /dev/bwtwo */
	cdev_notdef(),			/* 28: Systech VPC-2200 versatec/centronics */
	cdev_mouse_init(NKBD,kbd),	/* 29: /dev/kbd */
	cdev_notdef(),			/* 30: Xylogics tape */
	cdev_fb_init(NCGTWO,cgtwo),	/* 31: /dev/cgtwo */
	cdev_notdef(),			/* 32: should be /dev/gpone */
	cdev_notdef(),			/* 33 */
	cdev_notdef(),			/* 34 */
	cdev_notdef(),			/* 35 */
	cdev_tty_init(NCOM,com),	/* 36: NS16x50 compatible ports */
	cdev_notdef(),			/* 37 */
	cdev_notdef(),			/* 38 */
	cdev_fb_init(NCGFOUR,cgfour),	/* 39: /dev/cgfour */
	cdev_notdef(),			/* 40 */
	cdev_notdef(),			/* 41 */
	cdev_disk_init(NXD,xd),		/* 42: SMD disk */
	cdev_svr4_net_init(NSVR4_NET,svr4_net),	/* 43: svr4 net pseudo-device */
	cdev_notdef(),			/* 44 */
	cdev_notdef(),			/* 45 */
	cdev_notdef(),			/* 46 */
	cdev_vc_nb_init(NVCODA,vc_nb_),	/* 47: coda file system psuedo-device */
	cdev_notdef(),			/* 48 */
	cdev_notdef(),			/* 49 */
	cdev_notdef(),			/* 50 */
	cdev_notdef(),			/* 51 */
	cdev_notdef(),			/* 52 */
	cdev_notdef(),			/* 53 */
	cdev_disk_init(NFD,fd),		/* 54: floppy disk */
	cdev_fb_init(NCGTHREE,cgthree),	/* 55: /dev/cgthree */
	cdev_notdef(),			/* 56 */
	cdev_notdef(),			/* 57 */
	cdev_disk_init(NCD,cd),		/* 58: SCSI CD-ROM */
	cdev_scanner_init(NSS,ss),	/* 59: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 60: SCSI unknown */
	cdev_disk_init(NMD,md),		/* 61: memory disk */
	cdev_notdef(),			/* 62 */
	cdev_notdef(),			/* 63 */
	cdev_fb_init(NCGEIGHT,cgeight),	/* 64: /dev/cgeight */
	cdev_notdef(),			/* 65 */
	cdev_notdef(),			/* 66 */
	cdev_fb_init(NCGSIX,cgsix),	/* 67: /dev/cgsix */
	cdev_fb_init(NZX,zx),		/* 68: /dev/zx */
	cdev_gen_init(NAUDIO,audio),	/* 69: /dev/audio */
	cdev_openprom_init(1,openprom),	/* 70: /dev/openprom */
	cdev_tctrl_init(NTCTRL,tctrl),	/* 71: /dev/tctrl */
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
	cdev_fb_init(NCGFOURTEEN,cgfourteen), /* 99: /dev/cgfourteen */
	cdev_tty_init(NMTTY,mtty),	/* 100 */
	cdev_gen_init(NMBPP,mbpp),	/* 101 */
	cdev_notdef(),			/* 102 */
	cdev_notdef(),			/* 103 */
	cdev_notdef(),			/* 104 */
	cdev_bpftun_init(NBPFILTER,bpf),/* 105: packet filter */
	cdev_notdef(),			/* 106 */
	cdev_gen_init(NBPP,bpp),	/* 107: on-board parallel port */
	cdev_notdef(),			/* 108 */
	cdev_fb_init(NTCX,tcx),		/* 109: /dev/tcx */
	cdev_disk_init(NVND,vnd),	/* 110: vnode disk driver */
	cdev_bpftun_init(NTUN,tun),	/* 111: network tunnel */
	cdev_lkm_init(NLKM,lkm),	/* 112: loadable module driver */
	cdev_lkm_dummy(),		/* 113 */
	cdev_lkm_dummy(),		/* 114 */
	cdev_lkm_dummy(),		/* 115 */
	cdev_lkm_dummy(),		/* 116 */
	cdev_lkm_dummy(),		/* 117 */
	cdev_lkm_dummy(),		/* 118 */
	cdev_rnd_init(NRND,rnd),	/* 119: random source pseudo-device */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 120: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 121: RAIDframe disk driver */
	cdev_fb_init(NPNOZZ,p9100),	/* 122: /dev/cgfourteen */
	cdev_notdef(),			/* 123 */
	cdev_notdef(),			/* 124 */
	cdev_notdef(),			/* 125 */
	cdev_notdef(),			/* 126 */
	cdev_notdef(),			/* 127 */
	cdev_notdef(),			/* 128 */
	cdev_notdef(),			/* 129 */
	cdev_tape_init(NPTAPE,ptapes),	/* 130: pseudo tape */
	cdev__ocrwip_init(NPTAPE,ptapem), /* 131: pseudo tape controller */
	cdev__oci_init(NENCAP,encap),	/* 132: encap interfaces */
	cdev__oci_init(NSRT,srt),	/* 133: srt interfaces */
	cdev__oci_init(NVLAN,vlan),	/* 134: vlan interfaces */
	cdev__oci_init(NETHC,ethc),	/* 135: ethc interfaces */
	cdev_disk_init(NPDISK,pdisks),	/* 136: pseudo disk */
	cdev__ocrwip_init(NPDISK,pdiskm), /* 137: pseudo disk controller */
	cdev__ocrwip_init(NLPVI,lpvi),	/* 138: SPARCprinter interface */
	cdev__ocrwip_init(NPFW,pfw),	/* 139: reflex packet filtering */
	cdev__ocrwip_init(NDISKWATCH,diskwatch), /* 140: disk watching */
	cdev__ocrwip_init(NPEV,pevs),	/* 141: pseudo-event slave */
	cdev__ocrwip_init(NPEV,pevm),	/* 142: pseudo-event master */
	cdev__ocrwip_init(NED,edctl),	/* 143: encrypted disk control */
	cdev_disk_init(NED,ed),		/* 144: encrypted disk */
	cdev__ocrwip_init(NSBITI,sbiti),/* 145: IOtech,sbiti IEEE488 card */
	cdev_tty_init(NSPTTY,sptty),	/* 146: spif tty */
	cdev_gen_init(NSPBPP,spbpp),	/* 147: spif bpp */
	cdev__ocrwipm_init(NRTVC,rtvc),	/* 148: rtvc */
	cdev_notdef(),			/* 149 */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 3; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(4, 0);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) < 2);
}

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
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	3,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	NODEV,
	/* 14 */	NODEV,
	/* 15 */	NODEV,
	/* 16 */	NODEV,
	/* 17 */	7,
	/* 18 */	11,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	9,
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
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	NODEV,
	/* 40 */	NODEV,
	/* 41 */	NODEV,
	/* 42 */	10,
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
	/* 53 */	NODEV,
	/* 54 */	16,
	/* 55 */	NODEV,
	/* 56 */	NODEV,
	/* 57 */	NODEV,
	/* 58 */	18,
	/* 59 */	NODEV,
	/* 60 */	NODEV,
	/* 61 */	5,
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
	/*100 */	NODEV,
	/*101 */	NODEV,
	/*102 */	NODEV,
	/*103 */	NODEV,
	/*104 */	NODEV,
	/*105 */	NODEV,
	/*106 */	NODEV,
	/*107 */	NODEV,
	/*108 */	NODEV,
	/*109 */	NODEV,
	/*110 */	8,
	/*111 */	NODEV,
	/*112 */	NODEV,
	/*113 */	NODEV,
	/*114 */	NODEV,
	/*115 */	NODEV,
	/*116 */	NODEV,
	/*117 */	NODEV,
	/*118 */	NODEV,
	/*119 */	NODEV,
	/*120 */	NODEV,
	/*121 */	25,
	/*122 */	NODEV,
	/*123 */	NODEV,
	/*124 */	NODEV,
	/*125 */	NODEV,
	/*126 */	NODEV,
	/*127 */	NODEV,
	/*128 */	NODEV,
	/*129 */	NODEV,
	/*130 */	NODEV,
	/*131 */	NODEV,
	/*132 */	NODEV,
	/*133 */	NODEV,
	/*134 */	NODEV,
	/*135 */	NODEV,
	/*136 */	26,
	/*137 */	NODEV,
	/*138 */	NODEV,
	/*139 */	NODEV,
	/*140 */	NODEV,
	/*141 */	NODEV,
	/*142 */	NODEV,
	/*143 */	NODEV,
	/*144 */	27,
	/*145 */	NODEV,
	/*146 */	NODEV,
	/*147 */	NODEV,
	/*148 */	NODEV,
	/*149 */	NODEV,
};

/*
 * Routine to convert from character to block device number.
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
