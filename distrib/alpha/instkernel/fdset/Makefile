#	$NetBSD: Makefile,v 1.4 1999/05/11 20:08:01 veego Exp $

IMAGE1=		disk1of2
IMAGE2=		disk2of2

INSTKERNEL!=cd ${.CURDIR}/../instkernel; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/netbsd.gz\n" | \
	${MAKE} -s -f-

MOUNT_POINT?=	/mnt
VND?=		vnd0
VND_DEV=	/dev/${VND}a
VND_RDEV=	/dev/r${VND}a
VND_CDEV=	/dev/${VND}c
VND_CRDEV=	/dev/r${VND}c
KERN?=		netbsd-xxx
MDEC=		${DESTDIR}/usr/mdec
FSTMP=		ustar.tmp

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx.old
SECONDARYBOOT=	${DESTDIR}/usr/mdec/boot

DISKTYPE=	floppy

CLEANFILES+=	netbsd ${FSTMP} ${IMAGE1} ${IMAGE2} tmp tmp.gz

all:	netbsd
	@echo Making disk number one.
	tar cvf ${FSTMP} ${SECONDARYBOOT} netbsd
	@ echo ''
	@ls -l ${FSTMP} | (read mode links uid gid size junk;	\
		bytes=$$((2880 * 512 * 2 - $$size - 8 * 1024));		\
		echo There are $$bytes \($$(($$bytes / 1024))K\) bytes free\
			on disk 2.)
	@ echo ''
	dd if=/dev/zero of=${IMAGE1} count=2880
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE1}
	dd bs=8k seek=1 count=179 if=${FSTMP} of=${VND_CDEV}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	@echo "installing new bootblocks"
	${MDEC}/installboot.old -b 17 -v	\
		${SECONDARYBOOT} ${PRIMARYBOOT} ${VND_CRDEV}
	vnconfig -u ${VND_CDEV}
	@echo Making disk number two.
	echo USTARFS 2 > ${IMAGE2}
	(echo 'Prototype image created on: ';	\
		echo -n '     '; date;		\
		echo -n '     '; date -u) >> ${IMAGE2}
	(echo Hash value of corresponding volume:; md5 ${IMAGE1}) >> ${IMAGE2}
	dd bs=8k seek=1 skip=179 conv=sync if=${FSTMP} of=${IMAGE2}
	@rm ${FSTMP}

netbsd: ${INSTKERNEL}
	cp ${INSTKERNEL} netbsd

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_CDEV}
	-/bin/rm -f ${IMAGE1} ${IMAGE2}

clean cleandir distclean:
	/bin/rm -f *.core ${CLEANFILES}

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
