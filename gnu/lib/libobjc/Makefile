#	$NetBSD: Makefile,v 1.7 1999/09/29 13:19:15 takemura Exp $

HAVE_GCC28!= ${CXX} --version | egrep "^(2\.8|egcs)" ; echo

.if (${HAVE_GCC28} != "")

LIB=		objc
SRCS=		hash.c sarray.c class.c sendmsg.c init.c archive.c \
		encoding.c selector.c objects.c misc.c NXConstStr.m \
		Object.m Protocol.m nil_method.c thr.c linking.m \
		thr-single.c
DPSRCS+=	runtime-info.h

INCS=		hash.h objc-list.h sarray.h objc.h objc-api.h NXConstStr.h \
		Object.h Protocol.h encoding.h typedstream.h thr.h
INCSDIR=	/usr/include/objc

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb")
ARCHSUBDIR= mips
.else
ARCHSUBDIR= ${MACHINE_ARCH}
.endif

CPPFLAGS+=	-I. -I${ARCH}/${ARCHSUBDIR} -I${ARCH} \
		-I${DIST}/config -I${DIST}

.if (${MACHINE} == "hpcmips") # XXX
CPPFLAGS+=-D${MACHINE}
.endif

MKLINT=		no

DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

CLEANFILES+=	rtscratch rtscratch.s
runtime-info.h:
	@touch rtscratch
	`${OBJC} --print-prog-name=cc1obj` -print-objc-runtime-info rtscratch >$@

sendmsg.o sendmsg.so sendmsg.po: runtime-info.h

.PATH:		${DIST}/objc

.include <bsd.lib.mk>

.else

# XXX Temporary: during an egcs bootstrap, don't try to build with gcc 2.7
.include <bsd.own.mk>
${TARGETS}:

.endif
