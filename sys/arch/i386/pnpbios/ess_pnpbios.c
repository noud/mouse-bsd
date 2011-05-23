/*	$NetBSD: ess_pnpbios.c,v 1.1 2000/02/15 17:12:54 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <i386/pnpbios/pnpbiosvar.h>

#include <dev/isa/essreg.h>
#include <dev/isa/essvar.h>

int ess_pnpbios_match __P((struct device *, struct cfdata *, void *));
void ess_pnpbios_attach __P((struct device *, struct device *, void *));

struct cfattach ess_pnpbios_ca = {
	sizeof(struct ess_softc), ess_pnpbios_match, ess_pnpbios_attach
};

int
ess_pnpbios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pnpbiosdev_attach_args *aa = aux;

	if (strcmp(aa->idstr, "ESS0104"))
		return (0);

	return (1);
}

void
ess_pnpbios_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ess_softc *sc = (void *)self;
	struct pnpbiosdev_attach_args *aa = aux;

	if (pnpbios_io_map(aa->pbt, aa->resc, 0, &sc->sc_iot, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_ic = aa->ic;

	/* XXX These are only for setting chip configuration registers. */
	pnpbios_getiobase(aa->pbt, aa->resc, 0, 0, &sc->sc_iobase);

	sc->sc_audio1.ist = IST_EDGE;
	sc->sc_audio2.ist = IST_EDGE;

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 0, &sc->sc_audio1.irq)) {
		printf(": can't get IRQ\n");
		return;
	}

	if (pnpbios_getirqnum(aa->pbt, aa->resc, 1, &sc->sc_audio2.irq))
		sc->sc_audio2.irq = -1;

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 0, &sc->sc_audio1.drq)) {
		printf(": can't get DMA channel\n");
		return;
	}

	if (pnpbios_getdmachan(aa->pbt, aa->resc, 1, &sc->sc_audio2.drq))
		sc->sc_audio2.drq = -1;

	printf("\n");
	pnpbios_print_devres(self, aa);

	printf("%s", self->dv_xname);

	if (!essmatch(sc)) {
		printf("%s: essmatch failed\n", sc->sc_dev.dv_xname);
		/* XXX should probably use "pnpbios_io_unmap", but it doesn't exist. */
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ESS_NPORT);
		return;
	}

	essattach(sc);
}

