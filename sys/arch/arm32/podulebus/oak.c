/*	$NetBSD: oak.c,v 1.16 1998/11/19 21:44:59 thorpej Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
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
 * Oak SCSI 1 driver using the generic NCR5380 driver
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/io.h>
#include <machine/bootconfig.h>

#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>

void oak_attach __P((struct device *, struct device *, void *));
int  oak_match  __P((struct device *, struct cfdata *, void *));
static void oak_minphys __P((struct buf *bp));

struct scsipi_device oak_scsidev = {
	NULL,		/* use default error handler */
	NULL,		/* do not have a start functio */
	NULL,		/* have no async handler */
	NULL,		/* Use default done routine */
};

/*
 * Oak SCSI 1 softc structure.
 *
 * Contains the generic ncr5380 device node, podule information and global information
 * required by the driver.
 */

struct oak_softc {
	struct ncr5380_softc	sc_ncr5380;
	int			sc_podule_number;
	podule_t		*sc_podule;
};

struct cfattach oak_ca = {
	sizeof(struct oak_softc), oak_match, oak_attach
};

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
oak_match(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	struct podule_attach_args *pa = aux;

	if (matchpodule(pa, MANUFACTURER_OAK, PODULE_OAK_SCSI, -1) == 0)
		return(0);

	return(1);
}

/*
 * Card attach function
 *
 */

void
oak_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct oak_softc *sc = (struct oak_softc *)self;
	struct podule_attach_args *pa = aux;
	u_char *iobase;
	char hi_option[sizeof(sc->sc_ncr5380.sc_dev.dv_xname) + 8];

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	sc->sc_ncr5380.sc_flags |= NCR5380_FORCE_POLLING;
	sc->sc_ncr5380.sc_min_dma_len = 0;
	sc->sc_ncr5380.sc_no_disconnect = 0xff;
	sc->sc_ncr5380.sc_parity_disable = 0xff;

	sc->sc_ncr5380.sc_dma_alloc = NULL;
	sc->sc_ncr5380.sc_dma_free = NULL;
	sc->sc_ncr5380.sc_dma_poll = NULL;
	sc->sc_ncr5380.sc_dma_setup = NULL;
	sc->sc_ncr5380.sc_dma_start = NULL;
	sc->sc_ncr5380.sc_dma_eop = NULL;
	sc->sc_ncr5380.sc_dma_stop = NULL;
	sc->sc_ncr5380.sc_intr_on = NULL;
	sc->sc_ncr5380.sc_intr_off = NULL;

	iobase = (u_char *)pa->pa_podule->mod_base;
	sc->sc_ncr5380.sci_r0 = iobase + 0;
	sc->sc_ncr5380.sci_r1 = iobase + 4;
	sc->sc_ncr5380.sci_r2 = iobase + 8;
	sc->sc_ncr5380.sci_r3 = iobase + 12;
	sc->sc_ncr5380.sci_r4 = iobase + 16;
	sc->sc_ncr5380.sci_r5 = iobase + 20;
	sc->sc_ncr5380.sci_r6 = iobase + 24;
	sc->sc_ncr5380.sci_r7 = iobase + 28;

	sc->sc_ncr5380.sc_pio_in = ncr5380_pio_in;
	sc->sc_ncr5380.sc_pio_out = ncr5380_pio_out;

	sc->sc_ncr5380.sc_adapter.scsipi_cmd = ncr5380_scsi_cmd;
	sc->sc_ncr5380.sc_adapter.scsipi_minphys = oak_minphys;

	sc->sc_ncr5380.sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_ncr5380.sc_link.adapter_softc = sc;
	sc->sc_ncr5380.sc_link.scsipi_scsi.adapter_target = 7;
	sc->sc_ncr5380.sc_link.adapter = &sc->sc_ncr5380.sc_adapter;
	sc->sc_ncr5380.sc_link.device = &oak_scsidev;

	/* Provide an override for the host id */
	sprintf(hi_option, "%s.hostid", sc->sc_ncr5380.sc_dev.dv_xname);
	(void)get_bootconf_option(boot_args, hi_option,
	    BOOTOPT_TYPE_INT, &sc->sc_ncr5380.sc_link.scsipi_scsi.adapter_target);

	printf(" host=%d, using 8 bit PIO\n",
	    sc->sc_ncr5380.sc_link.scsipi_scsi.adapter_target);

	ncr5380_init(&sc->sc_ncr5380);
	ncr5380_reset_scsibus(&sc->sc_ncr5380);
	config_found(&sc->sc_ncr5380.sc_dev, &sc->sc_ncr5380.sc_link, scsiprint);
}

static void
oak_minphys(bp)
	struct buf *bp;
{
	return(minphys(bp));
}
