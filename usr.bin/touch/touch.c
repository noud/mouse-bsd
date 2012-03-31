/*	$NetBSD: touch.c,v 1.26 2006/03/18 11:15:00 dsl Exp $	*/

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)touch.c	8.2 (Berkeley) 4/28/95";
#endif
__RCSID("$NetBSD: touch.c,v 1.26 2006/03/18 11:15:00 dsl Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

int	main __P((int, char **));
int	rw __P((const char *, struct stat *, int));
void	stime_arg1 __P((char *, struct timeval *));
void	stime_arg2 __P((char *, int, struct timeval *));
void	stime_file __P((char *, struct timeval *));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat sb;
	struct timeval tv[2];
	int aflag, cflag, fflag, hflag, mflag, ch, fd, len, rval, timeset;
	char *p;
	int (*change_file_times) __P((const char *, const struct timeval *));
	int (*get_file_status) __P((const char *, struct stat *));

	setlocale(LC_ALL, "");

	aflag = cflag = fflag = hflag = mflag = timeset = 0;
	inc = 0;
	if (gettimeofday(&tv[0], NULL))
		err(1, "gettimeofday");

	while ((ch = getopt(argc, argv, "acd:fhmr:t:")) != -1)
		switch(ch) {
		case 'a':
			aflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'd':
			inc = atol(optarg);
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
			hflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'r':
			timeset = 1;
			stime_file(optarg, tv);
			break;
		case 't':
			timeset = 1;
			stime_arg1(optarg, tv);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/* Default is both -a and -m. */
	if (aflag == 0 && mflag == 0)
		aflag = mflag = 1;

	if (hflag) {
		cflag = 1;		/* Don't create new file */
		change_file_times = lutimes;
		get_file_status = lstat;
	} else {
		change_file_times = utimes;
		get_file_status = stat;
	}

	/*
	 * If no -r or -t flag, at least two operands, the first of which
	 * is an 8 or 10 digit number, use the obsolete time specification.
	 */
	if (!timeset && argc > 1) {
		(void)strtol(argv[0], &p, 10);
		len = p - argv[0];
		if (*p == '\0' && (len == 8 || len == 10)) {
			timeset = 1;
			stime_arg2(*argv++, len == 10, tv);
		}
	}

	/* Otherwise use the current time of day. */
	if (!timeset)
		tv[1] = tv[0];

	if (*argv == NULL)
		usage();

	for (rval = 0; *argv; ++argv, tv[0].tv_sec+=inc, tv[1].tv_sec+=inc) {
		/* See if the file exists. */
		if ((*get_file_status)(*argv, &sb)) {
			if (!cflag) {
				/* Create the file. */
				fd = open(*argv,
				    O_WRONLY | O_CREAT, DEFFILEMODE);
				if (fd == -1 || fstat(fd, &sb) || close(fd)) {
					rval = 1;
					warn("%s", *argv);
					continue;
				}

				/* If using the current time, we're done. */
				if (!timeset)
					continue;
			} else
				continue;
		}
		if (!aflag)
			TIMESPEC_TO_TIMEVAL(&tv[0], &sb.st_atimespec);
		if (!mflag)
			TIMESPEC_TO_TIMEVAL(&tv[1], &sb.st_mtimespec);

		/* Try utimes(2). */
		if (!(*change_file_times)(*argv, tv))
			continue;

		/* If the user specified a time, nothing else we can do. */
		if (timeset) {
			rval = 1;
			warn("%s", *argv);
		}

		/*
		 * System V and POSIX 1003.1 require that a NULL argument
		 * set the access/modification times to the current time.
		 * The permission checks are different, too, in that the
		 * ability to write the file is sufficient.  Take a shot.
		 */
		 if (!(*change_file_times)(*argv, NULL))
			continue;

		/* Try reading/writing. */
		if (!S_ISLNK(sb.st_mode) && rw(*argv, &sb, fflag))
			rval = 1;
	}
	exit(rval);
}

#define	ATOI2(s)	((s) += 2, ((s)[-2] - '0') * 10 + ((s)[-1] - '0'))

void
stime_arg1(arg, tvp)
	char *arg;
	struct timeval *tvp;
{
	struct tm *t;
	time_t tmptime;
	int yearset;
	char *p;
					/* Start with the current time. */
	tmptime = tvp[0].tv_sec;
	if ((t = localtime(&tmptime)) == NULL)
		err(1, "localtime");
					/* [[CC]YY]MMDDhhmm[.SS] */
	if ((p = strchr(arg, '.')) == NULL)
		t->tm_sec = 0;		/* Seconds defaults to 0. */
	else {
		if (strlen(p + 1) != 2)
			goto terr;
		*p++ = '\0';
		t->tm_sec = ATOI2(p);
	}
		
	yearset = 0;
	switch (strlen(arg)) {
	case 12:			/* CCYYMMDDhhmm */
		t->tm_year = ATOI2(arg) * 100 - TM_YEAR_BASE;
		yearset = 1;
		/* FALLTHROUGH */
	case 10:			/* YYMMDDhhmm */
		if (yearset) {
			t->tm_year += ATOI2(arg);
		} else {
			yearset = ATOI2(arg);
			if (yearset < 69)
				t->tm_year = yearset + 2000 - TM_YEAR_BASE;
			else
				t->tm_year = yearset + 1900 - TM_YEAR_BASE;
		}
		/* FALLTHROUGH */
	case 8:				/* MMDDhhmm */
		t->tm_mon = ATOI2(arg);
		--t->tm_mon;		/* Convert from 01-12 to 00-11 */
		/* FALLTHROUGH */
	case 6:
		t->tm_mday = ATOI2(arg);
		/* FALLTHROUGH */
	case 4:
		t->tm_hour = ATOI2(arg);
		/* FALLTHROUGH */
	case 2:
		t->tm_min = ATOI2(arg);
		break;
	default:
		goto terr;
	}

	t->tm_isdst = -1;		/* Figure out DST. */
	tvp[0].tv_sec = tvp[1].tv_sec = mktime(t);
	if (tvp[0].tv_sec == -1)
terr:		errx(1,
	"out of range or illegal time specification: [[CC]YY]MMDDhhmm[.SS]");

	tvp[0].tv_usec = tvp[1].tv_usec = 0;
}

void
stime_arg2(arg, year, tvp)
	char *arg;
	int year;
	struct timeval *tvp;
{
	struct tm *t;
	time_t tmptime;
					/* Start with the current time. */
	tmptime = tvp[0].tv_sec;
	if ((t = localtime(&tmptime)) == NULL)
		err(1, "localtime");

	t->tm_mon = ATOI2(arg);		/* MMDDhhmm[yy] */
	--t->tm_mon;			/* Convert from 01-12 to 00-11 */
	t->tm_mday = ATOI2(arg);
	t->tm_hour = ATOI2(arg);
	t->tm_min = ATOI2(arg);
	if (year) {
		year = ATOI2(arg);
		if (year < 69)
			t->tm_year = year + 2000 - TM_YEAR_BASE;
		else
			t->tm_year = year + 1900 - TM_YEAR_BASE;
	}
	t->tm_sec = 0;

	t->tm_isdst = -1;		/* Figure out DST. */
	tvp[0].tv_sec = tvp[1].tv_sec = mktime(t);
	if (tvp[0].tv_sec == -1)
		errx(1,
	"out of range or illegal time specification: MMDDhhmm[yy]");

	tvp[0].tv_usec = tvp[1].tv_usec = 0;
}

void
stime_file(fname, tvp)
	char *fname;
	struct timeval *tvp;
{
	struct stat sb;

	if (stat(fname, &sb))
		err(1, "%s", fname);
	TIMESPEC_TO_TIMEVAL(&tvp[0], &sb.st_atimespec);
	TIMESPEC_TO_TIMEVAL(&tvp[1], &sb.st_mtimespec);
}

int
rw(fname, sbp, force)
	const char *fname;
	struct stat *sbp;
	int force;
{
	int fd, needed_chmod, rval;
	u_char byte;

	/* Try regular files and directories. */
	if (!S_ISREG(sbp->st_mode) && !S_ISDIR(sbp->st_mode)) {
		warnx("%s: %s", fname, strerror(EFTYPE));
		return (1);
	}

	needed_chmod = rval = 0;
	if ((fd = open(fname, O_RDWR, 0)) == -1) {
		if (!force || chmod(fname, DEFFILEMODE))
			goto err;
		if ((fd = open(fname, O_RDWR, 0)) == -1)
			goto err;
		needed_chmod = 1;
	}

	if (sbp->st_size != 0) {
		if (read(fd, &byte, sizeof(byte)) != sizeof(byte))
			goto err;
		if (lseek(fd, (off_t)0, SEEK_SET) == -1)
			goto err;
		if (write(fd, &byte, sizeof(byte)) != sizeof(byte))
			goto err;
	} else {
		if (write(fd, &byte, sizeof(byte)) != sizeof(byte)) {
err:			rval = 1;
			warn("%s", fname);
		} else if (ftruncate(fd, (off_t)0)) {
			rval = 1;
			warn("%s: file modified", fname);
		}
	}

	if (fd >= 0 && close(fd) && rval != 1) {
		rval = 1;
		warn("%s", fname);
	}
	if (needed_chmod && chmod(fname, sbp->st_mode) && rval != 1) {
		rval = 1;
		warn("%s: permissions modified", fname);
	}
	return (rval);
}

__dead void
usage()
{
	(void)fprintf(stderr,
	    "usage: touch [-acfhm] [-r file] [-t time] [-d delta] file ...\n");
	exit(1);
}
