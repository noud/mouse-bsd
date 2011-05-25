/*	$NetBSD: font_8x16.c,v 1.2 1995/09/23 20:25:34 leo Exp $	*/

/*
 *  Copyright (c) 1992, 1993, 1994 Hellmuth Michaelis and Joerg Wunsch
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 * 	This product includes software developed by
 *	Hellmuth Michaelis and Joerg Wunsch
 *  4. The name authors may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Translated into compiler and human readable for for the Atari-TT port of
 * NetBSD by Leo Weppelman.
 *
 * Reorganized and edited some chars to fit the iso-8859-1 fontset by
 * Thomas Gerner
 */
#include <atari/dev/font.h>

char fontname_8x16[64] = "vt220iso.816";

extern unsigned char fontdata_8x16[];
font_info	font_info_8x16 = { 1, 8, 16, 14, 0, 255, &fontdata_8x16[0] };

int  fontheight_8x16 = 16;
int  fontwidth_8x16  = 8;

unsigned char fontdata_8x16[] = {
/* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x01 */ 0x00, 0x18, 0x18, 0x3c, 0x3c, 0x7e, 0x7e, 0xff,
           0xff, 0x7e, 0x7e, 0x3c, 0x3c, 0x18, 0x18, 0x00,
/* 0x02 */ 0x42, 0x99, 0x99, 0x42, 0x42, 0x99, 0x99, 0x42,
           0x42, 0x99, 0x99, 0x42, 0x42, 0x99, 0x99, 0x42,
/* 0x03 */ 0x00, 0x00, 0x90, 0x90, 0xf0, 0x90, 0x90, 0x00,
           0x3e, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00,
/* 0x04 */ 0x00, 0x00, 0xf0, 0x80, 0xe0, 0x80, 0x80, 0x00,
           0x1e, 0x10, 0x1c, 0x10, 0x10, 0x00, 0x00, 0x00,
/* 0x05 */ 0x00, 0x00, 0x60, 0x90, 0x80, 0x90, 0x60, 0x00,
           0x1c, 0x12, 0x1c, 0x12, 0x12, 0x00, 0x00, 0x00,
/* 0x06 */ 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0xf0, 0x00,
           0x1e, 0x10, 0x1c, 0x10, 0x10, 0x00, 0x00, 0x00,
/* 0x07 */ 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x08 */ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18,
           0x18, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
/* 0x09 */ 0x00, 0x00, 0x88, 0x98, 0xa8, 0xc8, 0x88, 0x00,
           0x10, 0x10, 0x10, 0x10, 0x1e, 0x00, 0x00, 0x00,
/* 0x0a */ 0x00, 0x00, 0x88, 0x88, 0x50, 0x50, 0x20, 0x00,
           0x3e, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00,
/* 0x0b */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x0c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x0d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x0e */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1f,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x0f */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x10 */ 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x11 */ 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x12 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x13 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
/* 0x14 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
/* 0x15 */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1f,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x16 */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xf8,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x17 */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0xff,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x18 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x19 */ 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
/* 0x1a */ 0x00, 0x00, 0x0c, 0x18, 0x30, 0x60, 0x30, 0x18,
           0x0c, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00,
/* 0x1b */ 0x00, 0x00, 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18,
           0x30, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00,
/* 0x1c */ 0x00, 0x00, 0x00, 0x00, 0xfe, 0x6c, 0x6c, 0x6c,
           0x6c, 0x6c, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00,
/* 0x1d */ 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18,
           0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x1e */ 0x00, 0x38, 0x6c, 0x64, 0x60, 0xf0, 0x60, 0x60,
           0x60, 0x60, 0xe6, 0xfc, 0x00, 0x00, 0x00, 0x00,
/* 0x1f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  ' ' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '!' */ 0x00, 0x00, 0x18, 0x3c, 0x3c, 0x3c, 0x18, 0x18,
           0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/*  '"' */ 0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '#' */ 0x00, 0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c,
           0x6c, 0xfe, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00,
/*  '$' */ 0x00, 0x18, 0x18, 0x7c, 0xc6, 0xc2, 0xc0, 0x7c,
           0x06, 0x86, 0xc6, 0x7c, 0x18, 0x18, 0x00, 0x00,
/*  '%' */ 0x00, 0x00, 0x00, 0x00, 0xc2, 0xc6, 0x0c, 0x18,
           0x30, 0x60, 0xc6, 0x86, 0x00, 0x00, 0x00, 0x00,
/*  '&' */ 0x00, 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x76, 0xdc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/*  ''' */ 0x00, 0x30, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '(' */ 0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30,
           0x30, 0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00,
/*  ')' */ 0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c,
           0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00,
/*  '*' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3c, 0xff,
           0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '+' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e,
           0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  ',' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00,
/*  '-' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '.' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/*  '/' */ 0x00, 0x00, 0x00, 0x00, 0x02, 0x06, 0x0c, 0x18,
           0x30, 0x60, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00,
/*  '0' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xce, 0xde, 0xf6,
           0xe6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  '1' */ 0x00, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x7e, 0x00, 0x00, 0x00, 0x00,
/*  '2' */ 0x00, 0x00, 0x7c, 0xc6, 0x06, 0x0c, 0x18, 0x30,
           0x60, 0xc0, 0xc6, 0xfe, 0x00, 0x00, 0x00, 0x00,
/*  '3' */ 0x00, 0x00, 0x7c, 0xc6, 0x06, 0x06, 0x3c, 0x06,
           0x06, 0x06, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  '4' */ 0x00, 0x00, 0x0c, 0x1c, 0x3c, 0x6c, 0xcc, 0xfe,
           0x0c, 0x0c, 0x0c, 0x1e, 0x00, 0x00, 0x00, 0x00,
/*  '5' */ 0x00, 0x00, 0xfe, 0xc0, 0xc0, 0xc0, 0xfc, 0x06,
           0x06, 0x06, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  '6' */ 0x00, 0x00, 0x38, 0x60, 0xc0, 0xc0, 0xfc, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  '7' */ 0x00, 0x00, 0xfe, 0xc6, 0x06, 0x06, 0x0c, 0x18,
           0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
/*  '8' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7c, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  '9' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0x7e, 0x06,
           0x06, 0x06, 0x0c, 0x78, 0x00, 0x00, 0x00, 0x00,
/*  ':' */ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
           0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  ';' */ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
           0x00, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00,
/*  '<' */ 0x00, 0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60,
           0x30, 0x18, 0x0c, 0x06, 0x00, 0x00, 0x00, 0x00,
/*  '=' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00,
           0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '>' */ 0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06,
           0x0c, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00,
/*  '?' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0x0c, 0x18, 0x18,
           0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/*  '@' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xde, 0xde,
           0xde, 0xdc, 0xc0, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'A' */ 0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6, 0xfe,
           0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'B' */ 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x66,
           0x66, 0x66, 0x66, 0xfc, 0x00, 0x00, 0x00, 0x00,
/*  'C' */ 0x00, 0x00, 0x3c, 0x66, 0xc2, 0xc0, 0xc0, 0xc0,
           0xc0, 0xc2, 0x66, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'D' */ 0x00, 0x00, 0xf8, 0x6c, 0x66, 0x66, 0x66, 0x66,
           0x66, 0x66, 0x6c, 0xf8, 0x00, 0x00, 0x00, 0x00,
/*  'E' */ 0x00, 0x00, 0xfe, 0x66, 0x62, 0x68, 0x78, 0x68,
           0x60, 0x62, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/*  'F' */ 0x00, 0x00, 0xfe, 0x66, 0x62, 0x68, 0x78, 0x68,
           0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00,
/*  'G' */ 0x00, 0x00, 0x3c, 0x66, 0xc2, 0xc0, 0xc0, 0xde,
           0xc6, 0xc6, 0x66, 0x3a, 0x00, 0x00, 0x00, 0x00,
/*  'H' */ 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xfe, 0xc6,
           0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'I' */ 0x00, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'J' */ 0x00, 0x00, 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
           0xcc, 0xcc, 0xcc, 0x78, 0x00, 0x00, 0x00, 0x00,
/*  'K' */ 0x00, 0x00, 0xe6, 0x66, 0x66, 0x6c, 0x78, 0x78,
           0x6c, 0x66, 0x66, 0xe6, 0x00, 0x00, 0x00, 0x00,
/*  'L' */ 0x00, 0x00, 0xf0, 0x60, 0x60, 0x60, 0x60, 0x60,
           0x60, 0x62, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/*  'M' */ 0x00, 0x00, 0xc6, 0xee, 0xfe, 0xfe, 0xd6, 0xc6,
           0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'N' */ 0x00, 0x00, 0xc6, 0xe6, 0xf6, 0xfe, 0xde, 0xce,
           0xc6, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'O' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'P' */ 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x60,
           0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00,
/*  'Q' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xd6, 0xde, 0x7c, 0x0c, 0x0e, 0x00, 0x00,
/*  'R' */ 0x00, 0x00, 0xfc, 0x66, 0x66, 0x66, 0x7c, 0x6c,
           0x66, 0x66, 0x66, 0xe6, 0x00, 0x00, 0x00, 0x00,
/*  'S' */ 0x00, 0x00, 0x7c, 0xc6, 0xc6, 0x60, 0x38, 0x0c,
           0x06, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'T' */ 0x00, 0x00, 0x7e, 0x7e, 0x5a, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'U' */ 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'V' */ 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00,
/*  'W' */ 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xd6, 0xd6,
           0xd6, 0xfe, 0xee, 0x6c, 0x00, 0x00, 0x00, 0x00,
/*  'X' */ 0x00, 0x00, 0xc6, 0xc6, 0x6c, 0x7c, 0x38, 0x38,
           0x7c, 0x6c, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'Y' */ 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'Z' */ 0x00, 0x00, 0xfe, 0xc6, 0x86, 0x0c, 0x18, 0x30,
           0x60, 0xc2, 0xc6, 0xfe, 0x00, 0x00, 0x00, 0x00,
/*  '[' */ 0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30,
           0x30, 0x30, 0x30, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  '\' */ 0x00, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x38,
           0x1c, 0x0e, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00,
/*  ']' */ 0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
           0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  '^' */ 0x10, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  '_' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
/*  '`' */ 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  'a' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/*  'b' */ 0x00, 0x00, 0xe0, 0x60, 0x60, 0x78, 0x6c, 0x66,
           0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'c' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc0,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'd' */ 0x00, 0x00, 0x1c, 0x0c, 0x0c, 0x3c, 0x6c, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/*  'e' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xfe,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'f' */ 0x00, 0x00, 0x38, 0x6c, 0x64, 0x60, 0xf0, 0x60,
           0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00,
/*  'g' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0xcc, 0x78, 0x00,
/*  'h' */ 0x00, 0x00, 0xe0, 0x60, 0x60, 0x6c, 0x76, 0x66,
           0x66, 0x66, 0x66, 0xe6, 0x00, 0x00, 0x00, 0x00,
/*  'i' */ 0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'j' */ 0x00, 0x00, 0x06, 0x06, 0x00, 0x0e, 0x06, 0x06,
           0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3c, 0x00,
/*  'k' */ 0x00, 0x00, 0xe0, 0x60, 0x60, 0x66, 0x6c, 0x78,
           0x78, 0x6c, 0x66, 0xe6, 0x00, 0x00, 0x00, 0x00,
/*  'l' */ 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/*  'm' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0xfe, 0xd6,
           0xd6, 0xd6, 0xd6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'n' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x66, 0x66,
           0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
/*  'o' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  'p' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x66, 0x66,
           0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0, 0x00,
/*  'q' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x7c, 0x0c, 0x0c, 0x1e, 0x00,
/*  'r' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x76, 0x66,
           0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00,
/*  's' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xc6, 0x60,
           0x38, 0x0c, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/*  't' */ 0x00, 0x00, 0x10, 0x30, 0x30, 0xfc, 0x30, 0x30,
           0x30, 0x30, 0x36, 0x1c, 0x00, 0x00, 0x00, 0x00,
/*  'u' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xcc, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/*  'v' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6,
           0xc6, 0x6c, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00,
/*  'w' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xd6,
           0xd6, 0xd6, 0xfe, 0x6c, 0x00, 0x00, 0x00, 0x00,
/*  'x' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x6c, 0x38,
           0x38, 0x38, 0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00,
/*  'y' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x0c, 0xf8, 0x00,
/*  'z' */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xcc, 0x18,
           0x30, 0x60, 0xc6, 0xfe, 0x00, 0x00, 0x00, 0x00,
/*  '{' */ 0x00, 0x00, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x18,
           0x18, 0x18, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00,
/*  '|' */ 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18,
           0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/*  '}' */ 0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x18,
           0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00, 0x00,
/*  '~' */ 0x00, 0x00, 0x76, 0xdc, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x7f */ 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6c, 0xc6,
           0xc6, 0xc6, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x80 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x81 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x82 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x83 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x84 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x85 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x86 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x87 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x88 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x89 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x8f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x90 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x91 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x92 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x93 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x94 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x95 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x96 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x97 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x98 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x99 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9a */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9b */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9c */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9d */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9e */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0x9f */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa1 */ 0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18,
           0x3c, 0x3c, 0x3c, 0x18, 0x00, 0x00, 0x00, 0x00,
/* 0xa2 */ 0x00, 0x18, 0x18, 0x3c, 0x66, 0x60, 0x60, 0x60,
           0x66, 0x3c, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/* 0xa3 */ 0x00, 0x38, 0x6c, 0x64, 0x60, 0xf0, 0x60, 0x60,
           0x60, 0x60, 0xe6, 0xfc, 0x00, 0x00, 0x00, 0x00,
/* 0xa4 */ 0xc3, 0x3c, 0x66, 0x42, 0x66, 0x3c, 0xc3, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa5 */ 0x00, 0x00, 0x66, 0x66, 0x3c, 0x18, 0x7e, 0x18,
           0x7e, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
/* 0xa6 */ 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
           0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
/* 0xa7 */ 0x00, 0x7c, 0xc6, 0x60, 0x38, 0x6c, 0xc6, 0xc6,
           0x6c, 0x38, 0x0c, 0xc6, 0x7c, 0x00, 0x00, 0x00,
/* 0xa8 */ 0x00, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xa9 */ 0x00, 0x7c, 0xc6, 0x82, 0x9a, 0xa6, 0xa2, 0xa6,
           0x9a, 0x82, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xaa */ 0x00, 0x3c, 0x6c, 0x6c, 0x3e, 0x00, 0x7e, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xab */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x6c, 0xd8,
           0x6c, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xac */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x06,
           0x06, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xad */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xae */ 0x00, 0x7c, 0xc6, 0x82, 0xba, 0xa6, 0xba, 0xaa,
           0xa6, 0x82, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xaf */ 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb0 */ 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb1 */ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18,
           0x18, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
/* 0xb2 */ 0x00, 0x70, 0xd8, 0x30, 0x60, 0xc8, 0xf8, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb3 */ 0x00, 0x70, 0xd8, 0x30, 0x30, 0xd8, 0x70, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb4 */ 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb5 */ 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66,
           0x66, 0x7c, 0x60, 0x60, 0xc0, 0x00, 0x00, 0x00,
/* 0xb6 */ 0x00, 0x00, 0x7f, 0xdb, 0xdb, 0xdb, 0x7b, 0x1b,
           0x1b, 0x1b, 0x1b, 0x1b, 0x00, 0x00, 0x00, 0x00,
/* 0xb7 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xb8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
           0x00, 0x00, 0x00, 0x18, 0x30, 0x60, 0x00, 0x00,
/* 0xb9 */ 0x00, 0x30, 0x70, 0xf0, 0x30, 0x30, 0x78, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xba */ 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x00, 0x7c, 0x00,
           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xbb */ 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8, 0x6c, 0x36,
           0x6c, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xbc */ 0x00, 0xc0, 0xc0, 0xc2, 0xc6, 0xcc, 0x18, 0x30,
           0x66, 0xce, 0x9e, 0x3e, 0x06, 0x06, 0x00, 0x00,
/* 0xbd */ 0x00, 0xc0, 0xc0, 0xc2, 0xc6, 0xcc, 0x18, 0x30,
           0x60, 0xdc, 0x86, 0x0c, 0x18, 0x3e, 0x00, 0x00,
/* 0xbe */ 0x00, 0xc0, 0x60, 0xc2, 0x66, 0xcc, 0x18, 0x30,
           0x66, 0xce, 0x9e, 0x3e, 0x06, 0x06, 0x00, 0x00,
/* 0xbf */ 0x00, 0x00, 0x30, 0x30, 0x00, 0x30, 0x30, 0x60,
           0xc0, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xc0 */ 0x18, 0x0c, 0x06, 0x00, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc1 */ 0x18, 0x30, 0x60, 0x00, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc2 */ 0x10, 0x38, 0x6c, 0x00, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc3 */ 0x76, 0xdc, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc4 */ 0xc6, 0xc6, 0x00, 0x10, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc5 */ 0x38, 0x6c, 0x38, 0x00, 0x38, 0x6c, 0xc6, 0xc6,
           0xfe, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xc6 */ 0x00, 0x00, 0x3e, 0x6c, 0xcc, 0xcc, 0xfe, 0xcc,
           0xcc, 0xcc, 0xcc, 0xce, 0x00, 0x00, 0x00, 0x00,
/* 0xc7 */ 0x00, 0x00, 0x3c, 0x66, 0xc2, 0xc0, 0xc0, 0xc0,
           0xc2, 0x66, 0x3c, 0x0c, 0x06, 0x7c, 0x00, 0x00,
/* 0xc8 */ 0x18, 0x0c, 0x06, 0x00, 0xfe, 0x66, 0x60, 0x7c,
           0x60, 0x60, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/* 0xc9 */ 0x18, 0x30, 0x60, 0x00, 0xfe, 0x66, 0x60, 0x7c,
           0x60, 0x60, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/* 0xca */ 0x10, 0x38, 0x6c, 0x00, 0xfe, 0x66, 0x60, 0x7c,
           0x60, 0x60, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/* 0xcb */ 0x00, 0xc6, 0x00, 0xfe, 0x66, 0x60, 0x60, 0x7c,
           0x60, 0x60, 0x66, 0xfe, 0x00, 0x00, 0x00, 0x00,
/* 0xcc */ 0x18, 0x0c, 0x06, 0x00, 0x3c, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xcd */ 0x18, 0x30, 0x60, 0x00, 0x3c, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xce */ 0x10, 0x38, 0x6c, 0x00, 0x3c, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xcf */ 0x00, 0x66, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xd0 */ 0x00, 0x00, 0xf8, 0x6c, 0x66, 0x66, 0xf6, 0x66,
           0x66, 0x66, 0x6c, 0xf8, 0x00, 0x00, 0x00, 0x00,
/* 0xd1 */ 0x76, 0xdc, 0x00, 0xc6, 0xe6, 0xf6, 0xfe, 0xde,
           0xce, 0xc6, 0xc6, 0xc6, 0x00, 0x00, 0x00, 0x00,
/* 0xd2 */ 0x18, 0x0c, 0x06, 0x00, 0x7c, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xd3 */ 0x18, 0x30, 0x60, 0x00, 0x7c, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xd4 */ 0x10, 0x38, 0x6c, 0x00, 0x7c, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xd5 */ 0x76, 0xdc, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xd6 */ 0xc6, 0xc6, 0x00, 0x7c, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xd7 */ 0x00, 0x00, 0x00, 0x00, 0xc6, 0x6c, 0x38, 0x38,
           0x6c, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xd8 */ 0x00, 0x06, 0x7e, 0xce, 0xce, 0xce, 0xd6, 0xd6,
           0xe6, 0xe6, 0xe6, 0xfc, 0xc0, 0x00, 0x00, 0x00,
/* 0xd9 */ 0x18, 0x0c, 0x06, 0x00, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xda */ 0x18, 0x30, 0x60, 0x00, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xdb */ 0x10, 0x38, 0x6c, 0x00, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xdc */ 0xc6, 0xc6, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xdd */ 0x18, 0x30, 0x60, 0x00, 0x66, 0x66, 0x66, 0x3c,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xde */ 0x00, 0xf0, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c,
           0x60, 0x60, 0x60, 0xf0, 0x00, 0x00, 0x00, 0x00,
/* 0xdf */ 0x00, 0x00, 0x78, 0xcc, 0xcc, 0xcc, 0xd8, 0xcc,
           0xc6, 0xc6, 0xc6, 0xcc, 0x00, 0x00, 0x00, 0x00,
/* 0xe0 */ 0x00, 0x60, 0x30, 0x18, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe1 */ 0x00, 0x18, 0x30, 0x60, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe2 */ 0x00, 0x10, 0x38, 0x6c, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe3 */ 0x00, 0x00, 0x76, 0xdc, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe4 */ 0x00, 0x00, 0xcc, 0x00, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe5 */ 0x00, 0x38, 0x6c, 0x38, 0x00, 0x78, 0x0c, 0x7c,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xe6 */ 0x00, 0x00, 0x00, 0x00, 0x6c, 0xfe, 0xb2, 0x32,
           0x7e, 0xd8, 0xd8, 0x6e, 0x00, 0x00, 0x00, 0x00,
/* 0xe7 */ 0x00, 0x00, 0x00, 0x00, 0x3c, 0x66, 0x60, 0x60,
           0x66, 0x3c, 0x0c, 0x06, 0x3c, 0x00, 0x00, 0x00,
/* 0xe8 */ 0x00, 0x60, 0x30, 0x18, 0x00, 0x7c, 0xc6, 0xfe,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xe9 */ 0x00, 0x0c, 0x18, 0x30, 0x00, 0x7c, 0xc6, 0xfe,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xea */ 0x00, 0x10, 0x38, 0x6c, 0x00, 0x7c, 0xc6, 0xfe,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xeb */ 0x00, 0x00, 0xc6, 0x00, 0x00, 0x7c, 0xc6, 0xfe,
           0xc0, 0xc0, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xec */ 0x00, 0x60, 0x30, 0x18, 0x00, 0x38, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xed */ 0x00, 0x0c, 0x18, 0x30, 0x00, 0x38, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xee */ 0x00, 0x18, 0x3c, 0x66, 0x00, 0x38, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xef */ 0x00, 0x00, 0x66, 0x00, 0x00, 0x38, 0x18, 0x18,
           0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00, 0x00,
/* 0xf0 */ 0x00, 0x00, 0x3e, 0x30, 0x18, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf1 */ 0x00, 0x00, 0x76, 0xdc, 0x00, 0xdc, 0x66, 0x66,
           0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
/* 0xf2 */ 0x00, 0x60, 0x30, 0x18, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf3 */ 0x00, 0x18, 0x30, 0x60, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf4 */ 0x00, 0x10, 0x38, 0x6c, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf5 */ 0x00, 0x00, 0x76, 0xdc, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf6 */ 0x00, 0x00, 0xc6, 0x00, 0x00, 0x7c, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7c, 0x00, 0x00, 0x00, 0x00,
/* 0xf7 */ 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x7e,
           0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 0xf8 */ 0x00, 0x00, 0x00, 0x00, 0x06, 0x7e, 0xce, 0xce,
           0xd6, 0xe6, 0xe6, 0xfc, 0xc0, 0x00, 0x00, 0x00,
/* 0xf9 */ 0x00, 0x60, 0x30, 0x18, 0x00, 0xcc, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xfa */ 0x00, 0x18, 0x30, 0x60, 0x00, 0xcc, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xfb */ 0x00, 0x30, 0x78, 0xcc, 0x00, 0xcc, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xfc */ 0x00, 0x00, 0xcc, 0x00, 0x00, 0xcc, 0xcc, 0xcc,
           0xcc, 0xcc, 0xcc, 0x76, 0x00, 0x00, 0x00, 0x00,
/* 0xfd */ 0x00, 0x18, 0x30, 0x60, 0x00, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x0c, 0x78, 0x00,
/* 0xfe */ 0x00, 0x00, 0x00, 0xf0, 0x60, 0x7c, 0x66, 0x66,
           0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0xf0, 0x00,
/* 0xff */ 0x00, 0xc6, 0xc6, 0x00, 0x00, 0xc6, 0xc6, 0xc6,
           0xc6, 0xc6, 0xc6, 0x7e, 0x06, 0x0c, 0x78, 0x00
};
