#	$NetBSD: Makefile,v 1.3 1998/01/09 06:55:15 perry Exp $
#
#	@(#)Makefile	8.1 (Berkeley) 6/8/93

DIR=	psd/21.ipc
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t
MACROS=	-ms

paper.ps: ${SRCS}
	${SOELIM} -I${.CURDIR} ${.ALLSRC} | ${TBL} | ${ROFF} > ${.TARGET}

.include <bsd.doc.mk>
