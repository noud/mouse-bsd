# $NetBSD: Makefile,v 1.1 1999/05/11 19:14:50 drochner Exp $

PROG= extest
NOMAN=

.PATH: $(.CURDIR)/../../../../sys/kern
SRCS= extest.c subr_extent.c

CPPFLAGS+= -D_EXTENT_TESTING -D__POOL_EXPOSE

regress: $(PROG)
	$(.OBJDIR)/$(PROG) >extest.out
	tail +4 $(.CURDIR)/extest.exp | diff - extest.out

extest.c: extest.awk tests
	awk -f $(.CURDIR)/extest.awk <$(.CURDIR)/tests >extest.c

CLEANFILES+= extest.c extest.out

.include <bsd.prog.mk>
