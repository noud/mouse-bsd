#	$NetBSD: Makefile,v 1.1 1999/12/09 14:59:01 tsutsui Exp $

TOP=		${.CURDIR}/..

.include "${TOP}/Makefile.inc"
IMAGE=		ramdisk.fs

CBIN=		ramdiskbin

MOUNT_POINT?=	/mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=		vnd1
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
MDEC=		${DESTDIR}/usr/mdec

LISTS=		list
CRUNCHCONF=	${CBIN}.conf
MTREE=		mtree.conf

SIZE=		2048k
GEOM=		512/16/2/128
DISKTYPE=	floppyinstfs

all: ${CBIN}
	dd if=/dev/zero of=${IMAGE} bs=${SIZE} count=1
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B be -m 0 -o space -i 4096 -c 80 ${VND_RDEV}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${TOP}/../../.. -L ${DESTDIR}/usr/lib ${.ALLSRC}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c #libhack.o
	make -f ${CBIN}.mk all

#HACKSRC=${TOP}/../../utils/libhack
#.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	rm -f ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache
	rm -f *.core *.o *.cro *.c

.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
