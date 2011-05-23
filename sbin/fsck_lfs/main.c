/*	$NetBSD: main.c,v 1.2 1999/07/03 19:55:03 kleink Exp $	*/

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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <sys/mount.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/lfs/lfs.h>
#include <fstab.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

int	returntosingle;
int	fake_cleanseg = -1;

int	main __P((int, char *[]));

static int	argtoi __P((int, char *, char *, int));
static int	checkfilesys __P((const char *, char *, long, int));
#if 0
static int	docheck __P((struct fstab *));
#endif
static  void usage __P((void));
#ifdef DEBUG_IFILE
void cat_ifile(void);
#endif

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int ch;
	int ret = 0;
	extern char *optarg;
	extern int optind;
#ifndef DEBUG_IFILE
        char *optstring = "b:C:dm:npy";
#else
        char *optstring = "b:C:dim:npy";
#endif

	sync();
	skipclean = 1;
        exitonfail = 0;
	while ((ch = getopt(argc, argv, optstring)) != EOF) {
		switch (ch) {
		case 'b':
			skipclean = 0;
			bflag = argtoi('b', "number", optarg, 10);
			printf("Alternate super block location: %d\n", bflag);
			break;
		case 'C':
			fake_cleanseg = atoi(optarg);
			break;
		case 'd':
			debug++;
			break;
                      case 'e':
                        exitonfail++;
                        break;
#ifdef DEBUG_IFILE
                case 'i':
                        debug_ifile++;
                        break;
#endif
		case 'm':
			lfmode = argtoi('m', "mode", optarg, 8);
			if (lfmode &~ 07777)
				errexit("bad mode to -m: %o\n", lfmode);
			printf("** lost+found creation mode %o\n", lfmode);
			break;

		case 'n':
			nflag++;
			yflag = 0;
			break;

		case 'p':
#if 1
                        /* For an LFS filesystem, "preen" means "do nothing" */
                        /* XXX should it instead mean "run in background"? */
                        exit(0);
#else
			preen++;
			break;
#endif

		case 'y':
			yflag++;
			nflag = 0;
			break;

		default:
			usage();
		}
	}
#ifndef NOTYET
	if(nflag==0) {
		errexit("fsck_lfs cannot write to the filesystem yet; the -n flag is required.\n");
	}
#endif

	argc -= optind;
	argv += optind;

	if (!argc)
		usage();

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);

	while (argc-- > 0)
		(void)checkfilesys(blockcheck(*argv++), 0, 0L, 0);

	if (returntosingle)
		ret = 2;

	exit(ret);
}

static int
argtoi(flag, req, str, base)
	int flag;
	char *req, *str;
	int base;
{
	char *cp;
	int ret;

	ret = (int)strtol(str, &cp, base);
	if (cp == str || *cp)
		errexit("-%c flag requires a %s\n", flag, req);
	return (ret);
}

#if 0
/*
 * Determine whether a filesystem should be checked.
 */
static int
docheck(fsp)
	register struct fstab *fsp;
{

	if ((strcmp(fsp->fs_vfstype, "ufs") &&
	     strcmp(fsp->fs_vfstype, "ffs")) ||
	    (strcmp(fsp->fs_type, FSTAB_RW) &&
	     strcmp(fsp->fs_type, FSTAB_RO)) ||
	    fsp->fs_passno == 0)
		return (0);
	return (1);
}
#endif

/*
 * Check the specified filesystem.
 */
/* ARGSUSED */
static int
checkfilesys(filesys, mntpt, auxdata, child)
	const char *filesys;
        char *mntpt;
	long auxdata;
	int child;
{
	daddr_t n_ffree=0, n_bfree=0;
	struct dups *dp;
	struct zlncnt *zlnp;

	if (preen && child)
		(void)signal(SIGQUIT, voidquit);
	setcdevname(filesys, preen);
	if (debug && preen)
		pwarn("starting\n");
	switch (setup(filesys)) {
	case 0:
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
	case -1:
		return (0);
	}

#ifdef DEBUG_IFILE
        if(debug_ifile==0 && preen == 0)
#else
	if(preen == 0)
#endif
        {
		printf("** Last Mounted on %s\n", sblock.lfs_fsmnt);
		if (hotroot())
			printf("** Root file system\n");
        }

        /*
         * 0: check segment checksums, inode ranges
         */
#ifdef DEBUG_IFILE
        if (debug_ifile==0 && preen == 0) {
#else
	if (preen == 0) {
#endif
            printf("** Phase 0 - Check Segment Summaries\n");
        }
        pass0();
#ifdef DEBUG_IFILE
        if(debug_ifile) {
            cat_ifile();
            exit(0);
        }
#endif
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();
#if 0 /* FFS */
	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}
#endif
	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();
#if 0 /* FFS */
	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();
#endif
	/*
	 * print out summary statistics
	 */
#if 0 /* FFS-specific */
	n_ffree = sblock.lfs_cstotal.cs_nffree;
	n_bfree = sblock.lfs_cstotal.cs_nbfree;
#endif
	pwarn("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.lfs_frag * n_bfree);
#if 0 /* FFS */
	printf("(%d frags, %d blocks, %d.%d%% fragmentation)\n",
	    n_ffree, n_bfree, (n_ffree * 100) / sblock.lfs_dsize,
	    ((n_ffree * 1000 + sblock.lfs_dsize / 2) / sblock.lfs_dsize) % 10);
	if (debug &&
	    (n_files -= maxino - ROOTINO - sblock.lfs_cstotal.cs_nifree))
		printf("%d files missing\n", n_files);
#else
        putchar('\n');
#endif
	if (debug) {
#if 0 /* FFS */
		n_blks += sblock.lfs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.lfs_cssize, sblock.lfs_fsize);
		if (n_blks -= maxfsblock - (n_ffree + sblock.lfs_frag * n_bfree))
			printf("%d blocks missing\n", n_blks);
#endif
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %u,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
	muldup = (struct dups *)0;
	inocleanup();
#if 0 /* no explicit dirty marker in LFS */
	if (fsmodified) {
		(void)time(&sblock.lfs_tstamp);
		sbdirty();
	}
	if (cvtlevel && sblk.b_dirty) {
		/* 
		 * Write out the duplicate super blocks
		 */
		for (cylno = 0; cylno < sblock.lfs_ncg; cylno++)
			bwrite(fswritefd, (char *)&sblock,
			    fsbtodb(&sblock, cgsblock(&sblock, cylno)), LFS_SBPAD);
	}
#endif
	ckfini(1);
	free(blockmap);
	free(statemap);
	free((char *)lncntp);
	if (!fsmodified)
		return (0);
	if (!preen)
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
	if (rerun)
		printf("\n***** PLEASE RERUN FSCK *****\n");
	if (hotroot()) {
		struct statfs stfs_buf;
		/*
		 * We modified the root.  Do a mount update on
		 * it, unless it is read-write, so we can continue.
		 */
		if (statfs("/", &stfs_buf) == 0) {
			long flags = stfs_buf.f_flags;
			struct ufs_args args;
			int ret;

			if (flags & MNT_RDONLY) {
				args.fspec = 0;
				args.export.ex_flags = 0;
				args.export.ex_root = 0;
				flags |= MNT_UPDATE | MNT_RELOAD;
				ret = mount(MOUNT_LFS, "/", flags, &args);
				if (ret == 0)
					return(0);
			}
		}
		if (!preen)
			printf("\n***** REBOOT NOW *****\n");
		sync();
		return (4);
	}
	return (0);
}

static void
usage()
{
	extern char *__progname;

	(void) fprintf(stderr,
	    "Usage: %s [-dnpy] [-b block] [-m mode] filesystem ...\n",
	    __progname);
	exit(1);
}

