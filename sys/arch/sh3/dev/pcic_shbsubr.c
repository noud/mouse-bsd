/*	$NetBSD: pcic_shbsubr.c,v 1.2 1999/09/14 10:22:35 tsubai Exp $	*/

#define	SHPCICSUBRDEBUG

/*
 * Copyright (c) 1998 Bill Sommerfeld.  All rights reserved.
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
 * 4. The name of the author may not be used to endorse or promote products
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


#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/shbvar.h>

#if 0
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#endif

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <sh3/dev/shpcicreg.h>
#include <sh3/dev/shpcicvar.h>
#if 0
#include <dev/isa/i82365_isavar.h>
#endif

/*****************************************************************************
 * Configurable parameters.
 *****************************************************************************/

#include "opt_pcic_shb_alloc_iobase.h"
#include "opt_pcic_shb_alloc_iosize.h"
#include "opt_pcic_shb_intr_alloc_mask.h"

/*
 * Default I/O allocation range.  If both are set to non-zero, these
 * values will be used instead.  Otherwise, the code attempts to probe
 * the bus width.  Systems with 10 address bits should use 0x300 and 0xff.
 * Systems with 12 address bits (most) should use 0x400 and 0xbff.
 */

#ifndef PCIC_SHB_ALLOC_IOBASE
#define	PCIC_SHB_ALLOC_IOBASE	0
#endif

#ifndef PCIC_SHB_ALLOC_IOSIZE
#define	PCIC_SHB_ALLOC_IOSIZE	0
#endif

int	pcic_shb_alloc_iobase = PCIC_SHB_ALLOC_IOBASE;
int	pcic_shb_alloc_iosize = PCIC_SHB_ALLOC_IOSIZE;


/*
 * Default IRQ allocation bitmask.  This defines the range of allowable
 * IRQs for PCMCIA slots.  Useful if order of probing would screw up other
 * devices, or if PCIC hardware/cards have trouble with certain interrupt
 * lines.
 *
 * We disable IRQ 10 by default, since some common laptops (namely, the
 * NEC Versa series) reserve IRQ 10 for the docking station SCSI interface.
 */

#ifndef PCIC_SHB_INTR_ALLOC_MASK
#define	PCIC_SHB_INTR_ALLOC_MASK	0xfbff
#endif

extern int	pcic_shb_intr_alloc_mask;

/*****************************************************************************
 * End of configurable parameters.
 *****************************************************************************/

#ifdef SHPCICISADEBUG
int	shpcicsubr_debug = 0 /* XXX */ ;
#define	DPRINTF(arg) if (shpcicsubr_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

void pcic_shb_bus_width_probe (sc, iot, ioh, base, length)
	struct shpcic_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t base;
	u_int32_t length;
{
#if 0
	bus_space_handle_t ioh_high;
	int i, iobuswidth, tmp1, tmp2;

	/*
	 * figure out how wide the isa bus is.  Do this by checking if the
	 * pcic controller is mirrored 0x400 above where we expect it to be.
	 */

	iobuswidth = 12;

	/* Map i/o space. */
	if (bus_space_map(iot, base + 0x400, length, 0, &ioh_high)) {
		printf("%s: can't map high i/o space\n", sc->dev.dv_xname);
		return;
	}

	for (i = 0; i < SHPCIC_NSLOTS; i++) {
		if (sc->handle[i].flags & SHPCIC_FLAG_SOCKETP) {
			/*
			 * read the ident flags from the normal space and
			 * from the mirror, and compare them
			 */

			bus_space_write_1(iot, ioh, SHPCIC_REG_INDEX,
			    sc->handle[i].sock + SHPCIC_IDENT);
			tmp1 = bus_space_read_1(iot, ioh, SHPCIC_REG_DATA);

			bus_space_write_1(iot, ioh_high, SHPCIC_REG_INDEX,
			    sc->handle[i].sock + SHPCIC_IDENT);
			tmp2 = bus_space_read_1(iot, ioh_high, SHPCIC_REG_DATA);

			if (tmp1 == tmp2)
				iobuswidth = 10;
		}
	}

	bus_space_free(iot, ioh_high, length);

	/*
	 * XXX mycroft recommends I/O space range 0x400-0xfff .  I should put
	 * this in a header somewhere
	 */

	/*
	 * XXX some hardware doesn't seem to grok addresses in 0x400 range--
	 * apparently missing a bit or more of address lines. (e.g.
	 * CIRRUS_PD672X with Linksys EthernetCard ne2000 clone in TI
	 * TravelMate 5000--not clear which is at fault)
	 *
	 * Add a kludge to detect 10 bit wide buses and deal with them,
	 * and also a config file option to override the probe.
	 */

	if (iobuswidth == 10) {
		sc->iobase = 0x300;
		sc->iosize = 0x0ff;
	} else {
#if 0
		/*
		 * This is what we'd like to use, but...
		 */
		sc->iobase = 0x400;
		sc->iosize = 0xbff;
#else
		/*
		 * ...the above bus width probe doesn't always work.
		 * So, experimentation has shown the following range
		 * to not lose on systems that 0x300-0x3ff loses on
		 * (e.g. the NEC Versa 6030X).
		 */
		sc->iobase = 0x330;
		sc->iosize = 0x0cf;
#endif
	}

	DPRINTF(("%s: bus_space_alloc range 0x%04lx-0x%04lx (probed)\n",
	    sc->dev.dv_xname, (long) sc->iobase,

	    (long) sc->iobase + sc->iosize));

	if (pcic_shb_alloc_iobase && pcic_shb_alloc_iosize) {
		sc->iobase = pcic_shb_alloc_iobase;
		sc->iosize = pcic_shb_alloc_iosize;

		DPRINTF(("%s: bus_space_alloc range 0x%04lx-0x%04lx "
		    "(config override)\n", sc->dev.dv_xname, (long) sc->iobase,
		    (long) sc->iobase + sc->iosize));
	}
#endif
}


void *
pcic_shb_chip_intr_establish(pch, pf, ipl, fct, arg)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_function *pf;
	int ipl;
	int (*fct) __P((void *));
	void *arg;
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;
	int irq, ist;
	void *ih;

	if (pf->cfe->flags & PCMCIA_CFE_IRQLEVEL)
		ist = IST_LEVEL;
	else if (pf->cfe->flags & PCMCIA_CFE_IRQPULSE)
		ist = IST_PULSE;
	else
		ist = IST_LEVEL;

#if 0
	if (sh_intr_alloc(
	    SHPCIC_INTR_IRQ_VALIDMASK & pcic_shb_intr_alloc_mask, ist, &irq))
		return (NULL);
#else
	if (pf->pf_pcmh.addr == SHPCIC_SLOT1_ADDR)
		irq = SHPCIC_IRQ1;
	else if (pf->pf_pcmh.addr == SHPCIC_SLOT2_ADDR)
		irq = SHPCIC_IRQ2;
	else
		panic("illegal pcmcia slot address");

#endif
	if ((ih = shb_intr_establish(irq, ist, ipl, fct, arg)) == NULL)
		return (NULL);

#if 0
	reg = shpcic_read(h, SHPCIC_INTR);
	reg &= ~SHPCIC_INTR_IRQ_MASK;
	reg |= irq;
	shpcic_write(h, SHPCIC_INTR, reg);
#endif

	h->ih_irq = irq;

	printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

	return (ih);
}

void
pcic_shb_chip_intr_disestablish(pch, ih)
	pcmcia_chipset_handle_t pch;
	void *ih;
{
	struct shpcic_handle *h = (struct shpcic_handle *) pch;

	h->ih_irq = 0;
	shb_intr_disestablish(NULL, ih);
}
