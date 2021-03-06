/*	$NetBSD: bwtwo.c,v 1.42 1999/08/10 04:56:30 christos Exp $ */

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
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
 *	@(#)bwtwo.c	8.1 (Berkeley) 6/11/93
 */

/*
 * black&white display (bwtwo) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * P4 and overlay plane support by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 * Overlay plane handling hints and ideas provided by Brad Spencer.
 */

#include <sys/param.h>
#include <sys/systm.h>
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
#include <machine/eeprom.h>
#include <machine/ctlreg.h>
#include <machine/conf.h>
#include <sparc/sparc/asm.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/bwtworeg.h>
#include <sparc/dev/bwtwovar.h>
#include <sparc/dev/pfourreg.h>

/* cdevsw prototypes */
cdev_decl(bwtwo);

extern struct cfdriver bwtwo_cd;

/* XXX we do not handle frame buffer interrupts (do not know how) */
static void	bwtwounblank __P((struct device *));

/* frame buffer generic driver */
static struct fbdriver bwtwofbdriver = {
	bwtwounblank, bwtwoopen, bwtwoclose, bwtwoioctl, bwtwopoll, bwtwommap
};

int
bwtwo_pfour_probe(vaddr, arg)
	void *vaddr;
	void *arg;
{
	struct cfdata *cf = arg;

	switch (fb_pfour_id(vaddr)) {
	case PFOUR_ID_BW:
	case PFOUR_ID_COLOR8P1:		/* bwtwo in ... */
	case PFOUR_ID_COLOR24:		/* ...overlay plane */
		/* This is wrong; should be done in bwtwo_attach() */
		cf->cf_flags |= FB_PFOUR;
		/* FALLTHROUGH */
	case PFOUR_NOTPFOUR:
		return (1);
	}
	return (0);
}

void
bwtwoattach(sc, name, isconsole, isfb)
	struct	bwtwo_softc *sc;
	char	*name;
	int	isconsole;
	int	isfb;
{
	struct fbdevice *fb = &sc->sc_fb;

	/* Fill in the remaining fbdevice values */
	fb->fb_driver = &bwtwofbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_type.fb_type = FBTYPE_SUN2BW;
	fb->fb_type.fb_cmsize = 0;
	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	printf(": %s, %d x %d", name,
	       fb->fb_type.fb_width, fb->fb_type.fb_height);

	/* Insure video is enabled */
	sc->sc_set_video(sc, 1);

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		/*
		 * XXX rcons doesn't seem to work properly on the overlay
		 * XXX plane.  This is a temporary kludge until someone
		 * XXX fixes it.
		 */
		if ((fb->fb_flags & FB_PFOUR) == 0 ||
		    (sc->sc_ovtype == BWO_NONE))
			fbrcons_init(fb);
#endif
	} else
		printf("\n");

	if ((fb->fb_flags & FB_PFOUR) && (sc->sc_ovtype != BWO_NONE)) {
		char *ovnam;

		switch (sc->sc_ovtype) {
		case BWO_CGFOUR:
			ovnam = "cgfour";
			break;

		case BWO_CGEIGHT:
			ovnam = "cgeight";
			break;

		default:
			ovnam = "unknown";
			break;
		}
		printf("%s: %s overlay plane\n", sc->sc_dev.dv_xname, ovnam);
	}

	if (isfb) {
		/*
		 * If we're on an overlay plane of a color framebuffer,
		 * then we don't force the issue in fb_attach() because
		 * we'd like the color framebuffer to actually be the
		 * "console framebuffer".  We're only around to speed
		 * up rconsole.
		 */
		if ((fb->fb_flags & FB_PFOUR) && (sc->sc_ovtype != BWO_NONE ))
			fb_attach(fb, 0);
		else
			fb_attach(fb, isconsole);
	}
}

int
bwtwoopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= bwtwo_cd.cd_ndevs || bwtwo_cd.cd_devs[unit] == NULL)
		return (ENXIO);

	return (0);
}

int
bwtwoclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{

	return (0);
}

int
bwtwoioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct bwtwo_softc *sc = bwtwo_cd.cd_devs[minor(dev)];

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGVIDEO:
		*(int *)data = sc->sc_get_video(sc);
		break;

	case FBIOSVIDEO:
		sc->sc_set_video(sc, (*(int *)data));
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static void
bwtwounblank(dev)
	struct device *dev;
{
	struct bwtwo_softc *sc = (struct bwtwo_softc *)dev;

	sc->sc_set_video(sc, 1);
}

int
bwtwopoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (seltrue(dev, events, p));
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
int
bwtwommap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	struct bwtwo_softc *sc = bwtwo_cd.cd_devs[minor(dev)];
	bus_space_handle_t bh;

	if (off & PGOFSET)
		panic("bwtwommap");

	if ((unsigned)off >= sc->sc_fb.fb_type.fb_size)
		return (-1);

	if (bus_space_mmap(sc->sc_bustag,
			   sc->sc_btype,
			   sc->sc_paddr + sc->sc_pixeloffset + off,
			   BUS_SPACE_MAP_LINEAR, &bh))
		return (-1);

	return ((int)bh);
}
