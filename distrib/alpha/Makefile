#	$NetBSD: Makefile,v 1.4 1999/11/20 13:03:09 sommerfeld Exp $

SUBDIR=	instkernel #floppy-GENERIC

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
CDHDTAPEOBJ != cd instkernel/cd-hd-tape; make print-objdir
FDSETOBJ != cd instkernel/fdset; make print-objdir
INSTKERNOBJ != cd instkernel/instkernel; make print-objdir
GENERICOBJ != cd floppy-GENERIC; make print-objdir

release:
.for i in floppy diskimage instkernel tapeimage
	${IINST} README.files ${ITARGET}/$i/.
.endfor
	${IINST} ${CDHDTAPEOBJ}/cdhdtape ${ITARGET}/diskimage/.
	${IINST} ${FDSETOBJ}/disk1of2 ${ITARGET}/floppy/.
	${IINST} ${FDSETOBJ}/disk2of2 ${ITARGET}/floppy/.
	${IINST} ${INSTKERNOBJ}/netbsd.gz ${ITARGET}/instkernel/.
	#${IINST} ${GENERICOBJ}/GENERIC.fs ${ITARGET}/misc/.
.endif

.include <bsd.subdir.mk>
