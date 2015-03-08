/*	$NetBSD: allow.c,v 1.4 1997/10/10 08:59:41 lukem Exp $	*/

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
#if 0
static char sccsid[] = "@(#)allow.c	8.1 (Berkeley) 5/31/93";
#else
__RCSID("$NetBSD: allow.c,v 1.4 1997/10/10 08:59:41 lukem Exp $");
#endif
#endif /* not lint */

#include "back.h"

static int movallow_(void)
{
	int     i, m, iold;
	int     r;

	if (d0)
		swap;
	m = (D0 == D1 ? 4 : 2);
	for (i = 0; i < 4; i++)
		p[i] = bar;
	i = iold = 0;
	while (i < m) {
		if (*offptr == 15)
			break;
		h[i] = 0;
		if (board[bar]) {
			if (i == 1 || m == 4)
				g[i] = bar + cturn * D1;
			else
				g[i] = bar + cturn * D0;
			if ((r = makmove(i)) != 0) {
				if (d0 || m == 4)
					break;
				swap;
				movback(i);
				if (i > iold)
					iold = i;
				for (i = 0; i < 4; i++)
					p[i] = bar;
				i = 0;
			} else
				i++;
			continue;
		}
		if ((p[i] += cturn) == home) {
			if (i > iold)
				iold = i;
			if (m == 2 && i) {
				movback(i);
				p[i--] = bar;
				if (p[i] != bar)
					continue;
				else
					break;
			}
			if (d0 || m == 4)
				break;
			swap;
			movback(i);
			for (i = 0; i < 4; i++)
				p[i] = bar;
			i = 0;
			continue;
		}
		if (i == 1 || m == 4)
			g[i] = p[i] + cturn * D1;
		else
			g[i] = p[i] + cturn * D0;
		if (g[i] * cturn > home) {
			if (*offptr >= 0)
				g[i] = home;
			else
				continue;
		}
		if (board[p[i]] * cturn > 0 && (r = makmove(i)) == 0)
			i++;
	}
	movback(i);
	return (iold > i ? iold : i);
}

int movallow(void)
{
 int a1;
 int w1;
 int a2;
 int w2;

 a1 = movallow_();
 w1 = (*offptr == 15);
 if (! d0) swap;
 d0 = 0;
 a2 = movallow_();
 w2 = (*offptr == 15);
 if (! d0) swap;
 d0 = 0;
 if (w1 && w2 && (((a1 == 1) && (a2 == 2)) || ((a1 == 2) && (a2 == 1)))) return(1);
 return((a1>a2)?a1:a2);
}
