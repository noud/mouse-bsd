/*	$NetBSD: tstp.c,v 1.21 2000/06/15 21:20:16 jdc Exp $	*/

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
static char sccsid[] = "@(#)tstp.c	8.3 (Berkeley) 5/4/94";
#else
__RCSID("$NetBSD: tstp.c,v 1.21 2000/06/15 21:20:16 jdc Exp $");
#endif
#endif				/* not lint */

#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#include "curses.h"
#include "curses_private.h"

/*
 * stop_signal_handler --
 *	Handle stop signals.
 */
void
__stop_signal_handler(/*ARGSUSED*/int signo)
{
	sigset_t oset, set;

	/*
	 * Block window change and timer signals.  The latter is because
	 * applications use timers to decide when to repaint the screen.
	 */
	(void) sigemptyset(&set);
	(void) sigaddset(&set, SIGALRM);
	(void) sigaddset(&set, SIGWINCH);
	(void) sigprocmask(SIG_BLOCK, &set, &oset);

	/*
	 * End the window, which also resets the terminal state to the
	 * original modes.
	 */
	__stopwin();

	/* Unblock SIGTSTP. */
	(void) sigemptyset(&set);
	(void) sigaddset(&set, SIGTSTP);
	(void) sigprocmask(SIG_UNBLOCK, &set, NULL);

	/* Stop ourselves. */
	(void) kill(0, SIGTSTP);

	/* Time passes ... */

	/* restart things */
	__restartwin();

	/* Reset the signals. */
	(void) sigprocmask(SIG_SETMASK, &oset, NULL);
}

static void (*otstpfn)
__P((int)) = SIG_DFL;

/*
 * Set the TSTP handler.
 */
void
__set_stophandler(void)
{
	otstpfn = signal(SIGTSTP, __stop_signal_handler);
}

/*
 * Restore the TSTP handler.
 */
void
__restore_stophandler(void)
{
	(void) signal(SIGTSTP, otstpfn);
}


/* To allow both SIGTSTP and endwin() to come back nicely, we provide
   the following routines. */

static struct termios save_termios;

int
__stopwin(void)
{
	/* Get the current terminal state (which the user may have changed). */
	(void) tcgetattr(STDIN_FILENO, &save_termios);

	__restore_stophandler();

	if (curscr != NULL) {
		__unsetattr(0);
		__mvcur((int) curscr->cury, (int) curscr->curx, (int) curscr->maxy - 1, 0, 0);
	}

	if (MO != NULL)
		(void) tputs(MO, 0, __cputchar);

	if (curscr->flags & __KEYPAD)
		(void) tputs(KE, 0, __cputchar);
	(void) tputs(VE, 0, __cputchar);
	(void) tputs(TE, 0, __cputchar);
	(void) fflush(stdout);
	(void) setvbuf(stdout, NULL, _IOLBF, (size_t) 0);

	__endwin = 1;

	return (tcsetattr(STDIN_FILENO, __tcaction ?
	    TCSASOFT | TCSADRAIN : TCSADRAIN, &__orig_termios) ? ERR : OK);
}


void
__restartwin(void)
{
	/* Reset the curses SIGTSTP signal handler. */
	__set_stophandler();

	/* save the new "default" terminal state */
	(void) tcgetattr(STDIN_FILENO, &__orig_termios);

	/* Reset the terminal state to the mode just before we stopped. */
	(void) tcsetattr(STDIN_FILENO, __tcaction ?
	    TCSASOFT | TCSADRAIN : TCSADRAIN, &save_termios);

	/* Restore colours */
	__restore_colors();

	/* Reset meta */
	__restore_meta_state();

	/* Reset cursor visibility */
	__restore_cursor_vis();

	/* Restart the screen. */
	__startwin();

	/* Repaint the screen. */
	wrefresh(curscr);
}

int
def_prog_mode(void)
{
	return (tcgetattr(STDIN_FILENO, &save_termios) ? ERR : OK);
}

int
reset_prog_mode(void)
{
	__restartwin();
	return(OK);
}

int
def_shell_mode(void)
{
	return (tcgetattr(STDIN_FILENO, &__orig_termios) ? ERR : OK);
}

int
reset_shell_mode(void)
{
	return (__stopwin());
}
