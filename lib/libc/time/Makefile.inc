#	$NetBSD: Makefile.inc,v 1.7 1999/05/04 15:34:50 kleink Exp $

.PATH: ${.CURDIR}/time

SRCS+=	asctime.c difftime.c localtime.c strftime.c strptime.c
MAN+=	ctime.3 time2posix.3 tzfile.5 tzset.3 strftime.3 strptime.3
CFLAGS+=-DALL_STATE

MLINKS+=ctime.3 ctime_r.3	\
	ctime.3 asctime.3	\
	ctime.3 asctime_r.3	\
	ctime.3 difftime.3	\
	ctime.3 gmtime.3	\
	ctime.3 gmtime_r.3	\
	ctime.3 localtime.3	\
	ctime.3 localtime_r.3	\
	ctime.3 mktime.3	\
	tzset.3 tzsetwall.3
