#	$NetBSD: Makefile,v 1.7 2000/01/23 21:05:02 eeh Exp $

CURDIR=	${.CURDIR}
S=	${CURDIR}/../../../..

#
# Override normal settings
#

PROG=	bootblk
SRCS=		bootblk.fth
OBJS=
CLEANFILES=	assym.fth.h machine bootblk bootblk.text

NOMAN=
STRIPFLAG=

# deal with Solaris vs. NetBSD build environments for now ..
OS!=uname -s
.if (${OS} == "NetBSD")
USE_GENASSYM?=	no
SVR4=
.else
USE_GENASSYM?=	yes
SVR4=-U__SVR4 -U__svr4__ -D__NetBSD__
.endif

INCLUDES=	-I. -I$S/arch -I$S -nostdinc
CPPFLAGS=	${INCLUDES} ${IDENT} ${PARAM} -D_LKM ${SVR4}

#all:: bootblk.text bootblk

#clean::
#	rm assym.fth.h bootblk.text machine bootblk

machine:
	ln -s ../../include machine

assym.fth.h: ./genassym.sh genfth.cf machine
	sh ./genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
	    < genfth.cf > assym.fth.h

bootblk.text: bootblk.fth assym.fth.h
	awk '/fload/ { print "#include \"" $$2 "\"" }; !/fload/' bootblk.fth | \
	/usr/bin/cpp -P >bootblk.text

bootblk: bootblk.fth assym.fth.h
	fgen -o bootblk bootblk.fth

#
# The following are if you grab the fakeboot program from the Sun website
#

fake: bootblk bootblk.text
	../fakeboot/fakeboot -elf32 <bootblk >/bootblk
	../fakeboot/fakeboot -elf32 <bootblk.text >/bootblk.text

.include <bsd.prog.mk>
