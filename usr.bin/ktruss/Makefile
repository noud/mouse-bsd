#	$NetBSD: Makefile,v 1.3 1999/07/28 01:56:06 enami Exp $

PROG=		ktruss
SRCS=		ktrace.c dump.c subr.c ioctl.c misc.c
CPPFLAGS+=	-I. -I${.CURDIR}/../ktrace -I${.CURDIR}/../../sys -DKTRUSS
LDSTATIC?=	-static
CLEANFILES+=	misc.c misc.h ioctl.c
MAN=

dump.c: misc.h
misc.c misc.h: ${DESTDIR}/usr/include/sys/errno.h \
	       ${DESTDIR}/usr/include/sys/signal.h
	${.CURDIR}/makeerrnos.sh \
	    ${DESTDIR}/usr/include/sys/errno.h \
	    ${DESTDIR}/usr/include/sys/signal.h misc

.include "${.CURDIR}/../kdump/Makefile.ioctl-c"
.include <bsd.prog.mk>

.PATH: ${.CURDIR}/../ktrace ${.CURDIR}/../kdump
