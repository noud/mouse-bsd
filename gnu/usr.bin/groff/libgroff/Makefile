#	$NetBSD: Makefile,v 1.12 1999/02/13 02:54:27 lukem Exp $

LIB=		groff
SRCS=		assert.cc change_lf.cc cmap.cc cset.cc device.cc errarg.cc\
		error.cc fatal.cc filename.cc font.cc fontfile.cc lf.cc\
		lineno.cc macropath.cc nametoindex.cc new.cc prime.cc\
	 	progname.cc ptable.cc searchpath.cc string.cc strsave.cc\
		tmpfile.cc illegal.cc version.cc
SRCS+=		iftoa.c itoa.c matherr.c
CPPFLAGS+=	-I$(.CURDIR)/../include

MKMAN=		no
MKPROFILE=	no
MKPIC=		no

CLEANFILES+=	version.cc
version.cc: $(.CURDIR)/../VERSION
	@echo Making version.cc
	@echo const char \*version_string = \"`cat $(.CURDIR)/../VERSION`\"\; >$@

# only needed during build
libinstall::

.include "../Makefile.cfg"
.include <bsd.lib.mk>
