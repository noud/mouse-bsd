#	$NetBSD: Makefile,v 1.8 2000/01/28 16:01:46 bouyer Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	fsck_ext2fs
MAN=    fsck_ext2fs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ext2fs_bswap.c
FSCK=	${.CURDIR}/../fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${.CURDIR}/../../sys/ufs/ext2fs ${FSCK}

.include <bsd.prog.mk>
