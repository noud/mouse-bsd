#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$NetBSD: Makefile.inc,v 1.49 1999/09/08 22:56:56 lukem Exp $

# string sources
.PATH: ${ARCHDIR}/string ${.CURDIR}/string

SRCS+=	bm.c strcasecmp.c strcoll.c strdup.c strerror.c strlcat.c strlcpy.c \
	strmode.c strsignal.c strtok.c strtok_r.c strxfrm.c \
	__strerror.c __strsignal.c

# machine-dependent net sources
# m-d Makefile.inc must include sources for:
#	bcmp() bcopy() bzero() ffs() index() memchr() memcmp() memset()
#	rindex() strcat() strcmp() strcpy() strcspn() strlen()
#	strncat() strncmp() strncpy() strpbrk() strsep()
#	strspn() strstr() swav()
# m-d Makefile.inc may include sources for:
#	memcpy() memmove() strchr() strrchr()

.include "${ARCHDIR}/string/Makefile.inc"

# if no m-d versions of memccpy(), memcpy(), memmove(), strchr(), or strrchr()
# then use the machine independent versions.
#
.if empty(SRCS:Mmemccpy.S)
SRCS+=	memccpy.c
.endif
.if empty(SRCS:Mmemcpy.S)
SRCS+=	memcpy.c
.endif
.if empty(SRCS:Mmemmove.S)
SRCS+=	memmove.c
.endif
.if empty(SRCS:Mstrchr.S)
SRCS+=	strchr.c
.endif
.if empty(SRCS:Mstrrchr.S)
SRCS+=	strrchr.c
.endif

MAN+=	bm.3 bcmp.3 bcopy.3 bstring.3 bzero.3 ffs.3 index.3 memccpy.3 memchr.3 \
	memcmp.3 memcpy.3 memmove.3 memset.3 rindex.3 strcasecmp.3 strcat.3 \
	strchr.3 strcmp.3 strcoll.3 strcpy.3 strcspn.3 strdup.3 strerror.3 \
	string.3 strlcpy.3 strlen.3 strmode.3 strpbrk.3 strrchr.3 strsep.3 \
	strsignal.3 strspn.3 strstr.3 strtok.3 strxfrm.3 swab.3

MLINKS+=bm.3 bm_comp.3 bm.3 bm_exec.3 bm.3 bm_free.3
MLINKS+=strcasecmp.3 strncasecmp.3
MLINKS+=strcat.3 strncat.3
MLINKS+=strcmp.3 strncmp.3
MLINKS+=strcpy.3 strncpy.3
MLINKS+=strlcpy.3 strlcat.3
MLINKS+=strtok.3 strtok_r.3
