#	$NetBSD: Makefile,v 1.8 2000/02/11 16:14:32 hubertf Exp $

MAN=	mbr.8

BINDIR=	/usr/mdec
BINMODE=444

STRIPFLAG=

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF"
LDFLAGS+= -e start
.else
LDFLAGS+= -e _start -N
.endif

.if ${MACHINE} == "i386"
PROG=	mbr
SRCS=	mbr.S

${PROG}: ${OBJS}
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0x600 ${OBJS}
	${STRIP} ${PROG}.tmp
	objcopy -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp
.endif

.include <bsd.prog.mk>
