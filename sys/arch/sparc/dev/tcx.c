/*	$NetBSD: tcx.c,v 1.15 1999/08/27 10:49:20 hannken Exp $ */

/*
 *  Copyright (c) 1996,1998 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to The NetBSD Foundation
 *  by Paul Kranenburg.
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
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * color display (TCX) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <machine/fbio.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifdef DEBUG
#include <sys/proc.h>
#include <sys/syslog.h>
#endif

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/fbvar.h>
#include <machine/cpu.h>
#include <machine/conf.h>

#include <sparc/dev/btreg.h>
#include <sparc/dev/btvar.h>
#include <sparc/dev/tcxreg.h>
#include <sparc/dev/sbusvar.h>

/* per-display variables */
struct tcx_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */
#ifdef RASTERCONSOLE
	struct fbdevice	sc_rcfb;	/* fb for rasterconsole */
#endif
	bus_space_tag_t	sc_bustag;
	struct sbus_reg	sc_physadr[TCX_NREG];	/* phys addr of h/w */

	volatile struct bt_regs *sc_bt;	/* Brooktree registers */
	volatile struct tcx_thc *sc_thc;/* THC registers */
	short	sc_blanked;		/* true if blanked */
	short	sc_8bit;		/* true if 8-bit-only */
	union	bt_cmap sc_cmap;	/* Brooktree color map */
};

/* autoconfiguration driver */
static void	tcxattach __P((struct device *, struct device *, void *));
static int	tcxmatch __P((struct device *, struct cfdata *, void *));
static void	tcx_unblank __P((struct device *));

/* cdevsw prototypes */
cdev_decl(tcx);

struct cfattach tcx_ca = {
	sizeof(struct tcx_softc), tcxmatch, tcxattach
};

extern struct cfdriver tcx_cd;

/* frame buffer generic driver */
static struct fbdriver tcx_fbdriver = {
	tcx_unblank, tcxopen, tcxclose, tcxioctl, tcxpoll, tcxmmap
};

extern int fbnode;

static void tcx_reset __P((struct tcx_softc *));
static void tcx_loadcmap __P((struct tcx_softc *, int, int));

#ifdef RASTERCONSOLE
int tcx_use_rasterconsole = 1;
#endif

#define OBPNAME	"SUNW,tcx"
/*
 * Match a tcx.
 */
int
tcxmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(sa->sa_name, OBPNAME) == 0);
}

/*
 * Attach a display.
 */
void
tcxattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct tcx_softc *sc = (struct tcx_softc *)self;
	struct sbus_attach_args *sa = args;
	int node, ramsize;
	volatile struct bt_regs *bt;
	struct fbdevice *fb = &sc->sc_fb;
	bus_space_handle_t bh;
	int isconsole;
	extern struct tty *fbconstty;

	sc->sc_bustag = sa->sa_bustag;
	node = sa->sa_node;

	sc->sc_8bit = node_has_property(node, "tcx-8-bit") ? 1 : 0;

	fb->fb_driver = &tcx_fbdriver;
	fb->fb_device = &sc->sc_dev;
	/* Mask out invalid flags from the user. */
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
	fb->fb_type.fb_depth = sc->sc_8bit ? 8 : 32;

	fb_setsize_obp(fb, fb->fb_type.fb_depth, 1152, 900, node);

	fb->fb_linebytes = (fb->fb_type.fb_width * fb->fb_type.fb_depth) >> 3;
	ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
	fb->fb_type.fb_cmsize = 256;
	fb->fb_type.fb_size = ramsize;
	printf(": %s, %d x %d (%dbpp)", OBPNAME,
		fb->fb_type.fb_width,
		fb->fb_type.fb_height,
		sc->sc_8bit?8:24);

	/*
	 * XXX - should be set to FBTYPE_TCX (which doesn't exist).
	 * XXX For CG3 emulation to work in current (96/6) X11 servers,
	 * XXX fb_type must point to an "unrecognised" entry.
	 */
	fb->fb_type.fb_type = FBTYPE_RESERVED3;

	if (sa->sa_nreg != TCX_NREG) {
		printf("\n%s: only %d register sets\n",
			self->dv_xname, sa->sa_nreg);
		return;
	}
	bcopy(sa->sa_reg, sc->sc_physadr,
	      sa->sa_nreg * sizeof(struct sbus_reg));

	/* I don't know what's with this.  Perhaps 8bit tcxen have
	   sbr_offset values one page too low or something.  But for
	   the s24, this is definitely wrong. */
#if 0
	/* XXX - fix THC and TEC offsets */
	sc->sc_physadr[TCX_REG_TEC].sbr_offset += 0x1000;
	sc->sc_physadr[TCX_REG_THC].sbr_offset += 0x1000;
#endif

	/* Map the register banks we care about */
	if (sbus_bus_map(sa->sa_bustag,
			 (bus_type_t)sc->sc_physadr[TCX_REG_THC].sbr_slot,
			 (bus_addr_t)sc->sc_physadr[TCX_REG_THC].sbr_offset,
			 sizeof (struct tcx_thc),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("tcxattach: cannot map thc registers\n");
		return;
	}
	sc->sc_thc = (volatile struct tcx_thc *)bh;

	if (sbus_bus_map(sa->sa_bustag,
			 (bus_type_t)sc->sc_physadr[TCX_REG_CMAP].sbr_slot,
			 (bus_addr_t)sc->sc_physadr[TCX_REG_CMAP].sbr_offset,
			 sizeof (struct bt_regs),
			 BUS_SPACE_MAP_LINEAR,
			 0, &bh) != 0) {
		printf("tcxattach: cannot map bt registers\n");
		return;
	}
	sc->sc_bt = bt = (volatile struct bt_regs *)bh;


	isconsole = node == fbnode && fbconstty != NULL;

	printf(", id %d, rev %d, sense %d",
		(sc->sc_thc->thc_config & THC_CFG_FBID) >> THC_CFG_FBID_SHIFT,
		(sc->sc_thc->thc_config & THC_CFG_REV) >> THC_CFG_REV_SHIFT,
		(sc->sc_thc->thc_config & THC_CFG_SENSE) >> THC_CFG_SENSE_SHIFT
	);

	/* reset cursor & frame buffer controls */
	tcx_reset(sc);

	/* Initialize the default color map. */
	bt_initcmap(&sc->sc_cmap, 256);
	tcx_loadcmap(sc, 0, 256);

	/* enable video */
	sc->sc_thc->thc_hcmisc |= THC_MISC_VIDEN;

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		if (tcx_use_rasterconsole) {
			if (sbus_bus_map( sc->sc_bustag,
					  sc->sc_physadr[TCX_REG_DFB8].sbr_slot,
					  sc->sc_physadr[TCX_REG_DFB8].sbr_offset,
					  1152*900, BUS_SPACE_MAP_LINEAR,
					  0, &bh) != 0) {
				printf("%s: cannot map pixels\n",&sc->sc_dev.dv_xname[0]);
			} else {
				sc->sc_rcfb = sc->sc_fb;
				sc->sc_rcfb.fb_type.fb_type = FBTYPE_SUN3COLOR;
				sc->sc_rcfb.fb_type.fb_depth = 8;
				sc->sc_rcfb.fb_linebytes = 1152;
				sc->sc_rcfb.fb_type.fb_size = roundup(1152*900,NBPG);
				sc->sc_rcfb.fb_pixels = (void *)bh;
				fbrcons_init(&sc->sc_rcfb);
			}
		}
#endif
	} else
		printf("\n");

	sbus_establish(&sc->sc_sd, &sc->sc_dev);
	if (node == fbnode)
		fb_attach(&sc->sc_fb, isconsole);
}

int
tcxopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = minor(dev);

	if (unit >= tcx_cd.cd_ndevs || tcx_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
tcxclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct tcx_softc *sc = tcx_cd.cd_devs[minor(dev)];

	tcx_reset(sc);
	return (0);
}

int
tcxioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct tcx_softc *sc = tcx_cd.cd_devs[minor(dev)];
	int error;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
#define fba ((struct fbgattr *)data)
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = FBTYPE_SUN3COLOR;
		fba->emu_types[2] = -1;
#undef fba
		break;

	case FBIOGETCMAP:
		return (bt_getcmap((struct fbcmap *)data, &sc->sc_cmap, 256));

	case FBIOPUTCMAP:
		/* copy to software map */
#define	p ((struct fbcmap *)data)
		error = bt_putcmap(p, &sc->sc_cmap, 256);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		tcx_loadcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = sc->sc_blanked;
		break;

	case FBIOSVIDEO:
		if (*(int *)data)
			tcx_unblank(&sc->sc_dev);
		else if (!sc->sc_blanked) {
			sc->sc_blanked = 1;
			sc->sc_thc->thc_hcmisc &= ~THC_MISC_VIDEN;
			/* Put monitor in `power-saving mode' */
			sc->sc_thc->thc_hcmisc |= THC_MISC_VSYNC_DISABLE;
			sc->sc_thc->thc_hcmisc |= THC_MISC_HSYNC_DISABLE;
		}
		break;

	default:
#ifdef DEBUG
		log(LOG_NOTICE, "tcxioctl(0x%lx) (%s[%d])\n", cmd,
		    p->p_comm, p->p_pid);
#endif
		return (ENOTTY);
	}
	return (0);
}

int
tcxpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (seltrue(dev, events, p));
}

/*
 * Clean up hardware state (e.g., after bootup or after X crashes).
 */
static void
tcx_reset(sc)
	struct tcx_softc *sc;
{
	volatile struct bt_regs *bt;

	/* Enable cursor in Brooktree DAC. */
	bt = sc->sc_bt;
	bt->bt_addr = 0x06 << 24;
	bt->bt_ctrl |= 0x03 << 24;
}

/*
 * Load a subset of the current (new) colormap into the color DAC.
 */
static void
tcx_loadcmap(sc, start, ncolors)
	struct tcx_softc *sc;
	int start, ncolors;
{
	volatile struct bt_regs *bt;
	u_int *ip, i;
	int count;

	ip = &sc->sc_cmap.cm_chip[BT_D4M3(start)];	/* start/4 * 3 */
	count = BT_D4M3(start + ncolors - 1) - BT_D4M3(start) + 3;
	bt = sc->sc_bt;
	bt->bt_addr = BT_D4M4(start) << 24;
	while (--count >= 0) {
		i = *ip++;
		/* hardware that makes one want to pound boards with hammers */
		bt->bt_cmap = i;
		bt->bt_cmap = i << 8;
		bt->bt_cmap = i << 16;
		bt->bt_cmap = i << 24;
	}
}

static void
tcx_unblank(dev)
	struct device *dev;
{
	struct tcx_softc *sc = (struct tcx_softc *)dev;

	if (sc->sc_blanked) {
		sc->sc_blanked = 0;
		sc->sc_thc->thc_hcmisc &= ~THC_MISC_VSYNC_DISABLE;
		sc->sc_thc->thc_hcmisc &= ~THC_MISC_HSYNC_DISABLE;
		sc->sc_thc->thc_hcmisc |= THC_MISC_VIDEN;
	}
}

/*
 * Base addresses at which users can mmap() the various pieces of a tcx.
 */
#define	TCX_USER_RAM	0x00000000
#define	TCX_USER_RAM24	0x01000000
#define	TCX_USER_RAM_COMPAT	0x04000000	/* cg3 emulation */
#define	TCX_USER_STIP	0x10000000
#define	TCX_USER_BLIT	0x20000000
#define	TCX_USER_RDFB32	0x28000000
#define	TCX_USER_RSTIP	0x30000000
#define	TCX_USER_RBLIT	0x38000000
#define	TCX_USER_TEC	0x70001000
#define	TCX_USER_BTREGS	0x70002000
#define	TCX_USER_THC	0x70004000
#define	TCX_USER_DHC	0x70008000
#define	TCX_USER_ALT	0x7000a000
#define	TCX_USER_UART	0x7000c000
#define	TCX_USER_VRT	0x7000e000
#define	TCX_USER_ROM	0x70010000

struct mmo {
	u_int	mo_uaddr;	/* user (virtual) address */
	u_int	mo_size;	/* size, or TCX_SZ_* value for video ram */
#define TCX_SZ_8  (-1) /* framebuffer size at 8bpp */
#define TCX_SZ_32 (-2) /* framebuffer size at 32bpp */
#define TCX_SZ_64 (-3) /* framebuffer size at 64bpp */
	u_int	mo_bank;	/* register bank number */
};

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 *
 * XXX	needs testing against `demanding' applications (e.g., aviator)
 */
int
tcxmmap(dev, off, prot)
	dev_t dev;
	int off, prot;
{
	struct tcx_softc *sc = tcx_cd.cd_devs[minor(dev)];
	bus_space_handle_t bh;
	struct sbus_reg *rr = sc->sc_physadr;
	struct mmo *mo;
	u_int u, sz;
	static struct mmo mmo[] = {
		{ TCX_USER_RAM, TCX_SZ_8, TCX_REG_DFB8 },
		{ TCX_USER_RAM24, TCX_SZ_32, TCX_REG_DFB24 },
		{ TCX_USER_RAM_COMPAT, TCX_SZ_8, TCX_REG_DFB8 },
		{ TCX_USER_STIP, TCX_SZ_64, TCX_REG_STIP },
		{ TCX_USER_BLIT, TCX_SZ_64, TCX_REG_BLIT },
		{ TCX_USER_RDFB32, TCX_SZ_32, TCX_REG_RDFB32 },
		{ TCX_USER_RSTIP, TCX_SZ_64, TCX_REG_RSTIP },
		{ TCX_USER_RBLIT, TCX_SZ_64, TCX_REG_RBLIT },
		{ TCX_USER_TEC, 1, TCX_REG_TEC },
		{ TCX_USER_BTREGS, 8192 /* XXX */, TCX_REG_CMAP },
		{ TCX_USER_THC, sizeof(struct tcx_thc), TCX_REG_THC },
		{ TCX_USER_DHC, 1, TCX_REG_DHC },
		{ TCX_USER_ALT, 1, TCX_REG_ALT },
		{ TCX_USER_ROM, 65536, TCX_REG_ROM },
	};
#define NMMO (sizeof mmo / sizeof *mmo)

	if (off & PGOFSET)
		panic("tcxmmap");

	/*
	 * Entries with size 0 map video RAM (i.e., the size in fb data).
	 *
	 * Since we work in pages, the fact that the map offset table's
	 * sizes are sometimes bizarre (e.g., 1) is effectively ignored:
	 * one byte is as good as one page.
	 */
	for (mo = mmo; mo < &mmo[NMMO]; mo++) {
		if ((u_int)off < mo->mo_uaddr)
			continue;
		u = off - mo->mo_uaddr;
		sz = mo->mo_size;
		switch (sz) {
			case TCX_SZ_8:
				if (sc->sc_8bit)
					sz = sc->sc_fb.fb_type.fb_size;
				else
					sz = sc->sc_fb.fb_type.fb_size >> 2;
				break;
			case TCX_SZ_32:
				if (sc->sc_8bit)
					continue;
				sz = sc->sc_fb.fb_type.fb_size;
				break;
			case TCX_SZ_64:
				if (sc->sc_8bit)
					continue;
				sz = sc->sc_fb.fb_type.fb_size << 1;
				break;
		}
		if (u < sz) {
			bus_type_t t = (bus_type_t)rr[mo->mo_bank].sbr_slot;
			bus_addr_t a = (bus_addr_t)rr[mo->mo_bank].sbr_offset;

			if (bus_space_mmap(sc->sc_bustag,
					   t,
					   a + u,
					   BUS_SPACE_MAP_LINEAR, &bh))
				return (-1);

			return ((int)bh);
		}
	}
	return (-1);	/* not a user-map offset */
}
