/*	$NetBSD: teach.c,v 1.10 1999/08/14 16:29:23 tron Exp $	*/

/*
 * Copyright (c) 1980, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif				/* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)teach.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: teach.c,v 1.10 1999/08/14 16:29:23 tron Exp $");
#endif
#endif				/* not lint */

#include "back.h"
#include "tutor.h"

extern short ospeed;		/* tty output speed for termlib */

const char   *const helpm[] = {
	"\nEnter a space or newline to roll, or",
	"     b   to display the board",
	"     d   to double",
	"     q   to quit\n",
	0
};

const char   *const contin[] = {
	"",
	0
};

int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	int     i;

	/* revoke setgid privileges */
	setregid(getgid(), getgid());

	signal(SIGINT, getout);
	if (tcgetattr(0, &old) == -1)	/* get old tty mode */
		errexit("teachgammon(gtty)");
	noech = old;
	noech.c_lflag &= ~ECHO;
	raw = noech;
	raw.c_lflag &= ~ICANON;	/* set up modes */
	ospeed = cfgetospeed(&old);	/* for termlib */
	tflag = getcaps(getenv("TERM"));
#ifdef V7
	while (*++argv != 0)
#else
	while (*++argv != -1)
#endif
		getarg(&argv);
	if (tflag) {
		noech.c_oflag &= ~(ONLCR | OXTABS);
		raw.c_oflag &= ~(ONLCR | OXTABS);
		clear();
	}
	text(hello);
	text(list);
	i = text(contin);
	if (i == 0)
		i = 2;
	init();
	while (i)
		switch (i) {
		case 1:
			leave();

		case 2:
			if ((i = text(intro1)) != 0)
				break;
			wrboard();
			if ((i = text(intro2)) != 0)
				break;

		case 3:
			if ((i = text(moves)) != 0)
				break;

		case 4:
			if ((i = text(removepiece)) != 0)
				break;

		case 5:
			if ((i = text(hits)) != 0)
				break;

		case 6:
			if ((i = text(endgame)) != 0)
				break;

		case 7:
			if ((i = text(doubl)) != 0)
				break;

		case 8:
			if ((i = text(stragy)) != 0)
				break;

		case 9:
			if ((i = text(prog)) != 0)
				break;

		case 10:
			if ((i = text(lastch)) != 0)
				break;
		}
	tutor();
	/* NOTREACHED */
	return (0);
}

void
leave()
{
	if (tflag)
		clear();
	else
		writec('\n');
	fixtty(&old);
	execl(EXEC, "backgammon", "-n", args[0]?args:0, 0);
	writel("Help! Backgammon program is missing\007!!\n");
	exit(-1);
}
