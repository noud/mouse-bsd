/*	$NetBSD: printw.c,v 1.15 2000/04/15 13:17:04 blymn Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
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
#if 0
static char sccsid[] = "@(#)printw.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: printw.c,v 1.15 2000/04/15 13:17:04 blymn Exp $");
#endif
#endif				/* not lint */

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "curses.h"
#include "curses_private.h"

/*
 * printw and friends.
 *
 * These routines make nonportable assumptions about varargs if __STDC__
 * is not in effect.
 */

static int __winwrite __P((void *, const char *, int));

/*
 * printw --
 *	Printf on the standard screen.
 */
int
#ifdef __STDC__
printw(const char *fmt,...)
#else
printw(fmt, va_alist)
	const char   *fmt;
va_dcl
#endif
{
	va_list ap;
	int     ret;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = vwprintw(stdscr, fmt, ap);
	va_end(ap);
	return (ret);
}
/*
 * wprintw --
 *	Printf on the given window.
 */
int
#ifdef __STDC__
wprintw(WINDOW *win, const char *fmt,...)
#else
wprintw(win, fmt, va_alist)
	WINDOW *win;
	const char   *fmt;
va_dcl
#endif
{
	va_list ap;
	int     ret;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = vwprintw(win, fmt, ap);
	va_end(ap);
	return (ret);
}
/*
 * mvprintw, mvwprintw --
 *	Implement the mvprintw commands.  Due to the variable number of
 *	arguments, they cannot be macros.  Sigh....
 */
int
#ifdef __STDC__
mvprintw(int y, int x, const char *fmt,...)
#else
mvprintw(y, x, fmt, va_alist)
	int     y, x;
	const char   *fmt;
va_dcl
#endif
{
	va_list ap;
	int     ret;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	if (move(y, x) != OK)
		return (ERR);
	ret = vwprintw(stdscr, fmt, ap);
	va_end(ap);
	return (ret);
}

int
#ifdef __STDC__
mvwprintw(WINDOW * win, int y, int x, const char *fmt,...)
#else
mvwprintw(win, y, x, fmt, va_alist)
	WINDOW *win;
	int     y, x;
	const char   *fmt;
va_dcl
#endif
{
	va_list ap;
	int     ret;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	if (wmove(win, y, x) != OK)
		return (ERR);

	ret = vwprintw(win, fmt, ap);
	va_end(ap);
	return (ret);
}
/*
 * Internal write-buffer-to-window function.
 */
static int
__winwrite(cookie, buf, n)
	void   *cookie;
	const char *buf;
	int     n;
{
	WINDOW *win;
	int     c;

	for (c = n, win = cookie; --c >= 0;)
	{
#ifdef DEBUG
		__CTRACE("__winwrite: %c\n", *buf);
#endif
		if (waddch(win, (chtype) (*buf++ & __CHARTEXT)) == ERR)
			return (-1);
	}
	return (n);
}
/*
 * vwprintw --
 *	This routine actually executes the printf and adds it to the window.
 */
int
vwprintw(WINDOW *win, const char *fmt, va_list ap)
{
	FILE   *f;

	if ((f = funopen(win, NULL, __winwrite, NULL, NULL)) == NULL)
		return (ERR);
	(void) vfprintf(f, fmt, ap);
	return (fclose(f) ? ERR : OK);
}
