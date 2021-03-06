/*	$NetBSD: setup.c,v 1.37 1999/11/15 19:18:26 fvdl Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)setup.c	8.10 (Berkeley) 5/9/95";
#else
__RCSID("$NetBSD: setup.c,v 1.37 1999/11/15 19:18:26 fvdl Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/file.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

struct bufarea asblk;
struct fs *altsblock;
#define POWEROF2(num)	(((num) & ((num) - 1)) == 0)

static void badsb __P((int, char *));
static int calcsb __P((const char *, int, struct fs *));
static struct disklabel *getdisklabel __P((const char *, int));
static int readsb __P((int));

/*
 * Read in a superblock finding an alternative if necessary.
 * Return 1 if successful, 0 if unsuccessful, -1 if filesystem
 * is already clean (preen mode only).
 */
int
setup(dev)
	const char *dev;
{
	long cg, size, asked, i, j;
	long bmapsize;
	struct disklabel *lp;
	off_t sizepb;
	struct stat statb;
	struct fs proto;
	int doskipclean;
	u_int64_t maxfilesize;

	havesb = 0;
	fswritefd = -1;
	doskipclean = skipclean;
	if (stat(dev, &statb) < 0) {
		printf("Can't stat %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (!S_ISCHR(statb.st_mode)) {
		pfatal("%s is not a character device", dev);
		if (reply("CONTINUE") == 0)
			return (0);
	}
	if ((fsreadfd = open(dev, O_RDONLY)) < 0) {
		printf("Can't open %s: %s\n", dev, strerror(errno));
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (fswritefd = open(dev, O_WRONLY)) < 0) {
		fswritefd = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	fsmodified = 0;
	lfdir = 0;
	initbarea(&sblk);
	initbarea(&asblk);
	sblk.b_un.b_buf = malloc(SBSIZE);
	sblock = malloc(SBSIZE);
	asblk.b_un.b_buf = malloc(SBSIZE);
	altsblock = malloc(SBSIZE);
	if (sblk.b_un.b_buf == NULL || asblk.b_un.b_buf == NULL ||
		sblock == NULL || altsblock == NULL)
		errx(EEXIT, "cannot allocate space for superblock");
	if ((lp = getdisklabel(NULL, fsreadfd)) != NULL)
		dev_bsize = secsize = lp->d_secsize;
	else
		dev_bsize = secsize = DEV_BSIZE;
	/*
	 * Read in the superblock, looking for alternatives if necessary
	 */
	if (readsb(1) == 0) {
#ifdef LITE2BORKEN
		skipclean = 0;
#endif
		if (bflag || preen || calcsb(dev, fsreadfd, &proto) == 0)
			return(0);
		if (reply("LOOK FOR ALTERNATIVE SUPERBLOCKS") == 0)
			return (0);
		for (cg = 0; cg < proto.fs_ncg; cg++) {
			bflag = fsbtodb(&proto, cgsblock(&proto, cg));
			if (readsb(0) != 0)
				break;
		}
		if (cg >= proto.fs_ncg) {
			printf("%s %s\n%s %s\n%s %s\n",
				"SEARCH FOR ALTERNATIVE SUPER-BLOCK",
				"FAILED. YOU MUST USE THE",
				"-b OPTION TO FSCK_FFS TO SPECIFY THE",
				"LOCATION OF AN ALTERNATIVE",
				"SUPER-BLOCK TO SUPPLY NEEDED",
				"INFORMATION; SEE fsck_ffs(8).");
			return(0);
		}
		doskipclean = 0;
		pwarn("USING ALTERNATIVE SUPERBLOCK AT %d\n", bflag);
	}
	if (debug)
		printf("clean = %d\n", sblock->fs_clean);
	if (doswap)
		doskipclean = 0;
	if (sblock->fs_clean & FS_ISCLEAN) {
		if (doskipclean) {
			pwarn("%sile system is clean; not checking\n",
			    preen ? "f" : "** F");
			return (-1);
		}
		if (!preen && !doswap)
			pwarn("** File system is already clean\n");
	}
	maxfsblock = sblock->fs_size;
	maxino = sblock->fs_ncg * sblock->fs_ipg;
	sizepb = sblock->fs_bsize;
	maxfilesize = sblock->fs_bsize * NDADDR - 1;
	for (i = 0; i < NIADDR; i++) {
		sizepb *= NINDIR(sblock);
		maxfilesize += sizepb;
	}
	/*
	 * Check and potentially fix certain fields in the super block.
	 */
	if (sblock->fs_optim != FS_OPTTIME && sblock->fs_optim != FS_OPTSPACE) {
		pfatal("UNDEFINED OPTIMIZATION IN SUPERBLOCK");
		if (reply("SET TO DEFAULT") == 1) {
			sblock->fs_optim = FS_OPTTIME;
			sbdirty();
		}
	}
	if ((sblock->fs_minfree < 0 || sblock->fs_minfree > 99)) {
		pfatal("IMPOSSIBLE MINFREE=%d IN SUPERBLOCK",
			sblock->fs_minfree);
		if (reply("SET TO DEFAULT") == 1) {
			sblock->fs_minfree = 10;
			sbdirty();
		}
	}
	if (sblock->fs_interleave < 1 ||
	    sblock->fs_interleave > sblock->fs_nsect) {
		pwarn("IMPOSSIBLE INTERLEAVE=%d IN SUPERBLOCK",
			sblock->fs_interleave);
		sblock->fs_interleave = 1;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("SET TO DEFAULT") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_npsect < sblock->fs_nsect ||
	    sblock->fs_npsect > sblock->fs_nsect*2) {
		pwarn("IMPOSSIBLE NPSECT=%d IN SUPERBLOCK",
			sblock->fs_npsect);
		sblock->fs_npsect = sblock->fs_nsect;
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("SET TO DEFAULT") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_bmask != ~(sblock->fs_bsize - 1)) {
		pwarn("INCORRECT BMASK=0x%x IN SUPERBLOCK",
			sblock->fs_bmask);
		sblock->fs_bmask = ~(sblock->fs_bsize - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_fmask != ~(sblock->fs_fsize - 1)) {
		pwarn("INCORRECT FMASK=0x%x IN SUPERBLOCK",
			sblock->fs_fmask);
		sblock->fs_fmask = ~(sblock->fs_fsize - 1);
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sbdirty();
			dirty(&asblk);
		}
	}
	if (sblock->fs_inodefmt >= FS_44INODEFMT) {
		if (sblock->fs_maxfilesize != maxfilesize) {
			pwarn("INCORRECT MAXFILESIZE=%qd IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_maxfilesize);
			sblock->fs_maxfilesize = maxfilesize;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_maxsymlinklen != MAXSYMLINKLEN) {
			pwarn("INCORRECT MAXSYMLINKLEN=%d IN SUPERBLOCK",
				sblock->fs_maxsymlinklen);
			sblock->fs_maxsymlinklen = MAXSYMLINKLEN;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_qbmask != ~sblock->fs_bmask) {
			pwarn("INCORRECT QBMASK=%qx IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_qbmask);
			sblock->fs_qbmask = ~sblock->fs_bmask;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		if (sblock->fs_qfmask != ~sblock->fs_fmask) {
			pwarn("INCORRECT QFMASK=%qx IN SUPERBLOCK",
			    (unsigned long long)sblock->fs_qfmask);
			sblock->fs_qfmask = ~sblock->fs_fmask;
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sbdirty();
				dirty(&asblk);
			}
		}
		newinofmt = 1;
	} else {
		sblock->fs_qbmask = ~sblock->fs_bmask;
		sblock->fs_qfmask = ~sblock->fs_fmask;
		newinofmt = 0;
	}
	/*
	 * Convert to new inode format.
	 */
	if (cvtlevel >= 2 && sblock->fs_inodefmt < FS_44INODEFMT) {
		if (preen)
			pwarn("CONVERTING TO NEW INODE FORMAT\n");
		else if (!reply("CONVERT TO NEW INODE FORMAT"))
			return(0);
		doinglevel2++;
		sblock->fs_inodefmt = FS_44INODEFMT;
		sblock->fs_maxfilesize = maxfilesize;
		sblock->fs_maxsymlinklen = MAXSYMLINKLEN;
		sblock->fs_qbmask = ~sblock->fs_bmask;
		sblock->fs_qfmask = ~sblock->fs_fmask;
		sbdirty();
		dirty(&asblk);
	}
	/*
	 * Convert to new cylinder group format.
	 */
	if (cvtlevel >= 1 && sblock->fs_postblformat == FS_42POSTBLFMT) {
		if (preen)
			pwarn("CONVERTING TO NEW CYLINDER GROUP FORMAT\n");
		else if (!reply("CONVERT TO NEW CYLINDER GROUP FORMAT"))
			return(0);
		doinglevel1++;
		sblock->fs_postblformat = FS_DYNAMICPOSTBLFMT;
		sblock->fs_nrpos = 8;
		sblock->fs_postbloff =
		    (char *)(&sblock->fs_opostbl[0][0]) -
		    (char *)(&sblock->fs_firstfield);
		sblock->fs_rotbloff = &sblock->fs_space[0] -
		    (u_char *)(&sblock->fs_firstfield);
		sblock->fs_cgsize =
			fragroundup(sblock, CGSIZE(sblock));
		sbdirty();
		dirty(&asblk);
	}
	if (asblk.b_dirty && !bflag) {
		memmove((struct fs*)sblk.b_un.b_fs, sblock, SBSIZE);
		if (needswap)
			ffs_sb_swap(sblock, (struct fs*)sblk.b_un.b_fs, 1);
		memmove(asblk.b_un.b_fs, sblk.b_un.b_fs, (size_t)sblock->fs_sbsize);
		flush(fswritefd, &asblk);
	}
	/*
	 * read in the summary info.
	 */
	asked = 0;
	for (i = 0, j = 0; i < sblock->fs_cssize; i += sblock->fs_bsize, j++) {
		size = sblock->fs_cssize - i < sblock->fs_bsize ?
		    sblock->fs_cssize - i : sblock->fs_bsize;
		sblock->fs_csp[j] = (struct csum *)calloc(1, (unsigned)size);
		if (bread(fsreadfd, (char *)sblock->fs_csp[j],
		    fsbtodb(sblock, sblock->fs_csaddr + j * sblock->fs_frag),
		    size) != 0 && !asked) {
			pfatal("BAD SUMMARY INFORMATION");
			if (reply("CONTINUE") == 0) {
				markclean = 0;
				exit(EEXIT);
			}
			asked++;
		}
		/*
		 * The following assumes that struct csum is made of
		 * u_int32_t
		 */
		if (doswap) {
			int k;
			u_int32_t *cd = (u_int32_t *)sblock->fs_csp[j];

			for (k = 0; k < size / sizeof(u_int32_t); k++)
				cd[k] = bswap32(cd[k]);
			bwrite(fswritefd, (char *)sblock->fs_csp[j],
			    fsbtodb(sblock,
				sblock->fs_csaddr + j * sblock->fs_frag),
			    size);
		}
		if (needswap) {
			int k;
			u_int32_t *cd = (u_int32_t *)sblock->fs_csp[j];

			for (k = 0; k < size / sizeof(u_int32_t); k++)
				cd[k] = bswap32(cd[k]);
		}
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsize = roundup(howmany(maxfsblock, NBBY), sizeof(int16_t));
	blockmap = calloc((unsigned)bmapsize, sizeof (char));
	if (blockmap == NULL) {
		printf("cannot alloc %u bytes for blockmap\n",
		    (unsigned)bmapsize);
		goto badsblabel;
	}
	statemap = calloc((unsigned)(maxino + 1), sizeof(char));
	if (statemap == NULL) {
		printf("cannot alloc %u bytes for statemap\n",
		    (unsigned)(maxino + 1));
		goto badsblabel;
	}
	typemap = calloc((unsigned)(maxino + 1), sizeof(char));
	if (typemap == NULL) {
		printf("cannot alloc %u bytes for typemap\n",
		    (unsigned)(maxino + 1));
		goto badsblabel;
	}
	lncntp = (int16_t *)calloc((unsigned)(maxino + 1), sizeof(int16_t));
	if (lncntp == NULL) {
		printf("cannot alloc %u bytes for lncntp\n",
		    (unsigned)((maxino + 1) * sizeof(int16_t)));
		goto badsblabel;
	}
	numdirs = sblock->fs_cstotal.cs_ndir;
	inplast = 0;
	listmax = numdirs + 10;
	inpsort = (struct inoinfo **)calloc((unsigned)listmax,
	    sizeof(struct inoinfo *));
	inphead = (struct inoinfo **)calloc((unsigned)numdirs,
	    sizeof(struct inoinfo *));
	if (inpsort == NULL || inphead == NULL) {
		printf("cannot alloc %u bytes for inphead\n",
		    (unsigned)(numdirs * sizeof(struct inoinfo *)));
		goto badsblabel;
	}
	cgrp = malloc(sblock->fs_cgsize);
	if (cgrp == NULL) {
		printf("cannot alloc %u bytes for cylinder group\n",
		    sblock->fs_cgsize);
		goto badsblabel;
	}
	bufinit();
	if (sblock->fs_flags & FS_DOSOFTDEP)
		usedsoftdep = 1;
	else
		usedsoftdep = 0;
	return (1);

badsblabel:
	markclean=0;
	ckfini();
	return (0);
}

/*
 * Read in the super block and its summary info.
 */
static int
readsb(listerr)
	int listerr;
{
	ufs_daddr_t super = bflag ? bflag : SBOFF / dev_bsize;
	struct fs *fs;

	if (bread(fsreadfd, (char *)sblk.b_un.b_fs, super, (long)SBSIZE) != 0)
		return (0);
	sblk.b_bno = super;
	sblk.b_size = SBSIZE;

	fs = sblk.b_un.b_fs;
	/* auto detect byte order */
	if( fs->fs_magic == FS_MAGIC) {
			if (endian == 0 || BYTE_ORDER == endian) {
				needswap = 0;
				doswap = do_blkswap = do_dirswap = 0;
			} else {
				needswap = 1;
				doswap = do_blkswap = do_dirswap = 1;
			}
	} else if (fs->fs_magic == bswap32(FS_MAGIC)) {
		if (endian == 0 || BYTE_ORDER != endian) {
			needswap = 1;
			doswap = do_blkswap = do_dirswap = 0;
		} else {
			needswap = 0;
			doswap = do_blkswap = do_dirswap = 1;
		}
	} else {
		badsb(listerr, "MAGIC NUMBER WRONG");
		return (0);
	}
	if (doswap) {
		if (preen)
			errx(EEXIT, "incompatible options -B and -p");
		if (nflag)
			errx(EEXIT, "incompatible options -B and -n");
		if (endian == LITTLE_ENDIAN) {
			if (!reply("CONVERT TO LITTLE ENDIAN"))
				return 0;
		} else if (endian == BIG_ENDIAN) {
			if (!reply("CONVERT TO BIG ENDIAN"))
				return 0;
		} else
			pfatal("INTERNAL ERROR: unknown endian");
	}
	if (needswap)
		printf("** Swapped byte order\n");
	/* swap SB byte order if asked */
	if (doswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblk.b_un.b_fs, needswap);

	memmove(sblock, sblk.b_un.b_fs, SBSIZE);
	if (needswap)
		ffs_sb_swap(sblk.b_un.b_fs, sblock, !doswap);

	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock->fs_ncg < 1)
		{ badsb(listerr, "NCG OUT OF RANGE"); return (0); }
	if (sblock->fs_cpg < 1)
		{ badsb(listerr, "CPG OUT OF RANGE"); return (0); }
	if (sblock->fs_ncg * sblock->fs_cpg < sblock->fs_ncyl ||
	    (sblock->fs_ncg - 1) * sblock->fs_cpg >= sblock->fs_ncyl)
		{ badsb(listerr, "NCYL LESS THAN NCG*CPG"); return (0); }
	if (sblock->fs_sbsize > SBSIZE)
		{ badsb(listerr, "SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/*
	 * Compute block size that the filesystem is based on,
	 * according to fsbtodb, and adjust superblock block number
	 * so we can tell if this is an alternative later.
	 */
	super *= dev_bsize;
	dev_bsize = sblock->fs_fsize / fsbtodb(sblock, 1);
	sblk.b_bno = super / dev_bsize;

	if (bflag) {
		havesb = 1;
		return (1);
	}
	/*
	 * Set all possible fields that could differ, then do check
	 * of whole super block against an alternative super block.
	 * When an alternative super-block is specified this check is skipped.
	 */
	getblk(&asblk, cgsblock(sblock, sblock->fs_ncg - 1), sblock->fs_sbsize);
	if (asblk.b_errs)
		return (0);
	/* swap SB byte order if asked */
	if (doswap)
		ffs_sb_swap(asblk.b_un.b_fs, asblk.b_un.b_fs, needswap);

	memmove(altsblock, asblk.b_un.b_fs, sblock->fs_sbsize);
	if (needswap)
		ffs_sb_swap(asblk.b_un.b_fs, altsblock, !doswap);
	altsblock->fs_firstfield = sblock->fs_firstfield;
	altsblock->fs_fscktime = sblock->fs_fscktime;
	altsblock->fs_unused_1 = sblock->fs_unused_1;
	altsblock->fs_time = sblock->fs_time;
	altsblock->fs_cstotal = sblock->fs_cstotal;
	altsblock->fs_cgrotor = sblock->fs_cgrotor;
	altsblock->fs_fmod = sblock->fs_fmod;
	altsblock->fs_clean = sblock->fs_clean;
	altsblock->fs_ronly = sblock->fs_ronly;
	altsblock->fs_flags = sblock->fs_flags;
	altsblock->fs_maxcontig = sblock->fs_maxcontig;
	altsblock->fs_minfree = sblock->fs_minfree;
	altsblock->fs_optim = sblock->fs_optim;
	altsblock->fs_rotdelay = sblock->fs_rotdelay;
	altsblock->fs_maxbpg = sblock->fs_maxbpg;
	memmove(altsblock->fs_csp, sblock->fs_csp, sizeof sblock->fs_csp);
	altsblock->fs_maxcluster = sblock->fs_maxcluster;
	memmove(altsblock->fs_fsmnt, sblock->fs_fsmnt, sizeof sblock->fs_fsmnt);
	memmove(altsblock->fs_sparecon,
		sblock->fs_sparecon, sizeof sblock->fs_sparecon);
	/*
	 * The following should not have to be copied.
	 */
	altsblock->fs_fsbtodb = sblock->fs_fsbtodb;
	altsblock->fs_interleave = sblock->fs_interleave;
	altsblock->fs_npsect = sblock->fs_npsect;
	altsblock->fs_nrpos = sblock->fs_nrpos;
	altsblock->fs_state = sblock->fs_state;
	altsblock->fs_qbmask = sblock->fs_qbmask;
	altsblock->fs_qfmask = sblock->fs_qfmask;
	altsblock->fs_state = sblock->fs_state;
	altsblock->fs_maxfilesize = sblock->fs_maxfilesize;
	if (memcmp(sblock, altsblock, (int)sblock->fs_sbsize)) {
		if (debug) {
			long *nlp, *olp, *endlp;

			printf("superblock mismatches\n");
			nlp = (long *)altsblock;
			olp = (long *)sblock;
			endlp = olp + (sblock->fs_sbsize / sizeof *olp);
			for ( ; olp < endlp; olp++, nlp++) {
				if (*olp == *nlp)
					continue;
				printf("offset %ld, original %lx, alternative %lx\n",
				    (long)(olp - (long *)sblock), *olp, *nlp);
			}
		}
		badsb(listerr,
		"VALUES IN SUPER BLOCK DISAGREE WITH THOSE IN LAST ALTERNATIVE");
		return (0);
	}
	/* Now we know the SB is valid, we can write it back if needed */
	if (doswap) {
		sbdirty();
		dirty(&asblk);
	}
	havesb = 1;
	return (1);
}

static void
badsb(listerr, s)
	int listerr;
	char *s;
{

	if (!listerr)
		return;
	if (preen)
		printf("%s: ", cdevname());
	pfatal("BAD SUPER BLOCK: %s\n", s);
}

/*
 * Calculate a prototype superblock based on information in the disk label.
 * When done the cgsblock macro can be calculated and the fs_ncg field
 * can be used. Do NOT attempt to use other macros without verifying that
 * their needed information is available!
 */
static int
calcsb(dev, devfd, fs)
	const char *dev;
	int devfd;
	struct fs *fs;
{
	struct disklabel *lp;
	struct partition *pp;
	char *cp;
	int i;

	cp = strchr(dev, '\0') - 1;
	if ((cp == (char *)-1 || (*cp < 'a' || *cp > 'h')) && !isdigit(*cp)) {
		pfatal("%s: CANNOT FIGURE OUT FILE SYSTEM PARTITION\n", dev);
		return (0);
	}
	lp = getdisklabel(dev, devfd);
	if (isdigit(*cp))
		pp = &lp->d_partitions[0];
	else
		pp = &lp->d_partitions[*cp - 'a'];
	if (pp->p_fstype != FS_BSDFFS) {
		pfatal("%s: NOT LABELED AS A BSD FILE SYSTEM (%s)\n",
			dev, pp->p_fstype < FSMAXTYPES ?
			fstypenames[pp->p_fstype] : "unknown");
		return (0);
	}
	/* avoid divide by 0 */
	if (pp->p_fsize == 0 || pp->p_frag == 0)
		return (0);
	memset(fs, 0, sizeof(struct fs));
	fs->fs_fsize = pp->p_fsize;
	fs->fs_frag = pp->p_frag;
	fs->fs_cpg = pp->p_cpg;
	fs->fs_size = pp->p_size;
	fs->fs_ntrak = lp->d_ntracks;
	fs->fs_nsect = lp->d_nsectors;
	fs->fs_spc = lp->d_secpercyl;
	fs->fs_nspf = fs->fs_fsize / lp->d_secsize;
	fs->fs_sblkno = roundup(
		howmany(lp->d_bbsize + lp->d_sbsize, fs->fs_fsize),
		fs->fs_frag);
	fs->fs_cgmask = 0xffffffff;
	for (i = fs->fs_ntrak; i > 1; i >>= 1)
		fs->fs_cgmask <<= 1;
	if (!POWEROF2(fs->fs_ntrak))
		fs->fs_cgmask <<= 1;
	fs->fs_cgoffset = roundup(
		howmany(fs->fs_nsect, NSPF(fs)), fs->fs_frag);
	fs->fs_fpg = (fs->fs_cpg * fs->fs_spc) / NSPF(fs);
	fs->fs_ncg = howmany(fs->fs_size / fs->fs_spc, fs->fs_cpg);
	for (fs->fs_fsbtodb = 0, i = NSPF(fs); i > 1; i >>= 1)
		fs->fs_fsbtodb++;
	dev_bsize = lp->d_secsize;
	return (1);
}

static struct disklabel *
getdisklabel(s, fd)
	const char *s;
	int	fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
		if (s == NULL)
			return ((struct disklabel *)NULL);
		pwarn("ioctl (GCINFO): %s\n", strerror(errno));
		errx(EEXIT, "%s: can't read disk label", s);
	}
	return (&lab);
}
