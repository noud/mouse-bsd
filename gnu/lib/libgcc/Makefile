#	$NetBSD: Makefile,v 1.9 1999/09/29 13:19:14 takemura Exp $

HAVE_GCC28!=	${CXX} --version | egrep "^(2\.8|egcs)" ; echo

LIB=		gcc
MKLINT=		no
MKPROFILE=	no
MKPIC=		no
DIST=		${.CURDIR}/../../dist/gcc
ARCH=		${.CURDIR}/../../usr.bin/egcs/arch

.if (${MACHINE_ARCH} == "mipsel") || (${MACHINE_ARCH} == "mipseb")
ARCHSUBDIR= mips
.else
ARCHSUBDIR= ${MACHINE_ARCH}
.endif

CPPFLAGS+=	-I${ARCH}/${ARCHSUBDIR} -I${ARCH} \
		-I${DIST}/config -I${DIST}/cp -I${DIST}

.if (${MACHINE} == "hpcmips") # XXX
CPPFLAGS+=-D${MACHINE}
.endif

LIB2OBJS=	__gcc_bcmp.o _varargs.o __dummy.o _eprintf.o _bb.o _shtab.o \
		_clear_cache.o _trampoline.o __main.o _exit.o _ctors.o \
		_pure.o
.if (${MACHINE_ARCH} == "m68k") || (${MACHINE_ARCH} == "i386") || \
    (${MACHINE_ARCH} == "sparc64")
LIB2OBJS+=	_fixxfdi.o _fixunsxfdi.o _floatdixf.o _fixunsxfsi.o
.endif
.if (${MACHINE_ARCH} == "powerpc")
OBJS+=		tramp.o
tramp.o: ${DIST}/config/rs6000/tramp.asm
	ln -s ${DIST}/config/rs6000/tramp.asm tramp.S
	${COMPILE.c} -I${DIST}/ginclude tramp.S -o tramp.o
	rm -f tramp.S
.endif
OBJS+=		${LIB2OBJS}

# Extra code to include in libgcc2 for C++ support.
.if !empty(HAVE_GCC28)
SRCS+=		tinfo.cc tinfo2.cc new.cc exception.cc frame.c
OBJS+=		${NEW1OBJS} ${NEW2OBJS}
CXXFLAGS+=	-I${DIST}/cp/inc
LIB2OBJS+=	_eh.o
NEW1OBJS=	_op_new.o _op_newnt.o
NEW2OBJS=	_op_vnew.o _op_vnewnt.o _op_delete.o _op_delnt.o \
		_op_vdel.o _op_vdelnt.o

INCS=		exception new new.h typeinfo
INCSDIR=	/usr/include/g++

.cc:
	@true

${NEW1OBJS}: new1.cc
	${COMPILE.cc} -DL$* -o $@ $>
${NEW2OBJS}: new2.cc
	${COMPILE.cc} -DL$* -o $@ $>
.endif

${LIB2OBJS}: libgcc2.c
	${COMPILE.c} -DL$* -o $@ $>

afterinstall:
	@if [ ! -f _eh.o ]; then\
		echo '***** You should rebuild and reinstall libgcc.a using the new egcs'; \
		echo '***** compiler in order to include all the necessary C++ support.'; \
	fi

.PATH:		${DIST} ${DIST}/cp ${DIST}/cp/inc

.include <bsd.lib.mk>
