#	$NetBSD: Makefile,v 1.5 2000/02/16 12:45:17 tsutsui Exp $

MKMAN=no
PROG=dotest
SRCS=dotest.c dotestsubs.S
AFLAGS+=-I.
CPPFLAGS+=-I.
SPDIR=${.CURDIR}/../../../../../sys/arch/m68k/060sp
CLEANFILES+=itest.S ftest.S

.SUFFIXES: .S .sa

.sa.S: ; sh ${SPDIR}/asm2gas ${.IMPSRC} >${.TARGET}

.PATH: ${SPDIR}

realdepend dotestsubs.o: itest.S ftest.S

regress: ${PROG}
	@/usr/sbin/sysctl hw.model | grep -v -q 68060 || \
		./${PROG} | tee /dev/tty | cmp -s - ${.CURDIR}/goodoutput


.include <bsd.prog.mk>
