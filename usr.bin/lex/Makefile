#	from: @(#)Makefile	5.4 (Berkeley) 6/24/90
#	$NetBSD: Makefile,v 1.19 1998/04/09 00:32:37 tv Exp $
#
# By default, flex will be configured to generate 8-bit scanners only if the
# -8 flag is given.  If you want it to always generate 8-bit scanners, add
# "-DDEFAULT_CSIZE=256" to CPPFLAGS.  Note that doing so will double the size
# of all uncompressed scanners.
#
# If on your system you have trouble building flex due to 8-bit character
# problems, remove the -8 from FLEX_FLAGS and the "#define FLEX_8_BIT_CHARS"
# from the beginning of flexdef.h.
#
# To bootstrap lex, cp initscan.c to scan.c and run make.

PROG=	lex
CPPFLAGS+=-I. -I${.CURDIR}
SRCS=	ccl.c dfa.c ecs.c gen.c main.c misc.c nfa.c parse.y sym.c tblcmp.c \
	yylex.c
OBJS+=	scan.o skel.o
YHEADER=1
CLEANFILES+=scan.c skel.c
LDADD=	-ll
DPADD=	${LIBL}

INCS	=FlexLexer.h
INCSDIR=/usr/include/g++

MAN = flex.1

LINKS=	${BINDIR}/lex ${BINDIR}/flex \
	${BINDIR}/lex ${BINDIR}/flex++
MLINKS=	flex.1 lex.1

skel.c:	flex.skl mkskel.sh
	sh ${.CURDIR}/mkskel.sh ${.CURDIR}/flex.skl > skel.c

scan.c: scan.l
	flex -t -p ${.CURDIR}/scan.l > scan.c

scan.o yylex.o: parse.h

.include <bsd.prog.mk>
