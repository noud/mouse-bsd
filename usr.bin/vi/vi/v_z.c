/*	$NetBSD: v_z.c,v 1.7 1998/01/09 08:08:46 perry Exp $	*/

/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1992, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "@(#)v_z.c	10.10 (Berkeley) 5/16/96";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <limits.h>
#include <stdio.h>

#include "../common/common.h"
#include "vi.h"

/*
 * v_z -- [count]z[count][-.+^<CR>]
 *	Move the screen.
 *
 * PUBLIC: int v_z __P((SCR *, VICMD *));
 */
int
v_z(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	recno_t lno;
	u_int value;

	/*
	 * The first count is the line to use.  If the value doesn't
	 * exist, use the last line.
	 */
	if (F_ISSET(vp, VC_C1SET)) {
		lno = vp->count;
		if (!db_exist(sp, lno) && db_last(sp, &lno))
			return (1);
	} else
		lno = vp->m_start.lno;

	/* Set default return cursor line. */
	vp->m_final.lno = lno;
	vp->m_final.cno = vp->m_start.cno;

	/*
	 * The second count is the displayed window size, i.e. the 'z' command
	 * is another way to get artificially small windows.  Note, you can't
	 * grow beyond the size of the window.
	 *
	 * !!!
	 * A window size of 0 was historically allowed, and simply ignored.
	 * This could be much more simply done by modifying the value of the
	 * o_WINDOW option, but that's not how it worked historically.
	 */
	if (F_ISSET(vp, VC_C2SET) && vp->count2 != 0) {
		if (vp->count2 > o_VAL(sp, o_WINDOW))
			vp->count2 = o_VAL(sp, o_WINDOW);
		if (vs_crel(sp, vp->count2))
			return (1);
	}

	switch (vp->character) {
	case '-':		/* Put the line at the bottom. */
		if (vs_sm_fill(sp, lno, P_BOTTOM))
			return (1);
		break;
	case '.':		/* Put the line in the middle. */
		if (vs_sm_fill(sp, lno, P_MIDDLE))
			return (1);
		break;
	case '+':
		/*
		 * If the user specified a line number, put that line at the
		 * top and move the cursor to it.  Otherwise, scroll forward
		 * a screen from the current screen.
		 */
		if (F_ISSET(vp, VC_C1SET)) {
			if (vs_sm_fill(sp, lno, P_TOP))
				return (1);
			if (vs_sm_position(sp, &vp->m_final, 0, P_TOP))
				return (1);
		} else
			if (vs_sm_scroll(sp, &vp->m_final, sp->t_rows, Z_PLUS))
				return (1);
		break;
	case '^':
		/*
		 * If the user specified a line number, put that line at the
		 * bottom, move the cursor to it, and then display the screen
		 * before that one.  Otherwise, scroll backward a screen from
		 * the current screen.
		 *
		 * !!!
		 * Note, we match the off-by-one characteristics of historic
		 * vi, here.
		 */
		if (F_ISSET(vp, VC_C1SET)) {
			if (vs_sm_fill(sp, lno, P_BOTTOM))
				return (1);
			if (vs_sm_position(sp, &vp->m_final, 0, P_TOP))
				return (1);
			if (vs_sm_fill(sp, vp->m_final.lno, P_BOTTOM))
				return (1);
		} else
			if (vs_sm_scroll(sp, &vp->m_final, sp->t_rows, Z_CARAT))
				return (1);
		break;
	default:		/* Put the line at the top for <cr>. */
		value = KEY_VAL(sp, vp->character);
		if (value != K_CR && value != K_NL) {
			v_emsg(sp, vp->kp->usage, VIM_USAGE);
			return (1);
		}
		if (vs_sm_fill(sp, lno, P_TOP))
			return (1);
		break;
	}
	return (0);
}

/*
 * vs_crel --
 *	Change the relative size of the current screen.
 *
 * PUBLIC: int vs_crel __P((SCR *, long));
 */
int
vs_crel(sp, count)
	SCR *sp;
	long count;
{
	sp->t_minrows = sp->t_rows = count;
	if (sp->t_rows > sp->rows - 1)
		sp->t_minrows = sp->t_rows = sp->rows - 1;
	TMAP = HMAP + (sp->t_rows - 1);
	F_SET(sp, SC_SCR_REDRAW);
	return (0);
}
