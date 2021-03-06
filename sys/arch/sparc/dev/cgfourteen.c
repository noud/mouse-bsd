/*	$NetBSD: cgfourteen.c,v 1.15 1999/03/24 05:51:10 mrg Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by Harvard University and
 *	its contributors.
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
 *
 *   Based on:
 *	NetBSD: cgthree.c,v 1.28 1996/05/31 09:59:22 pk Exp
 *	NetBSD: cgsix.c,v 1.25 1996/04/01 17:30:00 christos Exp
 */

/*
 * Driver for Campus-II on-board mbus-based video (cgfourteen).
 * Provides minimum emulation of a Sun cgthree 8-bit framebuffer to
 * allow X to run.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

/*
 * The following is for debugging only; it opens up a security hole
 * enabled by allowing any user to map the control registers for the
 * cg14 into their space.
 */
#define CG14_MAP_REGS

/*
 * The following enables 24-bit operation: when opened, the framebuffer
 * will switch to 24-bit mode (actually 32-bit mode), and provide a
 * simple cg8 emulation.
 */
/* do this in the kernel config if at all... #define CG14_CG8 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <vm/vm.h>

#include <machine/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/fbvar.h>
#include <machine/cpu.h>
#include <machine/conf.h>
#include <machine/db_enterleave.h>

#include <sparc/dev/cgfourteenreg.h>
#include <sparc/dev/cgfourteenvar.h>

#include "opt_ddb.h"

/* autoconfiguration driver */
static void	cgfourteenattach(struct device *, struct device *, void *);
static int	cgfourteenmatch(struct device *, struct cfdata *, void *);
static void	cgfourteenunblank(struct device *);

/* cdevsw prototypes */
cdev_decl(cgfourteen);

struct cfattach cgfourteen_ca = {
	sizeof(struct cgfourteen_softc), cgfourteenmatch, cgfourteenattach
};

extern struct cfdriver cgfourteen_cd;

/* frame buffer generic driver */
static struct fbdriver cgfourteenfbdriver = {
	cgfourteenunblank, cgfourteenopen, cgfourteenclose, cgfourteenioctl,
	cgfourteenpoll, cgfourteenmmap
};

extern int fbnode;
extern struct tty *fbconstty;

static void cg14_set_video __P((struct cgfourteen_softc *, int));
static int  cg14_get_video __P((struct cgfourteen_softc *));
static int  cg14_get_cmap __P((struct fbcmap *, union cg14cmap *, int));
static int  cg14_put_cmap __P((struct fbcmap *, union cg14cmap *, int));
static void cg14_load_hwcmap __P((struct cgfourteen_softc *, int, int));
static void cg14_init __P((struct cgfourteen_softc *));
static void cg14_reset __P((struct cgfourteen_softc *));
static void cg14_loadomap __P((struct cgfourteen_softc *));/* cursor overlay */
static void cg14_setcursor __P((struct cgfourteen_softc *));/* set position */
static void cg14_loadcursor __P((struct cgfourteen_softc *));/* set shape */

/*
 * Match a cgfourteen.
 */
int
cgfourteenmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;

	/*
	 * The cgfourteen is a local-bus video adaptor, accessed directly
	 * via the processor, and not through device space or an external
	 * bus. Thus we look _only_ at the obio bus.
	 * Additionally, these things exist only on the Sun4m.
	 */

	if (uoba->uoba_isobio4 != 0 || !CPU_ISSUN4M)
		return (0);

	/* Check driver name */
	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}

/*
 * Set COLOUR_OFFSET to the offset of the video RAM.  This is to provide
 *  space for faked overlay junk for the cg8 emulation.
 *
 * As it happens, this value is correct for both cg3 and cg8 emulation!
 */
#define COLOUR_OFFSET (256*1024)

static void cg14_set_rcons_luts(struct cgfourteen_softc *sc)
{
 int i;

 for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_xlut->xlut_lut[i] = 0x22;
 for (i=0;i<CG14_CLUT_SIZE;i++) sc->sc_clut2->clut_lut[i] = 0x00000070;
 sc->sc_clut2->clut_lut[0] = 0;
 sc->sc_clut2->clut_lut[255] = 0x00ffffff;
}

static void cg14_shutdownhook(void *arg)
{
 struct cgfourteen_softc *sc;

 sc = arg;
 sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL;
}

#ifdef DDB
static void cg14_debug_enter(void *arg)
{
 struct cgfourteen_softc *sc;

 sc = arg;
 sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL;
}
#endif

#ifdef DDB
static void cg14_debug_leave(void *arg)
{
 struct cgfourteen_softc *sc;

 sc = arg;
 cg14_set_rcons_luts(sc);
#ifdef CG14_CG8
 sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL;
#else
 sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL;
#endif
}
#endif

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
void
cgfourteenattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct sbus_attach_args *sa = &uoba->uoba_sbus;
	struct cgfourteen_softc *sc = (struct cgfourteen_softc *)self;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int node, ramsize;
	int i, isconsole;

	node = sa->sa_node;

	/* Remember cookies for cgfourteenmmap() */
	sc->sc_bustag = sa->sa_bustag;

	fb->fb_driver = &cgfourteenfbdriver;
	fb->fb_device = &sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;

	/*
	 * Represent ourselves as whatever we're emulating.
	 */
#ifdef CG14_CG8
	fb->fb_type.fb_type = FBTYPE_MEMCOLOR;
	fb->fb_type.fb_depth = 32;
	fb_setsize_obp(fb, sc->sc_fb.fb_type.fb_depth, 1152, 900, node);
	ramsize = roundup(fb->fb_type.fb_height * fb->fb_linebytes, NBPG);
#else
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;
	fb->fb_type.fb_depth = 8;
	fb_setsize_obp(fb, sc->sc_fb.fb_type.fb_depth, 1152, 900, node);
	ramsize = roundup(fb->fb_type.fb_height * fb->fb_linebytes, NBPG);
#endif
	fb->fb_type.fb_cmsize = CG14_CLUT_SIZE;
	fb->fb_type.fb_size = ramsize + COLOUR_OFFSET;

	if (sa->sa_nreg < 2) {
		printf(": only %d register sets\n",sa->sa_nreg);
		return;
	}
	bcopy(sa->sa_reg, sc->sc_physadr,
	      sa->sa_nreg * sizeof(struct sbus_reg));

	/*
	 * Now map in the 8 useful pages of registers
	 */
	if (sa->sa_size < 0x10000) {
#ifdef DIAGNOSTIC
		printf("warning: can't find all cgfourteen registers...\n");
#endif
		sa->sa_size = 0x10000;
	}
	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset,
			 sa->sa_size,
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf(": cannot map control registers\n");
		return;
	}

	sc->sc_ctl   = (struct cg14ctl  *) (bh);
	sc->sc_hwc   = (struct cg14curs *) (bh + CG14_OFFSET_CURS);
	sc->sc_dac   = (struct cg14dac  *) (bh + CG14_OFFSET_DAC);
	sc->sc_xlut  = (struct cg14xlut *) (bh + CG14_OFFSET_XLUT);
	sc->sc_clut1 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT1);
	sc->sc_clut2 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT2);
	sc->sc_clut3 = (struct cg14clut *) (bh + CG14_OFFSET_CLUT3);
	sc->sc_clutincr =        (u_int *) (bh + CG14_OFFSET_CLUTINCR);

	/*
	 * Let the user know that we're here
	 */
 printf(": rev %d impl %d\n%s",
	(sc->sc_ctl->ctl_rsr & CG14_RSR_REVMASK) /
	  (CG14_RSR_REVMASK & ~(CG14_RSR_REVMASK-1)),
	(sc->sc_ctl->ctl_rsr & CG14_RSR_IMPLMASK) /
	  (CG14_RSR_IMPLMASK & ~(CG14_RSR_IMPLMASK-1)),
	&self->dv_xname[0] );
#ifdef CG14_CG8
	printf(": cgeight emulated at %dx%dx24bpp",
		fb->fb_type.fb_width, fb->fb_type.fb_height);
#else
	printf(": cgthree emulated at %dx%dx8bpp",
		fb->fb_type.fb_width, fb->fb_type.fb_height);
#endif
	/*
	 * Enable the video.
	 */
	cg14_set_video(sc, 1);

	/*
	 * Set up initial colormap
	 */
#if 1
	/* Do what bt_initcmap does (can't use it 'cause we don't use bt_subr) */
	sc->sc_cmap.cm_chip[0] = 0;
	for (i=1;i<CG14_CLUT_SIZE;i++) sc->sc_cmap.cm_chip[i] = ~0U;
	cg14_load_hwcmap(sc,0,CG14_CLUT_SIZE);
#else
	{	u_int32_t *lut;
		lut = (u_int32_t *) sc->sc_clut1->clut_lut;
		for (i = 0; i < CG14_CLUT_SIZE; i++)
			sc->sc_cmap.cm_chip[i] = lut[i];
	}
#endif

	/* See if we're the console */
	isconsole = node == fbnode && fbconstty != NULL;

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		/* *sbus*_bus_map?  but that's how we map the regs... */
/* ugh.  sbus_bus_map is a macro, so we can't do #ifdef inside its args! */
#ifdef CG14_CG8
#define VIDEO_OFFSET 0x03800000
#else
#define VIDEO_OFFSET 0
#endif
		if (sbus_bus_map( sc->sc_bustag,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
				  sc->sc_physadr[CG14_PXL_IDX].sbr_offset + VIDEO_OFFSET,
				  fb->fb_type.fb_width*fb->fb_type.fb_height,
				  BUS_SPACE_MAP_LINEAR,
				  0, &bh) != 0) {
			printf("%s: cannot map pixels\n",&self->dv_xname[0]);
		} else {
			sc->sc_fb.fb_type.fb_type = FBTYPE_SUN3COLOR;
			sc->sc_fb.fb_type.fb_depth = 8;
			sc->sc_fb.fb_linebytes = fb->fb_type.fb_width;
			sc->sc_fb.fb_type.fb_size = roundup(fb->fb_type.fb_width*fb->fb_type.fb_height,NBPG);
			sc->sc_fb.fb_pixels = (void *)bh;
			fbrcons_init(&sc->sc_fb);
			cg14_set_rcons_luts(sc);
#ifdef CG14_CG8
			sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_32 | CG14_MCTL_POWERCTL;
#else
			sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 | CG14_MCTL_POWERCTL;
#endif
			shutdownhook_establish(&cg14_shutdownhook,sc);
#ifdef DDB
			register_debugger_enterleave(&cg14_debug_enter,&cg14_debug_leave,sc);
#endif
		}
#endif
	} else
		printf("\n");

	/* Attach to /dev/fb */
	if (node == fbnode)
		fb_attach(&sc->sc_fb, isconsole);
}

/*
 * Keep track of the number of opens made. In the 24-bit driver, we need to
 * switch to 24-bit mode on the first open, and switch back to 8-bit on
 * the last close. This kind of nonsense is needed to give screenblank
 * a fighting chance of working.
 */
static int cg14_opens = 0;

int
cgfourteenopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct cgfourteen_softc *sc;
	int unit;
	int s, oldopens;

	unit = minor(dev);
	if (unit >= cgfourteen_cd.cd_ndevs)
		return(ENXIO);
	sc = cgfourteen_cd.cd_devs[minor(dev)];
	if (sc == NULL)
		return(ENXIO);
	s = splhigh();
	oldopens = cg14_opens++;
	splx(s);

	/* Setup the cg14 as we want it, and save the original PROM state */
	if (oldopens == 0)	/* first open only, to make screenblank work */
		cg14_init(sc);

	return (0);
}

int
cgfourteenclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct cgfourteen_softc *sc = cgfourteen_cd.cd_devs[minor(dev)];
	int s, opens;

	s = splhigh();
	opens = --cg14_opens;
	if (cg14_opens < 0)
		opens = cg14_opens = 0;
	splx(s);

	/*
	 * Restore video state to make the PROM happy, on last close.
	 */
	if (opens == 0)
		cg14_reset(sc);

	return (0);
}

int
cgfourteenioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct cgfourteen_softc *sc = cgfourteen_cd.cd_devs[minor(dev)];
	struct fbgattr *fba;
	union cg14cursor_cmap tcm;
	int v, error;
	u_int count;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
		fba = (struct fbgattr *)data;
		fba->real_type = FBTYPE_MDICOLOR;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
		break;

	case FBIOGETCMAP:
		return (cg14_get_cmap((struct fbcmap *)data, &sc->sc_cmap,
				     CG14_CLUT_SIZE));

	case FBIOPUTCMAP:
		/* copy to software map */
#define p ((struct fbcmap *)data)
#ifdef CG14_CG8
		p->index &= 0xffffff;
#endif
		error = cg14_put_cmap(p, &sc->sc_cmap, CG14_CLUT_SIZE);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		cg14_load_hwcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = cg14_get_video(sc);
		break;

	case FBIOSVIDEO:
		cg14_set_video(sc, *(int *)data);
		break;

/* these are for both FBIOSCURSOR and FBIOGCURSOR */
#define p ((struct fbcursor *)data)
#define cc (&sc->sc_cursor)
	case FBIOGCURSOR:
		/* do not quite want everything here... */
		p->set = FB_CUR_SETALL;	/* close enough, anyway */
		p->enable = cc->cc_enable;
		p->pos = cc->cc_pos;
		p->hot = cc->cc_hot;
		p->size = cc->cc_size;

		/* begin ugh ... can we lose some of this crap?? */
		if (p->image != NULL) {
			count = cc->cc_size.y * 32 / NBBY;
			error = copyout((caddr_t)cc->cc_cplane,
			    (caddr_t)p->image, count);
			if (error)
				return (error);
			error = copyout((caddr_t)cc->cc_eplane,
			    (caddr_t)p->mask, count);
			if (error)
				return (error);
		}
		if (p->cmap.red != NULL) {
			error = cg14_get_cmap(&p->cmap,
			    (union cg14cmap *)&cc->cc_color, 2);
			if (error)
				return (error);
		} else {
			p->cmap.index = 0;
			p->cmap.count = 2;
		}
		/* end ugh */
		break;

	case FBIOSCURSOR:
		/*
		 * For setcmap and setshape, verify parameters, so that
		 * we do not get halfway through an update and then crap
		 * out with the software state screwed up.
		 */
		v = p->set;
		if (v & FB_CUR_SETCMAP) {
			/*
			 * This use of a temporary copy of the cursor
			 * colormap is not terribly efficient, but these
			 * copies are small (8 bytes)...
			 */
			tcm = cc->cc_color;
			error = cg14_put_cmap(&p->cmap, (union cg14cmap *)&tcm,
					      2);
			if (error)
				return (error);
		}
		if (v & FB_CUR_SETSHAPE) {
			if ((u_int)p->size.x > 32 || (u_int)p->size.y > 32)
				return (EINVAL);
			count = p->size.y * 32 / NBBY;
			if (!uvm_useracc(p->image, count, B_READ) ||
			    !uvm_useracc(p->mask, count, B_READ))
				return (EFAULT);
		}

		/* parameters are OK; do it */
		if (v & (FB_CUR_SETCUR | FB_CUR_SETPOS | FB_CUR_SETHOT)) {
			if (v & FB_CUR_SETCUR)
				cc->cc_enable = p->enable;
			if (v & FB_CUR_SETPOS)
				cc->cc_pos = p->pos;
			if (v & FB_CUR_SETHOT)
				cc->cc_hot = p->hot;
			cg14_setcursor(sc);
		}
		if (v & FB_CUR_SETCMAP) {
			cc->cc_color = tcm;
			cg14_loadomap(sc); /* XXX defer to vertical retrace */
		}
		if (v & FB_CUR_SETSHAPE) {
			cc->cc_size = p->size;
			count = p->size.y * 32 / NBBY;
			bzero((caddr_t)cc->cc_eplane, sizeof cc->cc_eplane);
			bzero((caddr_t)cc->cc_cplane, sizeof cc->cc_cplane);
			bcopy(p->mask, (caddr_t)cc->cc_eplane, count);
			bcopy(p->image, (caddr_t)cc->cc_cplane, count);
			cg14_loadcursor(sc);
		}
		break;

#undef cc
#undef p
	case FBIOGCURPOS:
		*(struct fbcurpos *)data = sc->sc_cursor.cc_pos;
		break;

	case FBIOSCURPOS:
		sc->sc_cursor.cc_pos = *(struct fbcurpos *)data;
		cg14_setcursor(sc);
		break;

	case FBIOGCURMAX:
		/* max cursor size is 32x32 */
		((struct fbcurpos *)data)->x = 32;
		((struct fbcurpos *)data)->y = 32;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
cgfourteenunblank(dev)
	struct device *dev;
{

	cg14_set_video((struct cgfourteen_softc *)dev, 1);
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * Since we're pretending to be a cg8, we put the main video RAM at the
 *  same place the cg8 does, at offset 256k.  The cg8 has an enable
 *  plane in the 256k space; our "enable" plane works differently.  We
 *  can't emulate the enable plane very well, but all X uses it for is
 *  to clear it at startup - so we map the first page of video RAM over
 *  and over to fill that 256k space.  We also map some other views of
 *  the video RAM space.
 *
 * Views of video RAM have to be sized according to the framebuffer's
 *  dimensions (or at least they should be); in the table below, a
 *  "size" of "fb8" is width*height bytes, "fb16" for width*height*2
 *  bytes, and "fb32" is width*height*4 bytes (8bpp, 16bpp, and 32bpp
 *  views).  The actual numbers for these depend on the resolution;
 *  here are values for the standard resolutions:
 *
 *	size		fb8		fb16		fb32
 *	1024x768	000c0000	00180000	00300000
 *	1152x900	000fd200	001fa400	003f4800
 *	1280x1024	00140000	00280000	00500000
 *	1600x1280	001f4000	003e8000	007d0000
 *	1920x1080	001fa400	003f4800	007e9000
 *
 * Our memory map thus looks like
 *
 *	base		size		space	card offset
 *	00000000	00040000	vram	0 (see above)
 *	00040000	fb32		vram	00000000
 *	01000000	fb32		vram	01000000
 *	02000000	fb16		vram	02000000
 *	02800000	fb16		vram	02800000
 *	03000000	fb8		vram	03000000
 *	03400000	fb8		vram	03400000
 *	03800000	fb8		vram	03800000
 *	03c00000	fb8		vram	03c00000
 *	10000000	00010000	regs	00000000 (iff CG14_MAP_REGS)
 */
int
cgfourteenmmap(dev_t dev, int off, int prot)
{
 struct cgfourteen_softc *sc;
 bus_space_handle_t bh;
 unsigned int sz;

 sc = cgfourteen_cd.cd_devs[minor(dev)];
 if (off & PGOFSET) panic("cgfourteenmmap");
 if (off < 0) return(-1);
 sz = sc->sc_fb.fb_type.fb_width * sc->sc_fb.fb_type.fb_height;
 switch (off >> 22)
  { case 0x000: case 0x001: case 0x002: case 0x003:
       /* 0x00000000 - fb32, with special case for COLOUR_OFFSET */
       if (off < COLOUR_OFFSET) off = 0; else off -= COLOUR_OFFSET;
       sz <<= 2;
       break;
    case 0x004 ... 0x007: /* 0x01000000 - fb32 */
       sz <<= 2;
       break;
    case 0x008: case 0x009: /* 0x02000000 - fb16 */
    case 0x00a: case 0x00b: /* 0x02800000 - fb16 */
       sz <<= 1;
       break;
    case 0x00c: /* 0x03000000 - fb8 */
    case 0x00d: /* 0x03400000 - fb8 */
    case 0x00e: /* 0x03800000 - fb8 */
    case 0x00f: /* 0x03c00000 - fb8 */
       break;
#if defined(CG14_MAP_REGS)
    case 0x040: /* 0x10000000 - 0x00010000 */
       /*
	* I am told this opens up a security hole and thus should be
	*  used only for debugging.  While I am inclined to believe
	*  this, I also have found no information on the details, only
	*  vague handwaving about giving access to blitters and such.
	*/
       if ((off & 0x003fffff) >= 0x00010000) return(-1);
       if (bus_space_mmap( sc->sc_bustag,
			   sc->sc_physadr[CG14_CTL_IDX].sbr_slot,
			   sc->sc_physadr[CG14_CTL_IDX].sbr_offset+(off&0x003fffff),
			   BUS_SPACE_MAP_LINEAR, &bh)) return(-1);
       return(bh);
       break;
#endif
    default:
       return(-1);
       break;
  }
 if ((off & 0x003fffff) >= sz) return(-1);
 if (bus_space_mmap( sc->sc_bustag,
		     sc->sc_physadr[CG14_PXL_IDX].sbr_slot,
		     sc->sc_physadr[CG14_PXL_IDX].sbr_offset+off,
		     BUS_SPACE_MAP_LINEAR, &bh)) return(-1);
 return(bh);
}

int
cgfourteenpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (seltrue(dev, events, p));
}

/*
 * Miscellaneous helper functions
 */

/* Initialize the framebuffer, storing away useful state for later reset */
static void
cg14_init(sc)
	struct cgfourteen_softc *sc;
{
	u_int32_t *clut;
	u_int8_t  *xlut;
	int i;

	/*
	 * We stash away the following to restore on close:
	 *
	 * 	color look-up table 1 	(sc->sc_saveclut)
	 *	x look-up table		(sc->sc_savexlut)
	 *	control register	(sc->sc_savectl)
	 *	cursor control register (sc->sc_savehwc)
	 */
	sc->sc_savectl = sc->sc_ctl->ctl_mctl;
	sc->sc_savehwc = sc->sc_hwc->curs_ctl;

	clut = (u_int32_t *) sc->sc_clut1->clut_lut;
	xlut = (u_int8_t *) sc->sc_xlut->xlut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++) {
		sc->sc_saveclut.cm_chip[i] = clut[i];
		sc->sc_savexlut[i] = xlut[i];
	}

#ifdef CG14_CG8
	/*
	 * Enable the video, and put in 24 bit mode.
	 */
	sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_32 |
		CG14_MCTL_POWERCTL;

	/*
	 * Zero the xlut to enable direct-color mode
	 */
	for (i = 0; i < CG14_CLUT_SIZE; i++)
		sc->sc_xlut->xlut_lut[i] = 0;
#else
	/*
	 * Enable the video and put it in 8 bit mode
	 */
	sc->sc_ctl->ctl_mctl = CG14_MCTL_ENABLEVID | CG14_MCTL_PIXMODE_8 |
		CG14_MCTL_POWERCTL;
#endif
}

static void
cg14_reset(sc)	/* Restore the state saved on cg14_init */
	struct cgfourteen_softc *sc;
{
	u_int32_t *clut;
	u_int8_t  *xlut;
	int i;

	/*
	 * We restore the following, saved in cg14_init:
	 *
	 * 	color look-up table 1 	(sc->sc_saveclut)
	 *	x look-up table		(sc->sc_savexlut)
	 *	control register	(sc->sc_savectl)
	 *	cursor control register (sc->sc_savehwc)
	 *
	 * Note that we don't touch the video enable bits in the
	 * control register; otherwise, screenblank wouldn't work.
	 */
	sc->sc_ctl->ctl_mctl = (sc->sc_ctl->ctl_mctl & (CG14_MCTL_ENABLEVID |
							CG14_MCTL_POWERCTL)) |
				(sc->sc_savectl & ~(CG14_MCTL_ENABLEVID |
						    CG14_MCTL_POWERCTL));
	sc->sc_hwc->curs_ctl = sc->sc_savehwc;

	clut = (u_int32_t *) sc->sc_clut1->clut_lut;
	xlut = (u_int8_t *) sc->sc_xlut->xlut_lut;
	for (i = 0; i < CG14_CLUT_SIZE; i++) {
		clut[i] = sc->sc_saveclut.cm_chip[i];
		xlut[i] = sc->sc_savexlut[i];
	}
}

/* Enable/disable video display; power down monitor if DPMS-capable */
static void
cg14_set_video(sc, enable)
	struct cgfourteen_softc *sc;
	int enable;
{
	/*
	 * We can only use DPMS to power down the display if the chip revision
	 * is greater than 0.
	 */
	if (enable) {
		if ((sc->sc_ctl->ctl_rsr & CG14_RSR_REVMASK) > 0)
			sc->sc_ctl->ctl_mctl |= (CG14_MCTL_ENABLEVID |
						 CG14_MCTL_POWERCTL);
		else
			sc->sc_ctl->ctl_mctl |= CG14_MCTL_ENABLEVID;
	} else {
		if ((sc->sc_ctl->ctl_rsr & CG14_RSR_REVMASK) > 0)
			sc->sc_ctl->ctl_mctl &= ~(CG14_MCTL_ENABLEVID |
						  CG14_MCTL_POWERCTL);
		else
			sc->sc_ctl->ctl_mctl &= ~CG14_MCTL_ENABLEVID;
	}
}

/* Get status of video display */
static int
cg14_get_video(sc)
	struct cgfourteen_softc *sc;
{
	return ((sc->sc_ctl->ctl_mctl & CG14_MCTL_ENABLEVID) != 0);
}

/* Read the software shadow colormap */
static int
cg14_get_cmap(p, cm, cmsize)
	struct fbcmap *p;
	union cg14cmap *cm;
	int cmsize;
{
        u_int i, start, count;
        u_char *cp;

        start = p->index;
        count = p->count;
        if (start >= cmsize || start + count > cmsize)
#ifdef DEBUG
	{
		printf("putcmaperror: start %d cmsize %d count %d\n",
		    start,cmsize,count);
#endif
                return (EINVAL);
#ifdef DEBUG
	}
#endif

        if (!uvm_useracc(p->red, count, B_WRITE) ||
            !uvm_useracc(p->green, count, B_WRITE) ||
            !uvm_useracc(p->blue, count, B_WRITE))
                return (EFAULT);
        for (cp = &cm->cm_map[start][0], i = 0; i < count; cp += 4, i++) {
                p->red[i] = cp[3];
                p->green[i] = cp[2];
                p->blue[i] = cp[1];
        }
        return (0);
}

/* Write the software shadow colormap */
static int
cg14_put_cmap(p, cm, cmsize)
        struct fbcmap *p;
        union cg14cmap *cm;
        int cmsize;
{
        u_int i, start, count;
        u_char *cp;

        start = p->index;
        count = p->count;
        if (start >= cmsize || start + count > cmsize)
#ifdef DEBUG
	{
		printf("putcmaperror: start %d cmsize %d count %d\n",
		    start,cmsize,count);
#endif
                return (EINVAL);
#ifdef DEBUG
	}
#endif

        if (!uvm_useracc(p->red, count, B_READ) ||
            !uvm_useracc(p->green, count, B_READ) ||
            !uvm_useracc(p->blue, count, B_READ))
                return (EFAULT);
        for (cp = &cm->cm_map[start][0], i = 0; i < count; cp += 4, i++) {
                cp[3] = p->red[i];
                cp[2] = p->green[i];
                cp[1] = p->blue[i];
		cp[0] = 0;	/* no alpha channel */
        }
        return (0);
}

static void
cg14_load_hwcmap(sc, start, ncolors)
	struct cgfourteen_softc *sc;
	int start, ncolors;
{
	/* XXX switch to auto-increment, and on retrace intr */

	/* Setup pointers to source and dest */
	u_int32_t *colp = &sc->sc_cmap.cm_chip[start];
	volatile u_int32_t *lutp = &sc->sc_clut1->clut_lut[start];

	/* Copy by words */
	while (--ncolors >= 0)
		*lutp++ = *colp++;
}

/*
 * Load the cursor (overlay `foreground' and `background') colors.
 */
static void
cg14_setcursor(sc)
	struct cgfourteen_softc *sc;
{
	/* we need to subtract the hot-spot value here */
#define COORD(f) (sc->sc_cursor.cc_pos.f - sc->sc_cursor.cc_hot.f)

	sc->sc_hwc->curs_ctl = (sc->sc_cursor.cc_enable ? CG14_CURS_ENABLE : 0);
	sc->sc_hwc->curs_x = COORD(x);
	sc->sc_hwc->curs_y = COORD(y);

#undef COORD
}

static void
cg14_loadcursor(sc)
	struct cgfourteen_softc *sc;
{
	volatile struct cg14curs *hwc;
	u_int edgemask, m;
	int i;

	/*
	 * Keep the top size.x bits.  Here we *throw out* the top
	 * size.x bits from an all-one-bits word, introducing zeros in
	 * the top size.x bits, then invert all the bits to get what
	 * we really wanted as our mask.  But this fails if size.x is
	 * 32---a sparc uses only the low 5 bits of the shift count---
	 * so we have to special case that.
	 */
	edgemask = ~0;
	if (sc->sc_cursor.cc_size.x < 32)
		edgemask = ~(edgemask >> sc->sc_cursor.cc_size.x);
	hwc = sc->sc_hwc;
	for (i = 0; i < 32; i++) {
		m = sc->sc_cursor.cc_eplane[i] & edgemask;
		hwc->curs_plane0[i] = m;
		hwc->curs_plane1[i] = m & sc->sc_cursor.cc_cplane[i];
	}
}

static void
cg14_loadomap(sc)
	struct cgfourteen_softc *sc;
{
	/* set background color */
	sc->sc_hwc->curs_color1 = sc->sc_cursor.cc_color.cm_chip[0];
	/* set foreground color */
	sc->sc_hwc->curs_color2 = sc->sc_cursor.cc_color.cm_chip[1];
}
