#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.17 2000/01/31 14:28:17 itojun Exp $

PROG=	inetd
SRCS=	inetd.c
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CPPFLAGS+=-DLIBWRAP -DINET6
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CPPFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap -lutil
DPADD+= ${LIBWRAP} ${LIBUTIL}

# This is necessary due to be friendly with past RFC2553 API changes...
CPPFLAGS+=-Dss_family=__ss_family -Dss_len=__ss_len

CFLAGS+=-DIPSEC
SRCS+=	ipsec.c
LDADD+= -lipsec
DPADD+=	${LIBIPSEC}

.include <bsd.prog.mk>
