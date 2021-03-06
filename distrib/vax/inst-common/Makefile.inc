#	$NetBSD: Makefile.inc,v 1.6 1999/05/11 20:47:00 veego Exp $

# TOP is assumed to be defined by Makefile including this one.

CBIN=		instbin
COMMONDIR=	${TOP}/inst-common

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}c
VND_RDEV=	/dev/r${VND}c
IMAGE?=		miniroot-${REV}.fs
BOOTSYS=	bootroot-${REV}.fs
MDEC=		${DESTDIR}/usr/mdec

LISTS=		${COMMONDIR}/list ${.CURDIR}/list
CRUNCHCONF=	${COMMONDIR}/${CBIN}.conf
MTREE=		${COMMONDIR}/mtree.conf

DISKTYPE=	miniroot
BOOTDISKTYPE=	bootroot

all: ${CBIN} bootroot
	dd if=/dev/zero of=${IMAGE} bs=64k count=32
	vnconfig -t ${DISKTYPE} -v -c ${VND_DEV} ${IMAGE}
	newfs -B le -O -m 0 -o space -i 8192 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	mtree -def ${MTREE} -p ${MOUNT_POINT}/ -u
	TOPDIR=${TOP} CURDIR=${.CURDIR} OBJDIR=${.OBJDIR} \
	    TARGDIR=${MOUNT_POINT} sh ${TOP}/runlist.sh ${LISTS}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount -f ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

bootroot: edlabel copy xxboot boot
	dd if=/dev/zero of=${BOOTSYS} bs=64k count=4
	vnconfig -t ${BOOTDISKTYPE} -v -c ${VND_DEV} ${BOOTSYS}
	newfs -B le -O -m 0 -o space -i 8192 ${VND_RDEV} ${BOOTDISKTYPE}
	dd if=xxboot of=/dev/rvnd0a count=16
	mount ${VND_DEV} ${MOUNT_POINT}
	cp edlabel copy boot ${MOUNT_POINT}
	umount -f ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

edlabel copy xxboot boot:
	(cd /sys/arch/vax/boot;make)
	cp /sys/arch/vax/boot/edlabel .
	cp /sys/arch/vax/boot/copy .
	cp /sys/arch/vax/boot/xxboot .
	cp /sys/arch/vax/boot/boot .

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV} ${IMAGE}
	-/bin/rm -f ${IMAGE}

${CBIN}.mk ${CBIN}.cache ${CBIN}.c: ${CRUNCHCONF}
	crunchgen ${CRUNCHCONF}

${CBIN}: ${CBIN}.mk ${CBIN}.cache ${CBIN}.c
	make -f ${CBIN}.mk

clean cleandir distclean:
	/bin/rm -f core ${IMAGE} ${CBIN} ${CBIN}.mk ${CBIN}.cache *.o *.cro *.c

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
