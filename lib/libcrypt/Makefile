#	$NetBSD: Makefile,v 1.8 1999/07/20 09:35:18 mrg Exp $
#
# This directory contains an implementation of crypt(3) and associated
# routines.  They have been modified so that decryption is no longer possible,
# and thus are exportable.  They were taken from the 4.4BSD-Lite foreign
# distribution.

SRCTOP=	../..
.include <bsd.crypto.mk>

LIB=	crypt

SRCS=	crypt.c realcrypt.c md5crypt.c

MAN=	crypt.3
MLINKS= crypt.3 encrypt.3 crypt.3 setkey.3 crypt.3 crypt_makesalt.3 crypt.3 des_crypt.3 crypt.3 md5_crypt.3

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/lib/libcrypt/Makefile.frag"
.endif

.include <bsd.lib.mk>
