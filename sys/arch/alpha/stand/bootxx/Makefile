# $NetBSD: Makefile,v 1.22 1999/04/06 18:43:18 cgd Exp $

PROG = bootxx.old

SRCS = start.S bootxx.c prom.c prom_disp.S putstr.c booted_dev.c

BOOT_RELOC = ${PRIMARY_LOAD_ADDRESS}
PRIMARY_MAX_LOAD!=	expr 8192 - 512

CPPFLAGS += ${PRIMARY_CPPFLAGS}

CLEANFILES+= ${PROG}.sym ${PROG}.trunc

${PROG}: ${PROG}.sym
	@echo creating ${PROG} from ${PROG}.sym...
	@objcopy --output-target=binary ${PROG}.sym ${PROG}.trunc
	@chmod 644 ${PROG}.trunc
	@dd if=${PROG}.trunc of=${PROG} obs=`expr 15 \* 512` conv=osync
	@ls -l ${PROG}
	@${CHECKSIZE_CMD} ${PROG}.sym ${PROG} ${PRIMARY_MAX_LOAD} \
	    ${PRIMARY_MAX_TOTAL} || (rm -f ${PROG} ; false)

SAMISCMAKEFLAGS= SA_INCLUDE_NET=no SA_USE_CREAD=no
.include "../Makefile.bootprogs"

NETBSD_VERS!=sh ${.CURDIR}/../../../../conf/osrelease.sh
CPPFLAGS+= -DNETBSD_VERS='"${NETBSD_VERS}"'

${PROG}.sym: ${OBJS} ${LIBKERN}
	${LD} -Ttext ${BOOT_RELOC} -N -e start -o ${PROG}.sym ${OBJS} \
	    ${LIBKERN}
	@chmod 644 ${PROG}.sym
	@${SIZE} ${PROG}.sym
