/*	$OpenBSD: com_lbus.c,v 1.4 1997/05/01 15:18:13 pefo Exp $	*/
/*	NetBSD: com_isa.c,v 1.12 1998/08/15 17:47:17 mycroft Exp 	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>	/* XXX for isa_chipset_tag_t in com_softc */

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#include <dev/ic/ns16550reg.h>

extern struct arc_bus_space *arc_bus_com;	/* XXX */
extern int com_freq;

int	com_localbus_probe __P((struct device *, struct cfdata *, void *));
void	com_localbus_attach __P((struct device *, struct device *, void *));


struct cfattach com_pica_ca = {
	sizeof(struct com_softc), com_localbus_probe, com_localbus_attach
};

struct cfattach com_algor_ca = {
	sizeof(struct com_softc), com_localbus_probe, com_localbus_attach
};

int
com_localbus_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int iobase;
	int rv = 1;
	struct confargs *ca = aux;

	if(!BUS_MATCHNAME(ca, "com"))
		return(0);

	iot = arc_bus_com;
	iobase = (long)BUS_CVTADDR(ca);

	/* if it's in use as console, it's there. */
	if (!com_is_console(iot, iobase, 0)) {
		if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh)) {
			return 0;
		}
		rv = comprobe1(iot, ioh);
		bus_space_unmap(iot, ioh, COM_NPORTS);
	}
	return (rv);
}

void
com_localbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct com_softc *sc = (void *)self;
	int iobase;
	bus_space_tag_t iot;
	struct confargs *ca = aux;

	sc->sc_iobase = iobase = (bus_addr_t)BUS_CVTADDR(ca);
	sc->sc_iot = iot = arc_bus_com;

	if (!com_is_console(iot, iobase, &sc->sc_ioh) &&
	    bus_space_map(iot, iobase, COM_NPORTS, 0, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_frequency = com_freq;
	SET(sc->sc_hwflags, COM_HW_TXFIFO_DISABLE);
	BUS_INTR_ESTABLISH(ca, comintr, sc);

	com_attach_subr(sc);
}
