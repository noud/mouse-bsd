#	$NetBSD: Makefile,v 1.8 1999/02/13 02:54:53 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/kernfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	kernfs

SRCS=	lkminit_vfs.c
SRCS+=	kernfs_vfsops.c kernfs_vnops.c

.include <bsd.kmod.mk>
