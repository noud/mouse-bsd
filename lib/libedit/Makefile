#	$NetBSD: Makefile,v 1.18 1999/07/06 14:10:21 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

LIB=	edit

OSRCS=	chared.c common.c el.c emacs.c fcns.c help.c hist.c key.c map.c \
	parse.c prompt.c read.c refresh.c search.c sig.c term.c tty.c vi.c

MAN=	editline.3 editrc.5

MLINKS=	editline.3 el_init.3 editline.3 el_end.3 editline.3 el_reset.3 \
	editline.3 el_gets.3 editline.3 el_getc.3 editline.3 el_push.3 \
	editline.3 el_parse.3 editline.3 el_set.3 editline.3 el_get.3 \
	editline.3 el_source.3 editline.3 el_resize.3 editline.3 el_line.3 \
	editline.3 el_insertstr.3 editline.3 el_deletestr.3 \
	editline.3 history_init.3 editline.3 history_end.3 editline.3 history.3

# For speed and debugging
#SRCS=   ${OSRCS} tokenizer.c history.c readline.c
# For protection
SRCS=	editline.c tokenizer.c history.c readline.c

SRCS+=	common.h emacs.h fcns.h help.h vi.h

LIBEDITDIR?=${.CURDIR}

INCS= histedit.h readline.h
INCSDIR=/usr/include
LINKS=${INCSDIR}/readline.h ${INCSDIR}/history.h

CLEANFILES+=common.h editline.c emacs.h fcns.c fcns.h help.c help.h vi.h
CPPFLAGS+=-I. -I${LIBEDITDIR}
CPPFLAGS+=-I. -I${.CURDIR}
CPPFLAGS+=#-DDEBUG_TTY -DDEBUG_KEY -DDEBUG_READ -DDEBUG -DDEBUG_REFRESH
CPPFLAGS+=#-DDEBUG_PASTE

AHDR=vi.h emacs.h common.h
ASRC=${LIBEDITDIR}/vi.c ${LIBEDITDIR}/emacs.c ${LIBEDITDIR}/common.c

vi.h: vi.c makelist
	sh ${LIBEDITDIR}/makelist -h ${LIBEDITDIR}/vi.c > ${.TARGET}

emacs.h: emacs.c makelist
	sh ${LIBEDITDIR}/makelist -h ${LIBEDITDIR}/emacs.c > ${.TARGET}

common.h: common.c makelist
	sh ${LIBEDITDIR}/makelist -h ${LIBEDITDIR}/common.c > ${.TARGET}

fcns.h: ${AHDR} makelist
	sh ${LIBEDITDIR}/makelist -fh ${AHDR} > ${.TARGET}

fcns.c: ${AHDR} fcns.h makelist
	sh ${LIBEDITDIR}/makelist -fc ${AHDR} > ${.TARGET}

help.c: ${ASRC} makelist
	sh ${LIBEDITDIR}/makelist -bc ${ASRC} > ${.TARGET}

help.h: ${ASRC} makelist
	sh ${LIBEDITDIR}/makelist -bh ${ASRC} > ${.TARGET}

editline.c: ${OSRCS}
	sh ${LIBEDITDIR}/makelist -e ${.ALLSRC:T} > ${.TARGET}

test.o:	${LIBEDITDIR}/TEST/test.c

test:	libedit.a test.o
	${CC} ${LDFLAGS} ${.ALLSRC} -o ${.TARGET} libedit.a ${LDADD} -ltermcap

# minimal dependency to make "make depend" optional
editline.o editline.po editline.so editline.ln:	\
	common.h emacs.h fcns.c fcns.h help.c help.h vi.h
readline.o readline.po readline.so readline.ln:	\
	common.h emacs.h fcns.h help.h vi.h

.include <bsd.lib.mk>
