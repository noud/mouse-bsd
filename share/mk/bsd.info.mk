#	$NetBSD: bsd.info.mk,v 1.10 2000/01/22 19:31:01 mycroft Exp $

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif
.include <bsd.own.mk>
.include <bsd.obj.mk>
.include <bsd.depall.mk>
.MAIN:		all
.endif

MAKEINFO?=	makeinfo
INFOFLAGS?=	
INSTALL_INFO?=	install-info

.SUFFIXES: .txi .texi .texinfo .info

.txi.info .texi.info .texinfo.info:
	@${MAKEINFO} ${INFOFLAGS} --no-split -o $@ $<

.if defined(TEXINFO) && !empty(TEXINFO) && ${MKINFO} != "no"
INFOFILES=	${TEXINFO:C/\.te?xi(nfo)?$/.info/}
FILES+=		${INFOFILES}

infoinstall:
.for F in ${INFOFILES}
	@${INSTALL_INFO} --remove --info-dir=${DESTDIR}${INFODIR} ${DESTDIR}${INFODIR}/${F}
	${INSTALL_INFO} --info-dir=${DESTDIR}${INFODIR} ${DESTDIR}${INFODIR}/${F}
.endfor

.for F in ${INFOFILES}
FILESDIR_${F}=	${INFODIR}
FILESOWN_${F}=	${INFOOWN}
FILESGRP_${F}=	${INFOGRP}
FILESMODE_${F}=	${INFOMODE}
FILESNAME_${F}=	${F:T}
.endfor

realall: ${INFOFILES}
.else
realall:
.endif

.if ${MKINFO} != "no"
cleaninfo:
	rm -f ${INFOFILES}
.else
cleaninfo infoinstall:
.endif

.include <bsd.files.mk>

# These need to happen *after* filesinstall.
.PHONY: infoinstall cleaninfo
realinstall: infoinstall
cleandir distclean: cleaninfo

# Make sure all of the standard targets are defined, even if they do nothing.
clean depend includes lint regress tags:
