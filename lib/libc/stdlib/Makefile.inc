#	$NetBSD: Makefile.inc,v 1.44 1999/08/02 03:18:46 lukem Exp $
#	from: @(#)Makefile.inc	8.3 (Berkeley) 2/4/95

# stdlib sources
.PATH: ${ARCHDIR}/stdlib ${.CURDIR}/stdlib

SRCS+=	_rand48.c _strtoq.c _strtouq.c a64l.c abort.c atexit.c atof.c \
	atoi.c atol.c bsearch.c calloc.c drand48.c erand48.c exit.c \
	getenv.c getopt.c getsubopt.c heapsort.c jrand48.c l64a.c \
	lcong48.c lrand48.c malloc.c merge.c mrand48.c multibyte.c \
	nrand48.c putenv.c qabs.c qdiv.c qsort.c radixsort.c rand.c \
	rand_r.c random.c seed48.c setenv.c srand48.c strtod.c \
	strtol.c strtoq.c strtoq.c strtoul.c strtouq.c system.c \
	tdelete.c tfind.c tsearch.c twalk.c \
	_strtoll.c _strtoull.c strtoll.c strtoull.c

# machine-dependent stdlib sources
# m-d Makefile.inc must include sources for:
#       abs() div() labs() ldiv()

.include "${ARCHDIR}/stdlib/Makefile.inc"

MAN+=	a64l.3 abort.3 abs.3 alloca.3 atexit.3 atof.3 atoi.3 atol.3 bsearch.3 \
	div.3 exit.3 getenv.3 getopt.3 getsubopt.3 \
	labs.3 ldiv.3 malloc.3 memory.3 qabs.3 qdiv.3 qsort.3 \
	radixsort.3 rand48.3 rand.3 random.3 strtod.3 \
	strtol.3 strtoul.3 system.3 tsearch.3

MLINKS+=a64l.3 l64a.3
MLINKS+=a64l.3 l64a_r.3
MLINKS+=getenv.3 setenv.3 getenv.3 unsetenv.3 getenv.3 putenv.3
MLINKS+=malloc.3 calloc.3 malloc.3 realloc.3 malloc.3 free.3
MLINKS+=qsort.3 heapsort.3 qsort.3 mergesort.3
MLINKS+=rand.3 rand_r.3
MLINKS+=rand.3 srand.3
MLINKS+=rand48.3 drand48.3 rand48.3 erand48.3 rand48.3 lrand48.3
MLINKS+=rand48.3 mrand48.3 rand48.3 nrand48.3 rand48.3 jrand48.3
MLINKS+=rand48.3 srand48.3 rand48.3 seed48.3 rand48.3 lcong48.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
MLINKS+=strtol.3 strtoq.3 strtol.3 strtoll.3
MLINKS+=strtoul.3 strtouq.3 strtoul.3 strtoull.3
MLINKS+=tsearch.3 tfind.3 tsearch.3 twalk.3 tsearch.3 tdelete.3
