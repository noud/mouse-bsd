#	$NetBSD: Makefile,v 1.5 1999/05/11 20:46:59 veego Exp $

TOP=		${.CURDIR}/..

# This include just sets REV=XX
.include "${TOP}/Makefile.inc"

IMAGE=		ramdisk-${REV}.fs
CBIN=		rd_bin

TREE=		mtree.conf
LISTS=		list
CRUNCHCONF=	${CBIN}.conf

KERNEL=		${SRCSYSDIR}/arch/mvme68k/compile/RAMDISK/netbsd

MOUNT_POINT?=   /mnt
# DEV/RDEV file system device, CDEV/RDEV vnconfig device
VND?=           vnd0
VND_CDEV=       /dev/${VND}c
VND_CRDEV=      /dev/r${VND}c

# These are all the parameters for the root fs: (384K)
# NOTE: If you change NBLKS, you *must* also edit the
#       rdroot entry in /etc/disktab
DISKTYPE=	rdroot
NBLKS=		768

CLEANFILES=	netbsd-rd ${IMAGE}

netbsd-rd:	${IMAGE}
	cp ${KERNEL} netbsd-tmp
	mdsetimage netbsd-tmp ${IMAGE}
	-mv -f netbsd-tmp $@

${IMAGE}:	${TREE} ${LISTS} ${CBIN}
	dd if=/dev/zero of=${IMAGE} count=${NBLKS}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	newfs -B be -m 0 -o space -i 2048 ${VND_CRDEV} ${DISKTYPE}
	mount ${VND_CDEV} ${MOUNT_POINT}
	mtree -def ${.CURDIR}/${TREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${.CURDIR}/runlist.sh ${.CURDIR}/${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE}

# Do not delete this if I change my mind and kill make...
.PRECIOUS: ${IMAGE}

CLEANFILES+= ${CBIN} ${CBIN}.c ${CBIN}.cache ${CBIN}.mk ${CBIN}.syms
CLEANFILES+= *.cro *_stub.o *_stub.c

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen -D ${BSDSRCDIR} -L ${DESTDIR}/usr/lib ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk all

# This is listed in rd_bin.conf but is built here.
${CBIN} : libhack.o

# Use stubs to eliminate some large stuff from libc
HACKSRC=${TOP}/../utils/libhack
.include "${HACKSRC}/Makefile.inc"

clean cleandir distclean:
	-rm -f a.out core *.core *.o
	-rm -f ${CLEANFILES}

# Standard rules needed by the above...
.include <bsd.obj.mk>
