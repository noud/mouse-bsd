#	$NetBSD: Makefile,v 1.22 2000/01/08 23:12:36 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	systat

.PATH:	${.CURDIR}/../../usr.bin/vmstat

CPPFLAGS+=-I${.CURDIR}/../../usr.bin/vmstat
CWARNFLAGS+=    -Wno-format-y2k
SRCS=	bufcache.c cmds.c cmdtab.c disks.c dkstats.c fetch.c globalcmds.c \
	icmp.c iostat.c ip.c keyboard.c main.c mbufs.c netcmds.c netstat.c \
	pigs.c ps.c swap.c tcp.c vmstat.c
DPADD=	${LIBCURSES} ${LIBM} ${LIBKVM}
LDADD=	-lcurses -lm -lkvm
BINGRP=	kmem
BINMODE=2555
LINKS=	${BINDIR}/systat ${BINDIR}/sysstat
MLINKS+=systat.1 sysstat.1

CPPFLAGS+=-DINET6 -DIPSEC
SRCS+=	ip6.c ipsec.c

.include <bsd.prog.mk>
