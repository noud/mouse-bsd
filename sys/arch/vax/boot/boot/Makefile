#	$NetBSD: Makefile,v 1.4 1999/05/23 21:58:19 ragge Exp $

S=	${.CURDIR}/../../../../

PROG=	boot
NOMAN=	1
DEVS=	hp.c ctu.c ra.c tmscp.c mfm.c if_qe.c if_le.c if_ze.c
SRCS=	srt0.s boot.c devopen.c conf.c autoconf.c netio.c rom.c romread.s \
	urem.s udiv.s consio.c str.s ${DEVS} findcpu.c

CLEANFILES+=${PROG} ${PROG}.mop
CPPFLAGS+=-DSUPPORT_BOOTPARAMS -DSUPPORT_BOOTP -DSUPPORT_DHCP -D_STANDALONE
#CPPFLAGS+=-DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DDEV_DEBUG \
#	-DRPC_DEBUG -DRARP_DEBUG -DPARANOID
BINDIR=	/

SA_AS=	library
SAMISCMAKEFLAGS=SA_USE_CREAD=yes SA_INCLUDE_NET=yes
.include "${S}/lib/libsa/Makefile.inc"
LIBSA=	${SALIB}

Z_AS=	library
.include "${S}/lib/libz/Makefile.inc"
LIBZ=	${ZLIB}

KERN_AS=library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN=${KERNLIB}

${PROG}: ${OBJS} ${LIBSA} ${LIBZ} ${LIBKERN}
	ld -N -Ttext ${RELOC} -e nisse -o ${PROG} ${OBJS} \
	    ${LIBSA} ${LIBZ} ${LIBSA} ${LIBKERN}
	/usr/sbin/mopa.out ${PROG} ${PROG}.mop
	strip ${PROG}
	size ${PROG}

.include <bsd.prog.mk>

cleandir distclean: cleanlibdir

cleanlibdir:
	rm -rf lib
