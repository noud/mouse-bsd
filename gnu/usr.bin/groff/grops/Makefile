#	$NetBSD: Makefile,v 1.9 1997/10/25 10:11:49 lukem Exp $

PROG=		grops
SRCS=		ps.cc psrm.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
