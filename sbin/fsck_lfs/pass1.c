/*	$NetBSD: pass1.c,v 1.4 2000/01/20 21:32:32 perseant Exp $	*/

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
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <sys/mount.h>
#include <ufs/lfs/lfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include "fsck.h"
#include "extern.h"
#include "fsutil.h"

extern struct dinode **din_table;

static daddr_t badblk;
static daddr_t dupblk;
static void checkinode __P((ino_t, struct inodesc *));

static void bmapcheck(void)
{
	int i;

	if(testbmap(0))
		raise(1);
	for(i=0;i<maxfsblock;i++)
		if(testbmap(i) > maxino)
			raise(1);
}

void
pass1()
{
	ino_t inumber;
	int total_segments;
	struct inodesc idesc;
        struct dinode *idinode, *tinode;
        struct ifile *ifp;
        CLEANERINFO *cp;

        idinode = lfs_difind(&sblock,sblock.lfs_ifile,&ifblock);

        /*
         * We now have the ifile's inode block in core.  Read out the
         * number of segments.
         */
#if 1
        if(pbp != 0)
            pbp->b_flags &= ~B_INUSE;
        pbp = getddblk(idinode->di_db[0], sblock.lfs_bsize);

        cp = (CLEANERINFO *)(pbp->b_un.b_buf);
#endif
        total_segments = sblock.lfs_size / sblock.lfs_bsize;

	/*
	 * Find all allocated blocks, initialize numdirs.
	 */
	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass1check;
	idesc.id_lblkno = 0;
	inumber = 0;
	n_files = n_blks = 0;

	/* find a value for numdirs */
	numdirs=0;
	for(inumber=0; inumber < maxino; inumber++) {
            tinode = lfs_ginode(inumber);
#if 0 /* debug */
            ifp = lfs_ientry(inumber);
            if(ifp)
                printf("Inode %lu has disk address %lx\n",
                       inumber, ifp->if_daddr);
#endif
            if(tinode && (tinode->di_mode & IFMT)==IFDIR)
                numdirs++;
	}

	/* from setup.c */
        inplast = 0;
        listmax = numdirs + 10;
        inpsort = (struct inoinfo **)calloc((unsigned)listmax,
            sizeof(struct inoinfo *));
        inphead = (struct inoinfo **)calloc((unsigned)numdirs,
            sizeof(struct inoinfo *));
        if (inpsort == NULL || inphead == NULL) {
                printf("cannot alloc %lu bytes for inphead\n",
                    (unsigned long)numdirs * sizeof(struct inoinfo *));
		exit(1);
        }

	/* resetinodebuf(); */

        for(inumber=ROOTINO; inumber <= maxino; inumber++) {
            ifp = lfs_ientry(inumber);
            if(ifp && ifp->if_daddr != LFS_UNUSED_DADDR) {
                checkinode(inumber, &idesc);
#if 1
                if(statemap[inumber]!=USTATE && din_table[inumber]==NULL) {
                    pwarn("Inode %d not claimed by any segment (belongs to %d)\n",inumber,datosn(&sblock,ifp->if_daddr));
                }
#endif
            } else
                statemap[inumber] = USTATE;
        }

	bmapcheck();
	/* freeinodebuf(); */
}

static void
checkinode(inumber, idesc)
	ino_t inumber;
	register struct inodesc *idesc;
{
	register struct dinode *dp;
	struct zlncnt *zlnp;
	int ndb, j;
	mode_t mode;
	char *symbuf;

	/* dp = getnextinode(inumber); */
        dp = lfs_ginode(inumber);

        if(dp==NULL) {
            /* pwarn("Could not find inode %ld\n",(long)inumber); */
            statemap[inumber]=USTATE;
            return;
        }

	mode = dp->di_mode & IFMT;

        /* XXX - LFS doesn't have this particular problem (?) */
	if (mode == 0) {
		if (memcmp(dp->di_db, zino.di_db, NDADDR * sizeof(daddr_t)) ||
		    memcmp(dp->di_ib, zino.di_ib, NIADDR * sizeof(daddr_t)) ||
		    dp->di_mode || dp->di_size) {
			pwarn("mode=o%o, ifmt=o%o\n", dp->di_mode, mode);
			pfatal("PARTIALLY ALLOCATED INODE I=%u", inumber);
			if (reply("CLEAR") == 1) {
				dp = ginode(inumber);
				clearinode(dp);
				inodirty();
			}
		}
		statemap[inumber] = USTATE;
		return;
	}
	lastino = inumber;
	if (/* dp->di_size < 0 || */
	    dp->di_size + sblock.lfs_bsize - 1 < dp->di_size) {
		if (debug)
			printf("bad size %qu:", (unsigned long long)dp->di_size);
		goto unknown;
	}
	if (!preen && mode == IFMT && reply("HOLD BAD BLOCK") == 1) {
		dp = ginode(inumber);
		dp->di_size = sblock.lfs_fsize;
		dp->di_mode = IFREG|0600;
		inodirty();
	}
	ndb = howmany(dp->di_size, sblock.lfs_bsize);
	if (ndb < 0) {
		if (debug)
			printf("bad size %qu ndb %d:",
				(unsigned long long)dp->di_size, ndb);
		goto unknown;
	}
	if (mode == IFBLK || mode == IFCHR)
		ndb++;
	if (mode == IFLNK) {
		/*
		 * Note that the old fastlink format always had di_blocks set
		 * to 0.  Other than that we no longer use the `spare' field
		 * (which is now the extended uid) for sanity checking, the
		 * new format is the same as the old.  We simply ignore the
		 * conversion altogether.  - mycroft, 19MAY1994
		 */
		if (doinglevel2 &&
		    dp->di_size > 0 && dp->di_size < MAXSYMLINKLEN &&
		    dp->di_blocks != 0) {
			symbuf = alloca(secsize);
			if (bread(fsreadfd, symbuf,
			    fsbtodb(&sblock, dp->di_db[0]),
			    (long)secsize) != 0)
				errexit("cannot read symlink");
			if (debug) {
				symbuf[dp->di_size] = 0;
				printf("convert symlink %d(%s) of size %qd\n",
					inumber, symbuf, (long long)dp->di_size);
			}
			dp = ginode(inumber);
			memcpy(dp->di_shortlink, symbuf, (long)dp->di_size);
			dp->di_blocks = 0;
			inodirty();
		}
		/*
		 * Fake ndb value so direct/indirect block checks below
		 * will detect any garbage after symlink string.
		 */
		if (dp->di_size < sblock.lfs_maxsymlinklen ||
		    (sblock.lfs_maxsymlinklen == 0 && dp->di_blocks == 0)) {
			ndb = howmany(dp->di_size, sizeof(daddr_t));
			if (ndb > NDADDR) {
				j = ndb - NDADDR;
				for (ndb = 1; j > 1; j--)
					ndb *= NINDIR(&sblock);
				ndb += NDADDR;
			}
		}
	}
	for (j = ndb; j < NDADDR; j++)
		if (dp->di_db[j] != 0) {
			if (debug)
				printf("bad direct addr: %d\n", dp->di_db[j]);
			goto unknown;
		}
	for (j = 0, ndb -= NDADDR; ndb > 0; j++)
		ndb /= NINDIR(&sblock);
	for (; j < NIADDR; j++)
		if (dp->di_ib[j] != 0) {
			if (debug)
				printf("bad indirect addr: %d\n",
					dp->di_ib[j]);
			goto unknown;
		}
	if (ftypeok(dp) == 0)
		goto unknown;
	n_files++;
	lncntp[inumber] = dp->di_nlink;
	if (dp->di_nlink <= 0) {
		zlnp = (struct zlncnt *)malloc(sizeof *zlnp);
		if (zlnp == NULL) {
			pfatal("LINK COUNT TABLE OVERFLOW");
			if (reply("CONTINUE") == 0)
				errexit("%s", "");
		} else {
			zlnp->zlncnt = inumber;
			zlnp->next = zlnhead;
			zlnhead = zlnp;
		}
	}
	if (mode == IFDIR) {
		if (dp->di_size == 0)
			statemap[inumber] = DCLEAR;
		else
			statemap[inumber] = DSTATE;
		cacheino(dp, inumber);
	} else
		statemap[inumber] = FSTATE;
	typemap[inumber] = IFTODT(mode);
#if 0 /* FFS */
	if (doinglevel2 &&
	    (dp->di_ouid != (u_short)-1 || dp->di_ogid != (u_short)-1)) {
		dp = ginode(inumber);
		dp->di_uid = dp->di_ouid;
		dp->di_ouid = -1;
		dp->di_gid = dp->di_ogid;
		dp->di_ogid = -1;
		inodirty();
	}
#endif
	badblk = dupblk = 0;
	idesc->id_number = inumber;
	(void)ckinode(dp, idesc);
	idesc->id_entryno *= btodb(sblock.lfs_fsize);
	if (dp->di_blocks != idesc->id_entryno) {
		pwarn("INCORRECT BLOCK COUNT I=%u (%d should be %d)",
		    inumber, dp->di_blocks, idesc->id_entryno);
		if (preen)
			printf(" (CORRECTED)\n");
		else if (reply("CORRECT") == 0)
			return;
		dp = ginode(inumber);
		dp->di_blocks = idesc->id_entryno;
		inodirty();
	}
	return;
unknown:
	pfatal("UNKNOWN FILE TYPE I=%u", inumber);
	statemap[inumber] = FCLEAR;
	if (reply("CLEAR") == 1) {
		statemap[inumber] = USTATE;
		dp = ginode(inumber);
		clearinode(dp);
		inodirty();
	}
}

int
pass1check(idesc)
	register struct inodesc *idesc;
{
	int res = KEEPON;
	int anyout, nfrags;
	daddr_t blkno = idesc->id_blkno;
	register struct dups *dlp;
	struct dups *new;

	if ((anyout = chkrange(blkno, idesc->id_numfrags)) != 0) {
		blkerror(idesc->id_number, "BAD", blkno);
		if (badblk++ >= MAXBAD) {
			pwarn("EXCESSIVE BAD BLKS I=%u",
				idesc->id_number);
			if (preen)
				printf(" (SKIPPING)\n");
			else if (reply("CONTINUE") == 0)
				errexit("%s", "");
			return (STOP);
		}
	}
	for (nfrags = idesc->id_numfrags; nfrags > 0; blkno++, nfrags--) {
		if (anyout && chkrange(blkno, 1)) {
			res = SKIP;
		} else if (!testbmap(blkno)) {
			n_blks++;
#ifndef VERBOSE_BLOCKMAP
			setbmap(blkno);
#else
#if 0
			if(idesc->id_number > 10000)
				printf("Oops, inum was %d\n", idesc->id_number);
#endif
			setbmap(blkno,idesc->id_number);
#endif
		} else {
			blkerror(idesc->id_number, "DUP", blkno);
#ifdef VERBOSE_BLOCKMAP
                        pwarn("(lbn %d: Holder is %d)\n", idesc->id_lblkno,
			      testbmap(blkno));
#endif
			if (dupblk++ >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%u",
					idesc->id_number);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("%s", "");
				return (STOP);
			}
			new = (struct dups *)malloc(sizeof(struct dups));
			if (new == NULL) {
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					errexit("%s", "");
				return (STOP);
			}
			new->dup = blkno;
			if (muldup == 0) {
				duplist = muldup = new;
				new->next = 0;
			} else {
				new->next = muldup->next;
				muldup->next = new;
			}
			for (dlp = duplist; dlp != muldup; dlp = dlp->next)
				if (dlp->dup == blkno)
					break;
			if (dlp == muldup && dlp->dup != blkno)
				muldup = new;
		}
		/*
		 * count the number of blocks found in id_entryno
		 */
		idesc->id_entryno ++;
	}
	return (res);
}
