#	$NetBSD: Makefile,v 1.9 2000/02/01 05:02:32 tsutsui Exp $

RAMDISK!=cd ${.CURDIR}/../ramdisk; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/ramdisk.fs\n" | \
	${MAKE} -s -f-

KERNEL?=${.CURDIR}/../../../../sys/arch/alpha/compile/INSTALL/netbsd
STRIP?=		strip		# make STRIP=echo can be useful

all:
	cp ${KERNEL} netbsd
	mdsetimage -v netbsd ${RAMDISK}
	${STRIP} netbsd
	gzip -9f netbsd

clean cleandir distclean:
	rm -f netbsd netbsd.gz

# nothing to do here
depend includes lint regress tags:

.include <bsd.own.mk>
.include <bsd.obj.mk>
