/*	$NetBSD: _setjmp.S,v 1.6 1999/06/27 11:10:18 pk Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <machine/asm.h>
#include <machine/trap.h>

#if defined(LIBC_SCCS) && !defined(lint)
RCSID("$NetBSD: _setjmp.S,v 1.6 1999/06/27 11:10:18 pk Exp $")
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v?v:1)" from
 * the last call to
 *	_setjmp(a)
 * The previous signal state is NOT restored.
 */

ENTRY(_setjmp)
	st	%sp, [%o0+0]	/* store caller's stack pointer */
	st	%o7, [%o0+4]	/* and the return pc */
	retl
	 clr	%o0		! return 0

ENTRY(_longjmp)
	sub	%sp, 64, %sp	! set up a local stack frame
	tst	%o1		! compute v ? v : 1
	be,a	0f
	 mov	1, %o1
0:
	t	ST_FLUSHWIN	! flush register windows out to the stack

	/*
	 * We restore the saved stack pointer to %fp, then issue
	 * a `restore' instruction which will reload the register
	 * window from the stack.
	 */
	ld	[%o0+4], %o7	/* restore return pc */
	ld	[%o0+0], %fp	/* and stack pointer */

	retl			! success, return %g6
	 restore	%o1, 0, %o0
