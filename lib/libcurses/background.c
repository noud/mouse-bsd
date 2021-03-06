/*	$NetBSD: background.c,v 1.6 2000/04/24 14:09:42 blymn Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: background.c,v 1.6 2000/04/24 14:09:42 blymn Exp $");
#endif				/* not lint */

#include "curses.h"
#include "curses_private.h"

/*
 * bkgdset
 *	Set new background attributes on stdscr.
 */
void
bkgdset(chtype ch)
{
	wbkgdset(stdscr, ch);
}

/*
 * bkgd --
 *	Set new background and new background attributes on stdscr.
 */
int
bkgd(chtype ch)
{
	return(wbkgd(stdscr, ch));
}

/*
 * wbkgdset
 *	Set new background attributes.
 */
void
wbkgdset(WINDOW *win, chtype ch)
{
	if (ch & __CHARTEXT)
		win->bch = (wchar_t) ch & __CHARTEXT;
	win->battr = (attr_t) ch & __ATTRIBUTES;
#ifdef DEBUG
	__CTRACE("wbkgdset: (%0.2o), '%s', %08x\n",
	    win, unctrl(win->bch), win->battr);
#endif
}

/*
 * wbkgd --
 *	Set new background and new background attributes.
 */
int
wbkgd(WINDOW *win, chtype ch)
{
	int	y, x;

	wbkgdset(win, ch);
	for (y = 0; y < win->maxy; y++)
		for (x = 0; x < win->maxx; x++) {
			if (ch & A_CHARTEXT)
				win->lines[y]->line[x].bch = ch & __CHARTEXT;
			win->lines[y]->line[x].battr = ch & __ATTRIBUTES;
		}
	__touchwin(win);
	return(OK);
}

/*
 * getbkgd --
 *	Get current background attributes.
 */
chtype
getbkgd(WINDOW *win)
{
	return ((chtype) ((win->bch & A_CHARTEXT) |
	    (win->battr & A_ATTRIBUTES)));
}
