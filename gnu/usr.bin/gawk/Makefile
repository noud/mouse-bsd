#	$NetBSD: Makefile,v 1.10 1999/06/13 22:21:58 tron Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/gawk ${DIST}/gawk/doc

PROG=		awk
SRCS=		alloca.c array.c awk.y builtin.c dfa.c eval.c field.c \
		gawkmisc.c getopt.c getopt1.c io.c main.c missing.c msg.c \
		node.c random.c re.c regex.c version.c
MLINKS=		awk.1

TEXINFO=	awk.info

CPPFLAGS+=	-DHAVE_CONFIG_H -DDEFPATH=\".:/usr/share/awk\" \
		-I${.CURDIR} -I${.CURDIR}/../../dist/gawk
LDADD=		-lm
CLEANFILES+=	${PROG}.1 ${PROG}.texi

${PROG}.1:	gawk.1
		@-rm -f $@
		cp $> $@

${PROG}.texi:	gawk.texi
		@-rm -f $@
		sed -e 's/gawk.info/awk.info/g' <$> >$@

.include <bsd.info.mk>
.include <bsd.prog.mk>
