#	$NetBSD: Makefile,v 1.10 2000/02/01 05:02:31 tsutsui Exp $

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC?=		${DESTDIR}/usr/mdec
KERNEL?=	${.CURDIR}/../../../sys/arch/alpha/compile/GENERIC/netbsd
STRIP?=		strip

GZKERNEL=	netbsd
IMAGE=		GENERIC.fs

DISKTYPE=	floppy

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	6144

CLEANFILES+=	${IMAGE} ${GZKERNEL}

all: GENERIC.fs

GENERIC.fs: ${GZKERNEL} ${MDEC}/boot ${MDEC}/bootxx.old
	@echo Creating tar image
	tar cvf ustar.tmp ${MDEC}/boot ${GZKERNEL}
	@echo Creating vn image
	dd if=/dev/zero of=${.TARGET}.tmp count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${.TARGET}.tmp
	dd bs=8k seek=1 if=ustar.tmp of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	${MDEC}/installboot.old -b 17 -v	\
		${MDEC}/boot ${MDEC}/bootxx.old ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}
	mv ${.TARGET}.tmp ${.TARGET}

${GZKERNEL}: ${KERNEL}
	cp ${KERNEL} ${.TARGET}
	${STRIP} ${.TARGET}
	gzip -9 ${.TARGET}
	mv ${.TARGET}.gz ${.TARGET}

unconfig:
	-vnconfig -u ${VND_CDEV}

disk: all
	dd if=GENERIC.fs bs=18k of=/dev/rfd0a

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES} ${GZKERNEL} ${GZKERNEL}.gz *.tmp

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
