#	$NetBSD: Makefile,v 1.4 1999/07/24 09:09:10 mrg Exp $

.include <bsd.own.mk>

SUBDIR=		common info install-info makeinfo texi2dvi texindex

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/misc
FILES=		epsf.tex texinfo.tex
.endif

.if ${MKNLS} != "no"
.for F in de.gmo fr.gmo
FILES+=		${F}
FILESDIR_${F}=  /usr/share/locale/${F:.gmo=}/LC_MESSAGES
FILESNAME_${F}= texinfo.mo
.endfor
.endif

TEXINFO=	info-stnd.texi info.texi texinfo.txi

DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/texinfo ${DIST}/texinfo/doc ${DIST}/texinfo/po

all-info all-install-info all-makeinfo all-texi2dvi all-texindex: all-common

.include <bsd.info.mk>
.include <bsd.subdir.mk>
