/*	$NetBSD: char.c,v 1.4 1997/11/21 08:35:43 lukem Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
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
static char sccsid[] = "@(#)char.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: char.c,v 1.4 1997/11/21 08:35:43 lukem Exp $");
#endif
#endif /* not lint */

#include "char.h"

char _cmap[] = {
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^@ - ^C */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^D - ^G */
	_C,		_C|_P,		_C,		_C|_U,	/* ^H - ^K */
	_C|_U,		_C,		_C|_U,		_C|_U,	/* ^L - ^O */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^P - ^S */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^T - ^W */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^U - ^[ */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* ^\ - ^_ */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/*   - # */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* $ - ' */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ( - + */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* , - / */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* 0 - 3 */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* 4 - 7 */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* 8 - ; */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* < - ? */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* @ - C */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* D - G */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* H - K */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* L - O */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* P - S */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* T - W */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* X - [ */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* \ - _ */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ` - c */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* d - g */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* h - k */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* l - o */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* p - s */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* t - w */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* x - { */
	_P|_U,		_P|_U,		_P|_U,		_C|_U,	/* | - ^? */

	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \200 - \203 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \204 - \207 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \210 - \213 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \214 - \217 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \220 - \223 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \224 - \227 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \230 - \233 */
	_C|_U,		_C|_U,		_C|_U,		_C|_U,	/* \234 - \237 */

	_C|_U,		_P|_U,		_P|_U,		_P|_U,	/* \240 - £ */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ¤ - § */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ¨ - « */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ¬ - ¯ */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ° - ³ */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ´ - · */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ¸ - » */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ¼ - ¿ */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* À - Ã */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ä - Ç */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* È - Ë */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ì - Ï */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ð - Ó */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ô - × */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ø - Û */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* Ü - ß */

	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* à - ã */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ä - ç */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* è - ë */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ì - ï */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ð - ó */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ô - ÷ */
	_P|_U,		_P|_U,		_P|_U,		_P|_U,	/* ø - û */
	_P|_U,		_P|_U,		_P|_U,		_P|_U	/* ü - ÿ */
};

char *_unctrl[] = {
	"^@",	"^A",	"^B",	"^C",	"^D",	"^E",	"^F",	"^G",
	"^H",	"^I",	"^J",	"^K",	"^L",	"^M",	"^N",	"^O",
	"^P",	"^Q",	"^R",	"^S",	"^T",	"^U",	"^V",	"^W",
	"^X",	"^Y",	"^Z",	"^[",	"^\\",	"^]",	"^^",	"^_",
	" ",	"!",	"\"",	"#",	"$",	"%",	"&",	"'",
	"(",	")",	"*",	"+",	",",	"-",	".",	"/",
	"0",	"1",	"2",	"3",	"4",	"5",	"6",	"7",
	"8",	"9",	":",	";",	"<",	"=",	">",	"?",
	"@",	"A",	"B",	"C",	"D",	"E",	"F",	"G",
	"H",	"I",	"J",	"K",	"L",	"M",	"N",	"O",
	"P",	"Q",	"R",	"S",	"T",	"U",	"V",	"W",
	"X",	"Y",	"Z",	"[",	"\\",	"]",	"^",	"_",
	"`",	"a",	"b",	"c",	"d",	"e",	"f",	"g",
	"h",	"i",	"j",	"k",	"l",	"m",	"n",	"o",
	"p",	"q",	"r",	"s",	"t",	"u",	"v",	"w",
	"x",	"y",	"z",	"{",	"|",	"}",	"~",	"^?",
	"\\200","\\201","\\202","\\203","\\204","\\205","\\206","\\207",
	"\\210","\\211","\\212","\\213","\\214","\\215","\\216","\\217",
	"\\220","\\221","\\222","\\223","\\224","\\225","\\226","\\227",
	"\\230","\\231","\\232","\\233","\\234","\\235","\\236","\\237",
	"\\240","¡",	"¢",	"£",	"¤",	"¥",	"¦",	"§",
	"¨",	"©",	"ª",	"«",	"¬",	"­",	"®",	"¯",
	"°",	"±",	"²",	"³",	"´",	"µ",	"¶",	"·",
	"¸",	"¹",	"º",	"»",	"¼",	"½",	"¾",	"¿",
	"À",	"Á",	"Â",	"Ã",	"Ä",	"Å",	"Æ",	"Ç",
	"È",	"É",	"Ê",	"Ë",	"Ì",	"Í",	"Î",	"Ï",
	"Ð",	"Ñ",	"Ò",	"Ó",	"Ô",	"Õ",	"Ö",	"×",
	"Ø",	"Ù",	"Ú",	"Û",	"Ü",	"Ý",	"Þ",	"ß",
	"à",	"á",	"â",	"ã",	"ä",	"å",	"æ",	"ç",
	"è",	"é",	"ê",	"ë",	"ì",	"í",	"î",	"ï",
	"ð",	"ñ",	"ò",	"ó",	"ô",	"õ",	"ö",	"÷",
	"ø",	"ù",	"ú",	"û",	"ü",	"ý",	"þ",	"ÿ"
};
