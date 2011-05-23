#	$NetBSD: Makefile,v 1.4 1999/11/20 19:48:06 veego Exp $

.include "../Makefile.inc"
.PATH:	${BIND_DIST_DIR}/bin/nsupdate \
	${BIND_DIST_DIR}/man

BINDIR=	/usr/bin
PROG=	nsupdate
SRCS=	${PROG}.c
CPPFLAGS+= ${INCLUDE}
LDADD=	${LIBRARY}
MAN=	${PROG}.8

.include "../../Makefile.inc"
.include <bsd.prog.mk>
