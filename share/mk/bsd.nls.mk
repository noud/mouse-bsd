#	$NetBSD: bsd.nls.mk,v 1.20 2000/01/22 19:31:02 mycroft Exp $

.if !target(__initialized__)
__initialized__:
.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.MAIN:		all
.endif
.PHONY:		cleannls nlsinstall
.if ${MKNLS} != "no"
realinstall:	nlsinstall
.endif
cleandir distclean: cleannls

.SUFFIXES: .cat .msg

.msg.cat:
	@rm -f ${.TARGET}
	gencat ${.TARGET} ${.IMPSRC}

.if defined(NLS) && !empty(NLS)
NLSALL= ${NLS:.msg=.cat}
.endif

.if !defined(NLSNAME)
.if defined(PROG)
NLSNAME=${PROG}
.else
NLSNAME=lib${LIB}
.endif
.endif

.if defined(NLSALL)
.if ${MKNLS} != "no"
realall: ${NLSALL}
.endif

cleannls:
	rm -f ${NLSALL}

.for F in ${NLSALL}
nlsinstall:: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
.endif
.if !defined(BUILD) && !make(all) && !make(${F})
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: .MADE
.endif

.PRECIOUS: ${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat
${DESTDIR}${NLSDIR}/${F:T:R}/${NLSNAME}.cat: ${F}
	${INSTALL} ${INSTPRIV} -d -o ${NLSOWN} -g ${NLSGRP} ${.TARGET:H}
	${INSTALL} ${RENAME} ${PRESERVE} ${COPY} ${INSTPRIV} -o ${NLSOWN} \
	    -g ${NLSGRP} -m ${NLSMODE} ${.ALLSRC} ${.TARGET}
.endfor
.else
cleannls:
.endif

.if !target(nlsinstall)
nlsinstall::
.endif
