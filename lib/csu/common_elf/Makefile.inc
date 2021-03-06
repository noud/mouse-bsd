#	$NetBSD: Makefile.inc,v 1.4 2000/01/22 19:37:58 mycroft Exp $

.if !defined(ELFSIZE)
ELFSIZE=32
.endif

CPPFLAGS+=	-DLIBC_SCCS -DPIC -DDYNAMIC -DELFSIZE=${ELFSIZE}
CPPFLAGS+=	-I${.CURDIR}/../../../libexec/ld.elf_so
CPPFLAGS+=	-I${.CURDIR}/../common_elf
COPTS+=		-fPIC

.PATH:		${.CURDIR}/../common_elf

OBJS=		crt0.o gcrt0.o crtbegin.o crtend.o

CLEANFILES+=	core a.out

realall: ${OBJS}

crt0.o: crt0.c
	@echo "${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

gcrt0.o: crt0.c
	@echo "${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtbegin.o: crtbegin.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

crtend.o: crtend.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o

FILES=${OBJS}
FILESDIR=/usr/lib

LINKS=	${LIBDIR}/crtbegin.o ${LIBDIR}/crtbeginS.o \
	${LIBDIR}/crtend.o ${LIBDIR}/crtendS.o

.include <bsd.prog.mk>
