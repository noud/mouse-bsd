/*	$NetBSD: inode.c,v 1.8 2000/01/28 16:01:46 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.
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
static char sccsid[] = "@(#)inode.c	8.5 (Berkeley) 2/8/95";
#else
__RCSID("$NetBSD: inode.c,v 1.8 2000/01/28 16:01:46 bouyer Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <ufs/ext2fs/ext2fs_dinode.h>
#include <ufs/ext2fs/ext2fs_dir.h>
#include <ufs/ext2fs/ext2fs.h>

#include <ufs/ufs/dinode.h> /* for IFMT & friends */
#ifndef SMALL
#include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"

/*
 * CG is stored in fs byte order in memory, so we can't use ino_to_fsba
 * here.
 */

#define fsck_ino_to_fsba(fs, x)                      \
	(fs2h32((fs)->e2fs_gd[ino_to_cg(fs, x)].ext2bgd_i_tables) + \
	(((x)-1) % (fs)->e2fs.e2fs_ipg)/(fs)->e2fs_ipb)


static ino_t startinum;

static int iblock __P((struct inodesc *, long, u_int64_t));

int
ckinode(dp, idesc)
	struct ext2fs_dinode *dp;
	struct inodesc *idesc;
{
	u_int32_t *ap;
	long ret, n, ndb;
	struct ext2fs_dinode dino;
	u_int64_t remsize, sizepb;
	mode_t mode;
	char pathbuf[MAXPATHLEN + 1];

	if (idesc->id_fix != IGNORE)
		idesc->id_fix = DONTKNOW;
	idesc->id_entryno = 0;
	idesc->id_filesize = fs2h32(dp->e2di_size);
	mode = fs2h16(dp->e2di_mode) & IFMT;
	if (mode == IFBLK || mode == IFCHR || mode == IFIFO ||
	    (mode == IFLNK && (fs2h32(dp->e2di_size) < EXT2_MAXSYMLINKLEN)))
		return (KEEPON);
	dino = *dp;
	ndb = howmany(fs2h32(dino.e2di_size), sblock.e2fs_bsize);
	for (ap = &dino.e2di_blocks[0]; ap < &dino.e2di_blocks[NDADDR];
																ap++,ndb--) {
		idesc->id_numfrags = 1;
		if (*ap == 0) {
			if (idesc->id_type == DATA && ndb > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->e2di_size = h2fs32((ap - &dino.e2di_blocks[0]) *
					    sblock.e2fs_bsize);
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
				}
			}
			continue;
		}
		idesc->id_blkno = fs2h32(*ap);
		if (idesc->id_type == ADDR)
			ret = (*idesc->id_func)(idesc);
		else
			ret = dirscan(idesc);
		if (ret & STOP)
			return (ret);
	}
	idesc->id_numfrags = 1;
	remsize = fs2h32(dino.e2di_size) - sblock.e2fs_bsize * NDADDR;
	sizepb = sblock.e2fs_bsize;
	for (ap = &dino.e2di_blocks[NDADDR], n = 1; n <= NIADDR; ap++, n++) {
		if (*ap) {
			idesc->id_blkno = fs2h32(*ap);
			ret = iblock(idesc, n, remsize);
			if (ret & STOP)
				return (ret);
		} else {
			if (idesc->id_type == DATA && remsize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->e2di_size = h2fs32(fs2h32(dp->e2di_size) - remsize);
					remsize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
					break;
				}
			}
		}
		sizepb *= NINDIR(&sblock);
		remsize -= sizepb;
	}
	return (KEEPON);
}

static int
iblock(idesc, ilevel, isize)
	struct inodesc *idesc;
	long ilevel;
	u_int64_t isize;
{
	daddr_t *ap;
	daddr_t *aplim;
	struct bufarea *bp;
	int i, n, (*func) __P((struct inodesc *)), nif;
	u_int64_t sizepb;
	char buf[BUFSIZ];
	char pathbuf[MAXPATHLEN + 1];
	struct ext2fs_dinode *dp;

	if (idesc->id_type == ADDR) {
		func = idesc->id_func;
		if (((n = (*func)(idesc)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (chkrange(idesc->id_blkno, idesc->id_numfrags))
		return (SKIP);
	bp = getdatablk(idesc->id_blkno, sblock.e2fs_bsize);
	ilevel--;
	for (sizepb = sblock.e2fs_bsize, i = 0; i < ilevel; i++)
		sizepb *= NINDIR(&sblock);
	if (isize > sizepb * NINDIR(&sblock))
		nif = NINDIR(&sblock);
	else
		nif = howmany(isize, sizepb);
	if (idesc->id_func == pass1check &&
		nif < NINDIR(&sblock)) {
		aplim = &bp->b_un.b_indir[NINDIR(&sblock)];
		for (ap = &bp->b_un.b_indir[nif]; ap < aplim; ap++) {
			if (*ap == 0)
				continue;
			(void)snprintf(buf, sizeof(buf),
			    "PARTIALLY TRUNCATED INODE I=%u", idesc->id_number);
			if (dofix(idesc, buf)) {
				*ap = 0;
				dirty(bp);
			}
		}
		flush(fswritefd, bp);
	}
	aplim = &bp->b_un.b_indir[nif];
	for (ap = bp->b_un.b_indir; ap < aplim; ap++) {
		if (*ap) {
			idesc->id_blkno = fs2h32(*ap);
			if (ilevel == 0)
				n = (*func)(idesc);
			else
				n = iblock(idesc, ilevel, isize);
			if (n & STOP) {
				bp->b_flags &= ~B_INUSE;
				return (n);
			}
		} else {
			if (idesc->id_type == DATA && isize > 0) {
				/* An empty block in a directory XXX */
				getpathname(pathbuf, idesc->id_number,
				    idesc->id_number);
				pfatal("DIRECTORY %s: CONTAINS EMPTY BLOCKS",
				    pathbuf);
				if (reply("ADJUST LENGTH") == 1) {
					dp = ginode(idesc->id_number);
					dp->e2di_size = h2fs32(fs2h32(dp->e2di_size) - isize);
					isize = 0;
					printf(
					    "YOU MUST RERUN FSCK AFTERWARDS\n");
					rerun = 1;
					inodirty();
					bp->b_flags &= ~B_INUSE;
					return(STOP);
				}
			}
		}
		isize -= sizepb;
	}
	bp->b_flags &= ~B_INUSE;
	return (KEEPON);
}

/*
 * Check that a block in a legal block number.
 * Return 0 if in range, 1 if out of range.
 */
int
chkrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	int c, overh;

	if ((unsigned)(blk + cnt) > maxfsblock)
		return (1);
	c = dtog(&sblock, blk);
	overh = cgoverhead(c);
	if (blk < sblock.e2fs.e2fs_bpg * c + overh +
	    sblock.e2fs.e2fs_first_dblock) {
		if ((blk + cnt) > sblock.e2fs.e2fs_bpg * c + overh +
		    sblock.e2fs.e2fs_first_dblock) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, sblock.e2fs.e2fs_bpg * c + overh +
				    sblock.e2fs.e2fs_first_dblock);
				printf(" blk + cnt %d > cgsbase %d\n",
				    blk + cnt, sblock.e2fs.e2fs_bpg * c +
				    overh + sblock.e2fs.e2fs_first_dblock);
			}
			return (1);
		}
	} else {
		if ((blk + cnt) > sblock.e2fs.e2fs_bpg * (c + 1) + overh +
		    sblock.e2fs.e2fs_first_dblock) {
			if (debug)  {
				printf("blk %d >= cgdmin %d;",
				    blk, sblock.e2fs.e2fs_bpg * c + overh +
				    sblock.e2fs.e2fs_first_dblock);
				printf(" blk + cnt %d > cgdmax %d\n",
				    blk+cnt, sblock.e2fs.e2fs_bpg * (c + 1) +
				    overh + sblock.e2fs.e2fs_first_dblock);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * General purpose interface for reading inodes.
 */
struct ext2fs_dinode *
ginode(inumber)
	ino_t inumber;
{
	daddr_t iblk;

	if ((inumber < EXT2_FIRSTINO && inumber != EXT2_ROOTINO)
		|| inumber > maxino)
		errexit("bad inode number %d to ginode\n", inumber);
	if (startinum == 0 ||
	    inumber < startinum || inumber >= startinum + sblock.e2fs_ipb) {
		iblk = fsck_ino_to_fsba(&sblock, inumber);
		if (pbp != 0)
			pbp->b_flags &= ~B_INUSE;
		pbp = getdatablk(iblk, sblock.e2fs_bsize);
		startinum = ((inumber -1) / sblock.e2fs_ipb) * sblock.e2fs_ipb + 1;
	}
	return (&pbp->b_un.b_dinode[(inumber-1) % sblock.e2fs_ipb]);
}

/*
 * Special purpose version of ginode used to optimize first pass
 * over all the inodes in numerical order.
 */
ino_t nextino, lastinum;
long readcnt, readpercg, fullcnt, inobufsize, partialcnt, partialsize;
struct ext2fs_dinode *inodebuf;

struct ext2fs_dinode *
getnextinode(inumber)
	ino_t inumber;
{
	long size;
	daddr_t dblk;
	static struct ext2fs_dinode *dp;

	if (inumber != nextino++ || inumber > maxino)
		errexit("bad inode number %d to nextinode\n", inumber);
	if (inumber >= lastinum) {
		readcnt++;
		dblk = fsbtodb(&sblock, fsck_ino_to_fsba(&sblock, lastinum));
		if (readcnt % readpercg == 0) {
			size = partialsize;
			lastinum += partialcnt;
		} else {
			size = inobufsize;
			lastinum += fullcnt;
		}
		(void)bread(fsreadfd, (char *)inodebuf, dblk, size);
		dp = inodebuf;
	}
	return (dp++);
}

void
resetinodebuf()
{

	startinum = 0;
	nextino = 1;
	lastinum = 1;
	readcnt = 0;
	inobufsize = blkroundup(&sblock, INOBUFSIZE);
	fullcnt = inobufsize / sizeof(struct ext2fs_dinode);
	readpercg = sblock.e2fs.e2fs_ipg / fullcnt;
	partialcnt = sblock.e2fs.e2fs_ipg % fullcnt;
	partialsize = partialcnt * sizeof(struct ext2fs_dinode);
	if (partialcnt != 0) {
		readpercg++;
	} else {
		partialcnt = fullcnt;
		partialsize = inobufsize;
	}
	if (inodebuf == NULL &&
	    (inodebuf = (struct ext2fs_dinode *)malloc((unsigned)inobufsize)) ==
		NULL)
		errexit("Cannot allocate space for inode buffer\n");
	while (nextino < EXT2_ROOTINO)
		(void)getnextinode(nextino);
}

void
freeinodebuf()
{

	if (inodebuf != NULL)
		free((char *)inodebuf);
	inodebuf = NULL;
}

/*
 * Routines to maintain information about directory inodes.
 * This is built during the first pass and used during the
 * second and third passes.
 *
 * Enter inodes into the cache.
 */
void
cacheino(dp, inumber)
	struct ext2fs_dinode *dp;
	ino_t inumber;
{
	struct inoinfo *inp;
	struct inoinfo **inpp;
	unsigned int blks;

	blks = howmany(fs2h32(dp->e2di_size), sblock.e2fs_bsize);
	if (blks > NDADDR)
		blks = NDADDR + NIADDR;
	inp = (struct inoinfo *)
		malloc(sizeof(*inp) + (blks - 1) * sizeof(daddr_t));
	if (inp == NULL)
		return;
	inpp = &inphead[inumber % numdirs];
	inp->i_nexthash = *inpp;
	*inpp = inp;
	inp->i_child = inp->i_sibling = inp->i_parentp = 0;
	if (inumber == EXT2_ROOTINO)
		inp->i_parent = EXT2_ROOTINO;
	else
		inp->i_parent = (ino_t)0;
	inp->i_dotdot = (ino_t)0;
	inp->i_number = inumber;
	inp->i_isize = fs2h32(dp->e2di_size);
	inp->i_numblks = blks * sizeof(daddr_t);
	memcpy(&inp->i_blks[0], &dp->e2di_blocks[0], (size_t)inp->i_numblks);
	if (inplast == listmax) {
		listmax += 100;
		inpsort = (struct inoinfo **)realloc((char *)inpsort,
		    (unsigned)listmax * sizeof(struct inoinfo *));
		if (inpsort == NULL)
			errexit("cannot increase directory list\n");
	}
	inpsort[inplast++] = inp;
}

/*
 * Look up an inode cache structure.
 */
struct inoinfo *
getinoinfo(inumber)
	ino_t inumber;
{
	struct inoinfo *inp;

	for (inp = inphead[inumber % numdirs]; inp; inp = inp->i_nexthash) {
		if (inp->i_number != inumber)
			continue;
		return (inp);
	}
	errexit("cannot find inode %d\n", inumber);
	return ((struct inoinfo *)0);
}

/*
 * Clean up all the inode cache structure.
 */
void
inocleanup()
{
	struct inoinfo **inpp;

	if (inphead == NULL)
		return;
	for (inpp = &inpsort[inplast - 1]; inpp >= inpsort; inpp--)
		free((char *)(*inpp));
	free((char *)inphead);
	free((char *)inpsort);
	inphead = inpsort = NULL;
}

void
inodirty()
{

	dirty(pbp);
}

void
clri(idesc, type, flag)
	struct inodesc *idesc;
	char *type;
	int flag;
{
	struct ext2fs_dinode *dp;

	dp = ginode(idesc->id_number);
	if (flag == 1) {
		pwarn("%s %s", type,
		    (dp->e2di_mode & IFMT) == IFDIR ? "DIR" : "FILE");
		pinode(idesc->id_number);
	}
	if (preen || reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
		n_files--;
		(void)ckinode(dp, idesc);
		clearinode(dp);
		statemap[idesc->id_number] = USTATE;
		inodirty();
	}
}

int
findname(idesc)
	struct inodesc *idesc;
{
	struct ext2fs_direct *dirp = idesc->id_dirp;
	u_int16_t namlen = dirp->e2d_namlen;

	if (fs2h32(dirp->e2d_ino) != idesc->id_parent)
		return (KEEPON);
	memcpy(idesc->id_name, dirp->e2d_name, (size_t)namlen);
	idesc->id_name[namlen] = '\0';
	return (STOP|FOUND);
}

int
findino(idesc)
	struct inodesc *idesc;
{
	struct ext2fs_direct *dirp = idesc->id_dirp;
	u_int32_t ino = fs2h32(dirp->e2d_ino);

	if (ino == 0)
		return (KEEPON);
	if (strcmp(dirp->e2d_name, idesc->id_name) == 0 &&
	    (ino == EXT2_ROOTINO || ino >= EXT2_FIRSTINO)
		&& ino <= maxino) {
		idesc->id_parent = ino;
		return (STOP|FOUND);
	}
	return (KEEPON);
}

void
pinode(ino)
	ino_t ino;
{
	struct ext2fs_dinode *dp;
	char *p;
	struct passwd *pw;
	time_t t;

	printf(" I=%u ", ino);
	if ((ino < EXT2_FIRSTINO && ino != EXT2_ROOTINO) || ino > maxino)
		return;
	dp = ginode(ino);
	printf(" OWNER=");
#ifndef SMALL
	if ((pw = getpwuid((int)dp->e2di_uid)) != 0)
		printf("%s ", pw->pw_name);
	else
#endif
		printf("%u ", (unsigned)fs2h16(dp->e2di_uid));
	printf("MODE=%o\n", fs2h16(dp->e2di_mode));
	if (preen)
		printf("%s: ", cdevname());
	printf("SIZE=%u ", fs2h32(dp->e2di_size));
	t = fs2h32(dp->e2di_mtime);
	p = ctime(&t);
	printf("MTIME=%12.12s %4.4s ", &p[4], &p[20]);
}

void
blkerror(ino, type, blk)
	ino_t ino;
	char *type;
	daddr_t blk;
{

	pfatal("%d %s I=%u", blk, type, ino);
	printf("\n");
	switch (statemap[ino]) {

	case FSTATE:
		statemap[ino] = FCLEAR;
		return;

	case DSTATE:
		statemap[ino] = DCLEAR;
		return;

	case FCLEAR:
	case DCLEAR:
		return;

	default:
		errexit("BAD STATE %d TO BLKERR\n", statemap[ino]);
		/* NOTREACHED */
	}
}

/*
 * allocate an unused inode
 */
ino_t
allocino(request, type)
	ino_t request;
	int type;
{
	ino_t ino;
	struct ext2fs_dinode *dp;
	time_t t;

	if (request == 0)
		request = EXT2_ROOTINO;
	else if (statemap[request] != USTATE)
		return (0);
	for (ino = request; ino < maxino; ino++) {
		if ((ino > EXT2_ROOTINO) && (ino < EXT2_FIRSTINO))
			continue;
		if (statemap[ino] == USTATE)
			break;
	}
	if (ino == maxino)
		return (0);
	switch (type & IFMT) {
	case IFDIR:
		statemap[ino] = DSTATE;
		break;
	case IFREG:
	case IFLNK:
		statemap[ino] = FSTATE;
		break;
	default:
		return (0);
	}
	dp = ginode(ino);
	dp->e2di_blocks[0] = h2fs32(allocblk());
	if (dp->e2di_blocks[0] == 0) {
		statemap[ino] = USTATE;
		return (0);
	}
	dp->e2di_mode = h2fs16(type);
	(void)time(&t);
	dp->e2di_atime = h2fs32(t);
	dp->e2di_mtime = dp->e2di_ctime = dp->e2di_atime;
	dp->e2di_dtime = 0;
	dp->e2di_size = h2fs32(sblock.e2fs_bsize);
	dp->e2di_nblock = h2fs32(btodb(sblock.e2fs_bsize));
	n_files++;
	inodirty();
	typemap[ino] = E2IFTODT(type);
	return (ino);
}

/*
 * deallocate an inode
 */
void
freeino(ino)
	ino_t ino;
{
	struct inodesc idesc;
	struct ext2fs_dinode *dp;

	memset(&idesc, 0, sizeof(struct inodesc));
	idesc.id_type = ADDR;
	idesc.id_func = pass4check;
	idesc.id_number = ino;
	dp = ginode(ino);
	(void)ckinode(dp, &idesc);
	clearinode(dp);
	inodirty();
	statemap[ino] = USTATE;
	n_files--;
}
