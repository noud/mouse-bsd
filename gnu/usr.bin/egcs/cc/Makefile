#	from: @{#}Makefile    6.2 (Berkeley) 3/25/91
#	$NetBSD: Makefile,v 1.4 1998/08/16 23:10:27 tv Exp $

PROG=           gcc
SRCS=		gcc.c prefix.c version.c choose-temp.c pexecute.c mkstemp.c \
		obstack.c
BINDIR=		/usr/bin

LINKS+=		${BINDIR}/gcc ${BINDIR}/cc

MAN=		gcc.1
MLINKS=		gcc.1 cc.1

.include <bsd.prog.mk>
