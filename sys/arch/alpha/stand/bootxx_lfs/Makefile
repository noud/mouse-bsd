# $NetBSD: Makefile,v 1.1 1999/08/18 20:02:39 cgd Exp $

PROG = bootxx_lfs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=lfs \
	    -DBOOTXX_FS_NAME='"LFS"' \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDLFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
