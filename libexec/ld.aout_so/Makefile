#	$NetBSD: Makefile,v 1.28 1999/03/19 22:06:37 thorpej Exp $

.include <bsd.own.mk>
.if (${OBJECT_FMT} == "a.out")
PROG=	ld.so
SRCS=	mdprologue.S rtld.c malloc.c shlib.c md.c vfprintf.c
CLIB=	${.CURDIR}/../../lib/libc
CLIBOBJ!=cd ${CLIB}; \
	 printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
PICFLAG=-fpic -fno-function-cse
CPPFLAGS+=$(PICFLAG) -DRTLD -DLIBC_SCCS \
	  -I${CLIB}/include -I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR)
.if defined(DEBUG)
CPPFLAGS+=-DDEBUG
.endif
ASFLAGS+=-k
LDFLAGS+=-Bshareable -Bsymbolic -assert nosymbolic -L${CLIBOBJ}
.if defined(DESTDIR)
LDFLAGS+= -nostdlib -L${DESTDIR}/usr/lib
.endif
LDADD+=	-lc_pic
DPADD+=	${LIBC_PIC}

.PATH: ${CLIB}/stdio $(.CURDIR)/arch/$(MACHINE_ARCH)

$(PROG):
	$(LD) -o $(PROG) $(LDFLAGS) $(OBJS) $(LDADD)

.S.o:
	${CPP} ${.IMPSRC} | ${AS} ${ASFLAGS} -o ${.TARGET} -
.endif

MAN= rtld.1
MLINKS=	rtld.1 ld.so.1

.include <bsd.prog.mk>
