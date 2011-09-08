/*	$NetBSD: getyx.c,v 1.3 2000/04/24 14:09:43 blymn Exp $	*/

/*
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
__RCSID("$NetBSD: getyx.c,v 1.3 2000/04/24 14:09:43 blymn Exp $");
#endif				/* not lint */

#include <stdlib.h>

#include "curses.h"
#include "curses_private.h"

/*
 * getcury --
 *	Get current y position on window.
 */
int
getcury(WINDOW *win)
{
	return(win->cury);
}

/*
 * getcurx --
 *	Get current x position on window.
 */
int
getcurx(WINDOW *win)
{
	return(win->curx);
}

/*
 * getbegy --
 *	Get begin y position on window.
 */
int
getbegy(WINDOW *win)
{
	return(win->begy);
}

/*
 * getbegx --
 *	Get begin x position on window.
 */
int
getbegx(WINDOW *win)
{
	return(win->begx);
}

/*
 * getmaxy --
 *	Get maximum y position on window.
 */
int
getmaxy(WINDOW *win)
{
	return(win->maxy);
}

/*
 * getmaxx --
 *	Get maximum x position on window.
 */
int
getmaxx(WINDOW *win)
{
	return(win->maxx);
}
