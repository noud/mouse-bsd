#	$NetBSD: Makefile,v 1.2 1999/08/30 17:06:33 mycroft Exp $

PROG=	ldexp
MKMAN=	no

CLEANFILES+=output

regress: ${PROG}
	@./${PROG} >output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi

.include <bsd.prog.mk>
