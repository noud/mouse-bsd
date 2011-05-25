/*	$NetBSD: printf.c,v 1.21 1998/12/19 20:21:44 christos Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#if !defined(BUILTIN) && !defined(SHELL)
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif
#endif

#ifndef lint
#if 0
static char sccsid[] = "@(#)printf.c	8.2 (Berkeley) 3/22/95";
#else
__RCSID("$NetBSD: printf.c,v 1.21 1998/12/19 20:21:44 christos Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

static int	 print_escape_str __P((const char *));
static size_t	 print_escape __P((const char *));

static int	 getchr __P((void));
static double	 getdouble __P((void));
static int	 getint __P((void));
static long	 getlong __P((void));
static unsigned long getulong __P ((void));
static char	*getstr __P((void));
static char	*mklong __P((const char *, int));
static void      check_conversion __P((const char *, const char *));
static void	 usage __P((void));

static int	rval;
static char  **gargv;

#ifdef BUILTIN
int progprintf __P((int, char **));
#else
int main __P((int, char **));
#endif

#define isodigit(c)	((c) >= '0' && (c) <= '7')
#define octtobin(c)	((c) - '0')
#define hextobin(c)	((c) >= 'A' && (c) <= 'F' ? c - 'A' + 10 : (c) >= 'a' && (c) <= 'f' ? c - 'a' + 10 : c - '0')

#ifdef SHELL
#define main printfcmd
#include "../../bin/sh/bltin/bltin.h"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <vararg.h>
#endif

static void warnx __P((const char *fmt, ...));

static void
#ifdef __STDC__
warnx(const char *fmt, ...)
#else
warnx(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{

	char buf[64];
	va_list ap;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vsprintf(buf, fmt, ap);
	va_end(ap);

	error(buf);
}
#endif /* SHELL */

#define PF(f, func) { \
	if (fieldwidth) { \
		if (precision) \
			(void)printf(f, fieldwidth, precision, func); \
		else \
			(void)printf(f, fieldwidth, func); \
	} else if (precision) \
		(void)printf(f, precision, func); \
	else \
		(void)printf(f, func); \
}

int
#ifdef BUILTIN
progprintf(argc, argv)
#else
main(argc, argv)
#endif
	int argc;
	char *argv[];
{
	char *fmt, *start;
	int fieldwidth, precision;
	char convch, nextch;
	char *format;
	int ch;

#if !defined(SHELL) && !defined(BUILTIN)
	(void)setlocale (LC_ALL, "");
#endif

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		case '?':
		default:
			usage();
			return (1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		return (1);
	}

	format = *argv;
	gargv = ++argv;

#define SKIP1	"#-+ 0"
#define SKIP2	"*0123456789"
	do {
		/*
		 * Basic algorithm is to scan the format string for conversion
		 * specifications -- once one is found, find out if the field
		 * width or precision is a '*'; if it is, gather up value.
		 * Note, format strings are reused as necessary to use up the
		 * provided arguments, arguments of zero/null string are
		 * provided to use up the format string.
		 */

		/* find next format specification */
		for (fmt = format; *fmt; fmt++) {
			switch (*fmt) {
			case '%':
				start = fmt++;

				if (*fmt == '%') {
					(void)putchar('%');
					break;
				} else if (*fmt == 'b') {
					char *p = getstr();
					if (print_escape_str(p)) {
						return (rval);
					}
					break;
				}

				/* skip to field width */
				for (; strchr(SKIP1, *fmt); ++fmt) ;
				fieldwidth = *fmt == '*' ? getint() : 0;

				/* skip to possible '.', get following precision */
				for (; strchr(SKIP2, *fmt); ++fmt) ;
				if (*fmt == '.')
					++fmt;
				precision = *fmt == '*' ? getint() : 0;

				for (; strchr(SKIP2, *fmt); ++fmt) ;
				if (!*fmt) {
					warnx ("missing format character");
					return(1);
				}

				convch = *fmt;
				nextch = *(fmt + 1);
				*(fmt + 1) = '\0';
				switch(convch) {
				case 'c': {
					char p = getchr();
					PF(start, p);
					break;
				}
				case 's': {
					char *p = getstr();
					PF(start, p);
					break;
				}
				case 'd':
				case 'i': {
					char *f = mklong(start, convch);
					long p = getlong();
					PF(f, p);
					break;
				}
				case 'o':
				case 'u':
				case 'x':
				case 'X': {
					char *f = mklong(start, convch);
					unsigned long p = getulong();
					PF(f, p);
					break;
				}
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G': {
					double p = getdouble();
					PF(start, p);
					break;
				}
				default:
					warnx ("%s: invalid directive", start);
					return(1);
				}
				*(fmt + 1) = nextch;
				break;

			case '\\':
				fmt += print_escape(fmt);
				break;

			default:
				(void)putchar(*fmt);
				break;
			}
		}
	} while (gargv > argv && *gargv);

	return (rval);
}


/*
 * Print SysV echo(1) style escape string
 *	Halts processing string and returns 1 if a \c escape is encountered.
 */
static int
print_escape_str(str)
	const char *str;
{
	int value;
	int c;

	while (*str) {
		if (*str == '\\') {
			str++;
			/*
			 * %b string octal constants are not like those in C.
			 * They start with a \0, and are followed by 0, 1, 2,
			 * or 3 octal digits.
			 */
			if (*str == '0') {
				str++;
				for (c = 3, value = 0; c-- && isodigit(*str); str++) {
					value <<= 3;
					value += octtobin(*str);
				}
				(void)putchar(value);
				str--;
			} else if (*str == 'c') {
				return 1;
			} else {
				str--;
				str += print_escape(str);
			}
		} else {
			(void)putchar(*str);
		}
		str++;
	}

	return 0;
}

/*
 * Print "standard" escape characters
 */
static size_t
print_escape(str)
	const char *str;
{
	const char *start = str;
	int value;
	int c;

	str++;

	switch (*str) {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		for (c = 3, value = 0; c-- && isodigit(*str); str++) {
			value <<= 3;
			value += octtobin(*str);
		}
		(void)putchar(value);
		return str - start - 1;
		/* NOTREACHED */

	case 'x':
		str++;
		for (value = 0; isxdigit((unsigned char)*str); str++) {
			value <<= 4;
			value += hextobin(*str);
		}
		if (value > UCHAR_MAX) {
			warnx ("escape sequence out of range for character");
			rval = 1;
		}
		(void)putchar (value);
		return str - start - 1;
		/* NOTREACHED */

	case '\\':			/* backslash */
		(void)putchar('\\');
		break;

	case '\'':			/* single quote */
		(void)putchar('\'');
		break;

	case '"':			/* double quote */
		(void)putchar('"');
		break;

	case 'a':			/* alert */
#ifdef __STDC__
		(void)putchar('\a');
#else
		(void)putchar(007);
#endif
		break;

	case 'b':			/* backspace */
		(void)putchar('\b');
		break;

	case 'e':			/* escape */
#ifdef __GNUC__
		(void)putchar('\e');
#else
		(void)putchar(033);
#endif
		break;

	case 'f':			/* form-feed */
		(void)putchar('\f');
		break;

	case 'n':			/* newline */
		(void)putchar('\n');
		break;

	case 'r':			/* carriage-return */
		(void)putchar('\r');
		break;

	case 't':			/* tab */
		(void)putchar('\t');
		break;

	case 'v':			/* vertical-tab */
		(void)putchar('\v');
		break;

	default:
		(void)putchar(*str);
		warnx("unknown escape sequence `\\%c'", *str);
		rval = 1;
		break;
	}

	return 1;
}

static char *
mklong(str, ch)
	const char *str;
	char ch;
{
	static char copy[64];
	size_t len;

	len = strlen(str) + 2;
	(void)memmove(copy, str, len - 3);
	copy[len - 3] = 'l';
	copy[len - 2] = ch;
	copy[len - 1] = '\0';
	return (copy);
}

static int
getchr()
{
	if (!*gargv)
		return ('\0');
	return ((int)**gargv++);
}

static char *
getstr()
{
	if (!*gargv)
		return ("");
	return (*gargv++);
}

static char *Number = "+-.0123456789";
static int
getint()
{
	if (!*gargv)
		return(0);

	if (strchr(Number, **gargv))
		return(atoi(*gargv++));

	return 0;
}

static long
getlong()
{
	long val;
	char *ep;

	if (!*gargv)
		return(0L);

	if (**gargv == '\"' || **gargv == '\'')
		return (long) *((*gargv++)+1);

	errno = 0;
	val = strtol (*gargv, &ep, 0);
	check_conversion(*gargv++, ep);
	return val;
}

static unsigned long
getulong()
{
	unsigned long val;
	char *ep;

	if (!*gargv)
		return(0UL);

	if (**gargv == '\"' || **gargv == '\'')
		return (unsigned long) *((*gargv++)+1);

	errno = 0;
	val = strtoul (*gargv, &ep, 0);
	check_conversion(*gargv++, ep);
	return val;
}

static double
getdouble()
{
	double val;
	char *ep;

	if (!*gargv)
		return(0.0);

	if (**gargv == '\"' || **gargv == '\'')
		return (double) *((*gargv++)+1);

	errno = 0;
	val = strtod (*gargv, &ep);
	check_conversion(*gargv++, ep);
	return val;
}

static void
check_conversion(s, ep)
	const char *s;
	const char *ep;
{
	if (*ep) {
		if (ep == s)
			warnx ("%s: expected numeric value", s);
		else
			warnx ("%s: not completely converted", s);
		rval = 1;
	} else if (errno == ERANGE) {
		warnx ("%s: %s", s, strerror(ERANGE));
		rval = 1;
	}
}

static void
usage()
{
	(void)fprintf(stderr, "usage: printf format [arg ...]\n");
}
