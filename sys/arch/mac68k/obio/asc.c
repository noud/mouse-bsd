/*	$NetBSD: asc.c,v 1.33 1999/07/08 18:08:54 thorpej Exp $	*/

/*
 * Copyright (C) 1997 Scott Reynolds
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ASC driver code and console bell support
 */

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/viareg.h>

#include <mac68k/obio/ascvar.h>
#include <mac68k/obio/obiovar.h>

#define	MAC68K_ASC_BASE		0x50f14000
#define	MAC68K_IIFX_ASC_BASE	0x50f10000
#define	MAC68K_ASC_LEN		0x01000

#ifdef DEBUG
#define ASC_DEBUG
#endif

#ifdef ASC_DEBUG
int	asc_debug = 0;		/* non-zero enables debugging output */
#endif

static u_int8_t		asc_wave_tab[0x800];

static int	asc_ring_bell __P((void *, int, int, int));
static void	asc_stop_bell __P((void *));
#if __notyet__
static void	asc_intr_enable __P((void));
static void	asc_intr __P((void *));
#endif

static int	ascmatch __P((struct device *, struct cfdata *, void *));
static void	ascattach __P((struct device *, struct device *, void *));

struct cfattach asc_ca = {
	sizeof(struct asc_softc), ascmatch, ascattach
};

extern struct cfdriver asc_cd;

static int
ascmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_addr_t addr;
	bus_space_handle_t bsh;
	int rval = 0;

	if (oa->oa_addr != (-1))
		addr = (bus_addr_t)oa->oa_addr;
	else if (current_mac_model->machineid == MACH_MACIIFX)
		addr = (bus_addr_t)MAC68K_IIFX_ASC_BASE;
	else
		addr = (bus_addr_t)MAC68K_ASC_BASE;

	if (bus_space_map(oa->oa_tag, addr, MAC68K_ASC_LEN, 0, &bsh))
		return (0);

	if (mac68k_bus_space_probe(oa->oa_tag, bsh, 0, 1))
		rval = 1;
	else
		rval = 0;

	bus_space_unmap(oa->oa_tag, bsh, MAC68K_ASC_LEN);

	return rval;
}

static void
ascattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct asc_softc *sc = (struct asc_softc *)self;
	struct obio_attach_args *oa = (struct obio_attach_args *)aux;
	bus_addr_t addr;
	int i;

	sc->sc_tag = oa->oa_tag;
	if (oa->oa_addr != (-1))
		addr = (bus_addr_t)oa->oa_addr;
	else if (current_mac_model->machineid == MACH_MACIIFX)
		addr = (bus_addr_t)MAC68K_IIFX_ASC_BASE;
	else
		addr = (bus_addr_t)MAC68K_ASC_BASE;
	if (bus_space_map(sc->sc_tag, addr, MAC68K_ASC_LEN, 0,
	    &sc->sc_handle)) {
		printf(": can't map memory space\n");
		return;
	}
	sc->sc_open = 0;
	sc->sc_ringing = 0;

	for (i = 0; i < 256; i++) {	/* up part of wave, four voices? */
		asc_wave_tab[i] = i / 4;
		asc_wave_tab[i + 512] = i / 4;
		asc_wave_tab[i + 1024] = i / 4;
		asc_wave_tab[i + 1536] = i / 4;
	}
	for (i = 0; i < 256; i++) {	/* down part of wave, four voices? */
		asc_wave_tab[i + 256] = 0x3f - (i / 4);
		asc_wave_tab[i + 768] = 0x3f - (i / 4);
		asc_wave_tab[i + 1280] = 0x3f - (i / 4);
		asc_wave_tab[i + 1792] = 0x3f - (i / 4);
	}

	printf(": Apple Sound Chip");
	if (oa->oa_addr != (-1))
		printf(" at %x", oa->oa_addr);
	printf("\n");

	mac68k_set_bell_callback(asc_ring_bell, sc);
#if __notyet__
	if (mac68k_machine.aux_interrupts) {
		intr_establish((int (*)(void *))asc_intr, sc, 5);
	} else {
		via2_register_irq(VIA2_ASC, asc_intr, sc);
	}
	asc_intr_enable();
#endif
}

int
ascopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct asc_softc *sc;
	int unit;

	unit = ASCUNIT(dev);
	sc = asc_cd.cd_devs[unit];
	if (unit >= asc_cd.cd_ndevs)
		return (ENXIO);
	if (sc->sc_open)
		return (EBUSY);
	sc->sc_open = 1;

	return (0);
}

int
ascclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct asc_softc *sc;

	sc = asc_cd.cd_devs[ASCUNIT(dev)];
	sc->sc_open = 0;

	return (0);
}

int
ascread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (ENXIO);
}

int
ascwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (ENXIO);
}

int
ascioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct asc_softc *sc;
	int error;
	int unit = ASCUNIT(dev);

	sc = asc_cd.cd_devs[unit];
	error = 0;

	switch (cmd) {
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

int
ascpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return (events & (POLLOUT | POLLWRNORM));
}

int
ascmmap(dev, off, prot)
	dev_t dev;
	int off;
	int prot;
{
	int unit = ASCUNIT(dev);
	struct asc_softc *sc;
	paddr_t pa;

	sc = asc_cd.cd_devs[unit];
	if ((u_int)off < MAC68K_ASC_LEN) {
		(void) pmap_extract(pmap_kernel(), (vaddr_t)sc->sc_handle, &pa);
		return m68k_btop(pa + off);
	}

	return (-1);
}

static int
asc_ring_bell(arg, freq, length, volume)
	void *arg;
	int freq, length, volume;
{
	struct asc_softc *sc = (struct asc_softc *)arg;
	unsigned long cfreq;
	int i;

	if (!sc)
		return (ENODEV);

	if (sc->sc_ringing == 0) {

		bus_space_write_multi_1(sc->sc_tag, sc->sc_handle,
		    0, 0, 0x800);
		bus_space_write_region_1(sc->sc_tag, sc->sc_handle,
		    0, asc_wave_tab, 0x800);

		/* Fix this.  Need to find exact ASC sampling freq */
		cfreq = 65536 * freq / 466;

		/* printf("beep: from %d, %02x %02x %02x %02x\n",
		 * cur_beep.freq, (cfreq >> 24) & 0xff, (cfreq >> 16) & 0xff,
		 * (cfreq >> 8) & 0xff, (cfreq) & 0xff); */
		for (i = 0; i < 8; i++) {
			bus_space_write_1(sc->sc_tag, sc->sc_handle,
			    0x814 + 8 * i, (cfreq >> 24) & 0xff);
			bus_space_write_1(sc->sc_tag, sc->sc_handle,
			    0x815 + 8 * i, (cfreq >> 16) & 0xff);
			bus_space_write_1(sc->sc_tag, sc->sc_handle,
			    0x816 + 8 * i, (cfreq >> 8) & 0xff);
			bus_space_write_1(sc->sc_tag, sc->sc_handle,
			    0x817 + 8 * i, (cfreq) & 0xff);
		}		/* frequency; should put cur_beep.freq in here
				 * somewhere. */

		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x807, 3); /* 44 ? */
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x806,
		    255 * volume / 100);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x805, 0);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x80f, 0);
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x802, 2); /* sampled */
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x801, 2); /* enable sampled */
	}
	sc->sc_ringing++;
	timeout(asc_stop_bell, sc, length);

	return (0);
}

static void
asc_stop_bell(arg)
	void *arg;
{
	struct asc_softc *sc = (struct asc_softc *)arg;

	if (!sc)
		return;

	if (sc->sc_ringing > 1000 || sc->sc_ringing < 0)
		panic("bell got out of sync?");

	if (--sc->sc_ringing == 0)	/* disable ASC */
		bus_space_write_1(sc->sc_tag, sc->sc_handle, 0x801, 0);
}

#if __notyet__
static void
asc_intr_enable()
{
	int s;

	s = splhigh();
	if (VIA2 == VIA2OFF)
		via2_reg(vIER) = 0x80 | V2IF_ASC;
	else
		via2_reg(rIER) = 0x80 | V2IF_ASC;
	splx(s);
}


/*ARGSUSED*/
static void
asc_intr(arg)
        void *arg;
{
#ifdef ASC_DEBUG
	struct asc_softc *sc = (struct asc_softc *)arg;

	if (asc_debug)
		printf("asc_intr(%p)\n", sc);
#endif
}
#endif
