#	$NetBSD: Makefile,v 1.17 1999/09/12 01:19:24 chs Exp $

PROG =	rpc.rstatd
.PATH: ${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
SRCS =	dkstats.c rstatd.c rstat_proc.c
MAN =	rpc.rstatd.8
MLINKS=	rpc.rstatd.8 rstatd.8

DPADD=	${LIBRPCSVC} ${LIBKVM}
LDADD=	-lrpcsvc -lkvm

.include <bsd.prog.mk>
