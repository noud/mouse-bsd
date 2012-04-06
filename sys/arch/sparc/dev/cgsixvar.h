/*	$NetBSD: cgsixvar.h,v 1.1 1998/03/31 21:05:04 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

/*
 * color display (cgsix) driver; common definitions.
 */

union cursor_cmap {		/* colormap, like bt_cmap, but tiny */
	u_char	cm_map[2][3];	/* 2 R/G/B entries */
	u_int	cm_chip[2];	/* 2 chip equivalents */
};

struct cg6_cursor {		/* cg6 hardware cursor status */
	short	cc_enable;		/* cursor is enabled */
	struct	fbcurpos cc_pos;	/* position */
	struct	fbcurpos cc_hot;	/* hot-spot */
	struct	fbcurpos cc_size;	/* size of mask & image fields */
	u_int	cc_bits[2][32];		/* space for mask & image bits */
	union	cursor_cmap cc_color;	/* cursor colormap */
};

struct cg6_fbc {		/* cg6 hardware acceleration goop */
	/* Most of this we don't care about; here are only the portions
	   we need, most notably the blitter.  Comments are merely my
	   best guesses as to register functions, based largely on the
	   X11R6.4 sunGX code.  Some of these are here only so we can
	   stuff canned values in them (eg, offx). */
	unsigned int cf_pad1[2];
	unsigned int cf_clip;		/* function unknown */
	unsigned int cf_pad2[1];
	unsigned int cf_s;		/* global status */
	unsigned int cf_draw;		/* drawing pipeline status */
	unsigned int cf_blit;		/* blitter status */
	unsigned int cf_pad3[25];
	unsigned int cf_x0;		/* blitter, src llx */
	unsigned int cf_y0;		/* blitter, src lly */
	unsigned int cf_pad4[2];
	unsigned int cf_x1;		/* blitter, src urx */
	unsigned int cf_y1;		/* blitter, src ury */
	unsigned int cf_pad5[2];
	unsigned int cf_x2;		/* blitter, dst llx */
	unsigned int cf_y2;		/* blitter, dst lly */
	unsigned int cf_pad6[2];
	unsigned int cf_x3;		/* blitter, dst urx */
	unsigned int cf_y3;		/* blitter, dst ury */
	unsigned int cf_pad7[2];
	unsigned int cf_offx;		/* x offset for drawing */
	unsigned int cf_offy;		/* y offset for drawing */
	unsigned int cf_pad8[6];
	unsigned int cf_clipminx;	/* clip rectangle llx */
	unsigned int cf_clipminy;	/* clip rectangle lly */
	unsigned int cf_pad9[2];
	unsigned int cf_clipmaxx;	/* clip rectangle urx */
	unsigned int cf_clipmaxy;	/* clip rectangle ury */
	unsigned int cf_pad10[2];
	unsigned int cf_fg;		/* fg value for rop */
	unsigned int cf_pad11[1];
	unsigned int cf_alu;		/* operation to be performed */
	unsigned int cf_pad12[509];
	unsigned int cf_arectx;		/* rectangle drawing, x coord */
	unsigned int cf_arecty;		/* rectangle drawing, y coord */
	/* actually much more, but nothing more we need */
};

/* per-display variables */
struct cgsix_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
	bus_space_tag_t	sc_bustag;
	bus_type_t	sc_btype;	/* phys address description */
	bus_addr_t	sc_paddr;	/* for device mmap() */

	volatile struct bt_regs *sc_bt;		/* Brooktree registers */
	volatile int *sc_fhc;			/* FHC register */
	volatile struct cg6_thc *sc_thc;	/* THC registers */
	volatile struct cg6_tec_xxx *sc_tec;	/* TEC registers */
	volatile struct cg6_fbc *sc_fbc;	/* FBC registers */
	short	sc_fhcrev;		/* hardware rev */
	short	sc_blanked;		/* true if blanked */
	struct	cg6_cursor sc_cursor;	/* software cursor info */
	union	bt_cmap sc_cmap;	/* Brooktree color map */
	unsigned int quirks;
};

#ifdef RASTERCONSOLE
extern int cgsix_use_rasterconsole;
#else
#define cgsix_use_rasterconsole 0
#endif

/* XXX - export sbus attach struct for overloaded obio bus */
extern struct cfattach cgsix_sbus_ca;

void	cg6attach __P((struct cgsix_softc *, char *, int, int));
