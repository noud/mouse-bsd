/*	$NetBSD: newfs.c,v 1.3 2000/02/12 23:58:09 perseant Exp $	*/

/*-
 * Copyright (c) 1989, 1992, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)newfs.c	8.5 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: newfs.c,v 1.3 2000/02/12 23:58:09 perseant Exp $");
#endif
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/sysctl.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>

#include <disktab.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <paths.h>
#include <util.h>
#include "config.h"
#include "extern.h"

#define	COMPAT			/* allow non-labeled disks */

int	Nflag;			/* run without writing file system */
int	fssize;			/* file system size */
int	sectorsize;		/* bytes/sector */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	minfree = MINFREE;	/* free space threshold */
int	bbsize = BBSIZE;	/* boot block size */
int	sbsize = SBSIZE;	/* superblock size */
u_long	memleft;		/* virtual memory available */
caddr_t	membase;		/* start address of memory based filesystem */
#ifdef COMPAT
char	*disktype;
int	unlabeled;
#endif

char	device[MAXPATHLEN];
char	*progname, *special;

int main __P((int, char **));
static struct disklabel *getdisklabel __P((char *, int));
static struct disklabel *debug_readlabel __P((int));
#ifdef notdef
static void rewritelabel __P((char *, int, struct disklabel *));
#endif
static void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	struct partition *pp;
	struct disklabel *lp;
	struct stat st;
	int debug, force, lfs, fsi, fso, segsize, maxpartitions;
	char *cp, *opstring;

	if ((progname = strrchr(*argv, '/')) != NULL)
		++progname;
	else
		progname = *argv;

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		fatal("insane maxpartitions value %d", maxpartitions);

	opstring = "B:DFLNb:f:m:s:";

	debug = force = lfs = segsize = 0;
	while ((ch = getopt(argc, argv, opstring)) != -1)
		switch(ch) {
		case 'B':	/* LFS segment size */
			if ((segsize = atoi(optarg)) < LFS_MINSEGSIZE)
				fatal("%s: bad segment size", optarg);
			break;
		case 'D':
			debug = 1;
			break;
		case 'F':
			force = 1;
			break;
		case 'L':	/* Create lfs */
			lfs = 1;
			break;
		case 'N':
			Nflag++;
			break;
#ifdef COMPAT
		case 'T':
			disktype = optarg;
			break;
#endif
		case 'b':	/* used for LFS */
			if ((bsize = atoi(optarg)) < LFS_MINBLOCKSIZE)
				fatal("%s: bad block size", optarg);
			break;
		case 'f':
			if ((fsize = atoi(optarg)) <= 0)
				fatal("%s: bad frag size", optarg);
			break;
		case 'm':
			if ((minfree = atoi(optarg)) < 0 || minfree > 99)
				fatal("%s: bad free space %%\n", optarg);
			break;
		case 's':
			if ((fssize = atoi(optarg)) <= 0)
				fatal("%s: bad file system size", optarg);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 && argc != 1)
		usage();

	/*
	 * If the -N flag isn't specified, open the output file.  If no path
	 * prefix, try /dev/r%s and then /dev/%s.
	 */
	special = argv[0];
	if (strchr(special, '/') == NULL) {
		(void)snprintf(device, sizeof(device), "%sr%s", _PATH_DEV,
		    special);
		if (stat(device, &st) == -1)
			(void)snprintf(device, sizeof(device), "%s%s",
			    _PATH_DEV, special);
		special = device;
	}
	if (!Nflag) {
		fso = open(special,
		    (debug ? O_CREAT : 0) | O_WRONLY, DEFFILEMODE);
		if (fso < 0)
			fatal("%s: %s", special, strerror(errno));
	} else
		fso = -1;

	/* Open the input file. */
	fsi = open(special, O_RDONLY);
	if (fsi < 0)
		fatal("%s: %s", special, strerror(errno));
	if (fstat(fsi, &st) < 0)
		fatal("%s: %s", special, strerror(errno));

	if (!debug && !S_ISCHR(st.st_mode))
		(void)printf("%s: %s: not a character-special device\n",
		    progname, special);
	cp = strchr(argv[0], '\0') - 1;
	if (!debug
	    && (cp == 0 || ((*cp < 'a' || *cp > ('a' + maxpartitions - 1))
	    && !isdigit(*cp))))
		fatal("%s: can't figure out file system partition", argv[0]);

#ifdef COMPAT
	if (disktype == NULL)
		disktype = argv[1];
#endif
	if (debug)
		lp = debug_readlabel(fsi);
	else
		lp = getdisklabel(special, fsi);

	if (isdigit(*cp))
		pp = &lp->d_partitions[0];
	else
		pp = &lp->d_partitions[*cp - 'a'];
	if (pp->p_size == 0)
		fatal("%s: `%c' partition is unavailable", argv[0], *cp);

	/* If force, make the partition look like an LFS */
	if(force) {
		pp->p_fstype = FS_BSDLFS;
		/* 0 means to use defaults */
		pp->p_fsize  = 0;
		pp->p_frag   = 0;
		pp->p_sgs    = 0;
	}

	/* If we're making a LFS, we break out here */
	exit(make_lfs(fso, lp, pp, minfree, bsize, fsize, segsize));
}

#ifdef COMPAT
char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
char lmsg[] = "%s: can't read disk label";
#endif

static struct disklabel *
getdisklabel(s, fd)
	char *s;
	int fd;
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, (char *)&lab) < 0) {
#ifdef COMPAT
		if (disktype) {
			struct disklabel *lp;

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				fatal("%s: unknown disk type", disktype);
			return (lp);
		}
#endif
		(void)fprintf(stderr,
		    "%s: ioctl (GDINFO): %s\n", progname, strerror(errno));
		fatal(lmsg, s);
	}
	return (&lab);
}


static struct disklabel *
debug_readlabel(fd)
	int fd;
{
	static struct disklabel lab;
	int n;

	if ((n = read(fd, &lab, sizeof(struct disklabel))) < 0)
		fatal("unable to read disk label: %s", strerror(errno));
	else if (n < sizeof(struct disklabel))
		fatal("short read of disklabel: %d of %d bytes", n,
			sizeof(struct disklabel));
	return(&lab);
}

#ifdef notdef
static void
rewritelabel(s, fd, lp)
	char *s;
	int fd;
	struct disklabel *lp;
{
#ifdef COMPAT
	if (unlabeled)
		return;
#endif
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		(void)fprintf(stderr,
		    "%s: ioctl (WDINFO): %s\n", progname, strerror(errno));
		fatal("%s: can't rewrite disk label", s);
	}
#if __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		int i;
		int cfd;
		daddr_t alt;
		char specname[64];
		char blk[1024];
		char *cp;

		/*
		 * Make name for 'c' partition.
		 */
		strcpy(specname, s);
		cp = specname + strlen(specname) - 1;
		if (!isdigit(*cp))
			*cp = 'c';
		cfd = open(specname, O_WRONLY);
		if (cfd < 0)
			fatal("%s: %s", specname, strerror(errno));
		memset(blk, 0, sizeof(blk));
		*(struct disklabel *)(blk + LABELOFFSET) = *lp;
		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			if (lseek(cfd, (off_t)((alt + i) * lp->d_secsize),
			    SEEK_SET) == -1)
				fatal("lseek to badsector area: %s",
				    strerror(errno));
			if (write(cfd, blk, lp->d_secsize) < lp->d_secsize)
				fprintf(stderr,
				    "%s: alternate label %d write: %s\n",
				    progname, i/2, strerror(errno));
		}
		close(cfd);
	}
#endif /* vax */
}
#endif /* notdef */

void
usage()
{
	fprintf(stderr, "usage: newfs_lfs [ -fsoptions ] special-device\n");
	fprintf(stderr, "where fsoptions are:\n");
	fprintf(stderr, "\t-B LFS segment size\n");
	fprintf(stderr, "\t-D debug\n");
	/* fprintf(stderr, "\t-L create LFS file system\n"); */
	fprintf(stderr,
	    "\t-N do not create file system, just print out parameters\n");
	fprintf(stderr, "\t-b block size\n");
	fprintf(stderr, "\t-f frag size\n");
	fprintf(stderr, "\t-m minimum free space %%\n");
	fprintf(stderr, "\t-s file system size (sectors)\n");
	exit(1);
}
