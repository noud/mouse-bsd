/*	$NetBSD: bwtwo_obio.c,v 1.1 1999/08/10 04:56:30 christos Exp $ */

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

/* autoconfiguration driver */
static void	bwtwoattach_obio __P((struct device *, struct device *, void *));
static int	bwtwomatch_obio __P((struct device *, struct cfdata *, void *));


struct cfattach bwtwo_obio_ca = {
	sizeof(struct bwtwo_softc), bwtwomatch_obio, bwtwoattach_obio
};

static int	bwtwo_get_video_sun4  __P((struct bwtwo_softc *));
static void	bwtwo_set_video_sun4 __P((struct bwtwo_softc *, int));

extern int fbnode;
extern struct tty *fbconstty;

static int
bwtwomatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (bus_space_probe(oba->oba_bustag, 0, oba->oba_paddr,
				4,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				bwtwo_pfour_probe, cf));
}

static void
bwtwoattach_obio(parent, self, uax)
	struct device *parent, *self;
	void *uax;
{
	struct bwtwo_softc *sc = (struct bwtwo_softc *)self;
	union obio_attach_args *uoba = uax;
	struct obio4_attach_args *oba;
	struct fbdevice *fb = &sc->sc_fb;
	struct eeprom *eep = (struct eeprom *)eeprom_va;
	bus_space_handle_t bh;
	int constype, isconsole;
	char *name;

	oba = &uoba->uoba_oba4;

	/* Remember cookies for bwtwo_mmap() */
	sc->sc_bustag = oba->oba_bustag;
	sc->sc_btype = (bus_type_t)0;
	sc->sc_paddr = (bus_addr_t)oba->oba_paddr;

	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags;
	fb->fb_type.fb_depth = 1;
	fb_setsize_eeprom(fb, fb->fb_type.fb_depth, 1152, 900);

	constype = (fb->fb_flags & FB_PFOUR) ? EE_CONS_P4OPT : EE_CONS_BW;
	if (eep == NULL || eep->eeConsole == constype)
		isconsole = (fbconstty != NULL);
	else
		isconsole = 0;

	if (fb->fb_flags & FB_PFOUR) {
		/*
		 * Map the pfour control register.
		 * Set pixel offset to appropriate overlay plane.
		 */
		name = "bwtwo/p4";

		if (obio_bus_map(oba->oba_bustag,
				 oba->oba_paddr,
				 0,
				 sizeof(u_int32_t),
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pfour register\n",
				self->dv_xname);
			return;
		}
		fb->fb_pfour = (u_int32_t *)bh;
		sc->sc_reg = NULL;

		/*
		 * Notice if this is an overlay plane on a color
		 * framebuffer.  Note that PFOUR_COLOR_OFF_OVERLAY
		 * is the same as PFOUR_BW_OFF, but we use the
		 * different names anyway.
		 */
		switch (PFOUR_ID(*fb->fb_pfour)) {
		case PFOUR_ID_COLOR8P1:
			sc->sc_ovtype = BWO_CGFOUR;
			sc->sc_pixeloffset = PFOUR_COLOR_OFF_OVERLAY;
			break;

		case PFOUR_ID_COLOR24:
			sc->sc_ovtype = BWO_CGEIGHT;
			sc->sc_pixeloffset = PFOUR_COLOR_OFF_OVERLAY;
			break;

		default:
			sc->sc_ovtype = BWO_NONE;
			sc->sc_pixeloffset = PFOUR_BW_OFF;
			break;
		}

	} else {
		/* A plain bwtwo */
		if (obio_bus_map(oba->oba_bustag,
				 oba->oba_paddr,
				 BWREG_REG,
				 sizeof(struct fbcontrol),
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map control registers\n",
				self->dv_xname);
			return;
		}
		sc->sc_reg = (struct fbcontrol *)bh;
		fb->fb_pfour = NULL;

		name = "bwtwo";
		sc->sc_pixeloffset = BWREG_MEM;
	}
	sc->sc_get_video = bwtwo_get_video_sun4;
	sc->sc_set_video = bwtwo_set_video_sun4;

	if (isconsole) {
		int ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
		if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
				 sc->sc_pixeloffset,
				 ramsize,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s: cannot map pixels\n", self->dv_xname);
			return;
		}
		sc->sc_fb.fb_pixels = (char *)bh;
	}

	bwtwoattach(sc, name, isconsole, 1);
}

static void
bwtwo_set_video_sun4(sc, enable)
	struct bwtwo_softc *sc;
	int enable;
{

	if (sc->sc_fb.fb_flags & FB_PFOUR) {
		/*
		 * This handles the overlay plane case, too.
		 */
		fb_pfour_set_video(&sc->sc_fb, enable);
		return;
	}
	if (enable)
		stba(AC_SYSENABLE, ASI_CONTROL,
		     lduba(AC_SYSENABLE, ASI_CONTROL) | SYSEN_VIDEO);
	else
		stba(AC_SYSENABLE, ASI_CONTROL,
		     lduba(AC_SYSENABLE, ASI_CONTROL) & ~SYSEN_VIDEO);

	return;
}

static int
bwtwo_get_video_sun4(sc)
	struct bwtwo_softc *sc;
{

	if (sc->sc_fb.fb_flags & FB_PFOUR) {
		/*
		 * This handles the overlay plane case, too.
		 */
		return (fb_pfour_get_video(&sc->sc_fb));
	} else
		return ((lduba(AC_SYSENABLE, ASI_CONTROL) & SYSEN_VIDEO) != 0);
}
