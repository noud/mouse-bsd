/* $NetBSD: if_awi_pcmcia.c,v 1.12 2000/02/17 15:58:32 sommerfeld Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Bill Sommerfeld
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
 * PCMCIA attachment for BayStack 650 802.11FH PCMCIA card,
 * based on the AMD 79c930 802.11 controller chip.
 *
 * This attachment can probably be trivally adapted for other FH and
 * DS cards based on the same chipset.
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/am79c930reg.h>
#include <dev/ic/am79c930var.h>
#include <dev/ic/awireg.h>
#include <dev/ic/awivar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

int	awi_pcmcia_match __P((struct device *, struct cfdata *, void *));
void	awi_pcmcia_attach __P((struct device *, struct device *, void *));
int	awi_pcmcia_detach __P((struct device *, int));

int	awi_pcmcia_get_enaddr __P((struct pcmcia_tuple *, void *));
int	awi_pcmcia_enable __P((struct awi_softc *));
void	awi_pcmcia_disable __P((struct awi_softc *));

struct awi_pcmcia_softc {
	struct awi_softc sc_awi;		/* real "awi" softc */

	/* PCMCIA-specific goo */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	struct pcmcia_mem_handle sc_memh;	/* PCMCIA mem space info */
	int sc_io_window;			/* our i/o window */
	int sc_mem_window;			/* our memory window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
};

int	awi_pcmcia_find __P((struct awi_pcmcia_softc *,
    struct pcmcia_attach_args *, struct pcmcia_config_entry *));

struct cfattach awi_pcmcia_ca = {
	sizeof(struct awi_pcmcia_softc),
	awi_pcmcia_match, awi_pcmcia_attach,
	awi_pcmcia_detach, awi_activate
};

#if __NetBSD_Version__ <= 104120000
#define	PCMCIA_VENDOR_BAY	0x01eb	/* Bay Networks */
#define	PCMCIA_PRODUCT_BAY_STACK_650	0x804
#endif

/*
 *  XXX following is common to most PCMCIA NIC's and belongs
 * in common code
 */

struct awi_pcmcia_get_enaddr_args {
	u_int8_t enaddr[ETHER_ADDR_LEN];
};
int	awi_pcmcia_get_enaddr __P((struct pcmcia_tuple *, void *));

struct awi_pcmcia_product {
	u_int32_t	app_vendor;	/* vendor ID */
	u_int32_t	app_product;	/* product ID */
	const char	*app_cisinfo[4]; /* CIS information */
	const char	*app_name;	/* product name */
} awi_pcmcia_products[] = {
	{ PCMCIA_VENDOR_BAY,		PCMCIA_PRODUCT_BAY_STACK_650,
	  PCMCIA_CIS_BAY_STACK_650,	PCMCIA_STR_BAY_STACK_650 },

#ifdef notyet
	{ PCMCIA_VENDOR_MELCO,		PCMCIA_PRODUCT_MELCO_WLI_PCM,
	  PCMCIA_CIS_MELCO_WLI_PCM,	PCMCIA_STR_MELCO_WLI_PCM },

	{ PCMCIA_VENDOR_ICOM,		PCMCIA_PRODUCT_ICOM_SL200,
	  PCMCIA_CIS_ICOM_SL200,	PCMCIA_STR_ICOM_SL200 },
#endif

	{ 0,				0,
	  { NULL, NULL, NULL, NULL },	NULL },
};

struct awi_pcmcia_product *
	awi_pcmcia_lookup __P((struct pcmcia_attach_args *));

struct awi_pcmcia_product *
awi_pcmcia_lookup(pa)
	struct pcmcia_attach_args *pa;
{
	struct awi_pcmcia_product *app;

	for (app = awi_pcmcia_products; app->app_name != NULL; app++) {
		/* match by vendor/product id */
		if (pa->manufacturer != PCMCIA_VENDOR_INVALID &&
		    pa->manufacturer == app->app_vendor &&
		    pa->product != PCMCIA_PRODUCT_INVALID &&
		    pa->product == app->app_product)
			return (app);

		/* match by CIS information */
		if (pa->card->cis1_info[0] != NULL &&
		    app->app_cisinfo[0] != NULL &&
		    strcmp(pa->card->cis1_info[0], app->app_cisinfo[0]) == 0 &&
		    pa->card->cis1_info[1] != NULL &&
		    app->app_cisinfo[1] != NULL &&
		    strcmp(pa->card->cis1_info[1], app->app_cisinfo[1]) == 0)
			return (app);
	}

	return (NULL);
}

int
awi_pcmcia_enable(sc)
	struct awi_softc *sc;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	/* establish the interrupt. */
	sc->sc_ih = pcmcia_intr_establish(pf, IPL_NET, awi_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	return (pcmcia_function_enable(pf));
}

void
awi_pcmcia_disable(sc)
	struct awi_softc *sc;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)sc;
	struct pcmcia_function *pf = psc->sc_pf;

	pcmcia_function_disable(pf);
	pcmcia_intr_disestablish(pf, sc->sc_ih);
}

int
awi_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (awi_pcmcia_lookup(pa) != NULL)
		return (1);

	return (0);
}

int
awi_pcmcia_find(psc, pa, cfe)
	struct awi_pcmcia_softc *psc;
	struct pcmcia_attach_args *pa;
	struct pcmcia_config_entry *cfe;
{
	struct awi_softc *sc = &psc->sc_awi;
	int fail = 0;
	u_int8_t version[AWI_BANNER_LEN];

	/*
	 * see if we can read the firmware version sanely
	 * through the i/o ports.
	 * if not, try a different CIS string..
	 */
	if (pcmcia_io_alloc(psc->sc_pf, cfe->iospace[0].start,
	    cfe->iospace[0].length, cfe->iospace[0].length,
	    &psc->sc_pcioh) != 0)
		goto fail;

	if (pcmcia_io_map(psc->sc_pf, PCMCIA_WIDTH_AUTO, 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window))
		goto fail_io_free;

	/* Enable the card. */
	pcmcia_function_init(psc->sc_pf, cfe);
	if (pcmcia_function_enable(psc->sc_pf))
		goto fail_io_unmap;

	sc->sc_chip.sc_iot = psc->sc_pcioh.iot;
	sc->sc_chip.sc_ioh = psc->sc_pcioh.ioh;
	am79c930_chip_init(&sc->sc_chip, 0);

	DELAY(100);

	awi_read_bytes(sc, AWI_BANNER, version, AWI_BANNER_LEN);

	if (memcmp(version, "PCnetMobile:", 12) == 0)
		return (0);

	fail++;
	pcmcia_function_disable(psc->sc_pf);

 fail_io_unmap:
	fail++;
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	psc->sc_io_window = -1;

 fail_io_free:
	fail++;
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
 fail:
	fail++;
	return (fail);
}



void
awi_pcmcia_attach(parent, self, aux)
	struct device  *parent, *self;
	void           *aux;
{
	struct awi_pcmcia_softc *psc = (void *)self;
	struct awi_softc *sc = &psc->sc_awi;
	struct awi_pcmcia_product *app;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct awi_pcmcia_get_enaddr_args pgea;
	bus_addr_t memoff;
	int rv;

	psc->sc_mem_window = -1;
	psc->sc_io_window = -1;
#if 0
	int i, j;

	for (cfe = pa->pf->cfe_head.sqh_first, i=0;
	     cfe != NULL;
	     cfe = cfe->cfe_list.sqe_next, i++) {
		printf("%d: %d memspaces, %d iospaces\n",
		    i, cfe->num_memspace, cfe->num_iospace);
		printf("%d: number %d flags %x iftype %d iomask %lx "
		    "irqmask %x maxtwins %x\n",
		    i, cfe->number, cfe->flags, cfe->iftype, cfe->iomask,
		    cfe->irqmask, cfe->maxtwins);
		for (j = 0; j < cfe->num_memspace; j++) {
			printf("%d: mem %d: len %lx card %lx host %lx\n",
			    i, j,
			    cfe->memspace[j].length,
			    cfe->memspace[j].cardaddr,
			    cfe->memspace[j].hostaddr);
		}
		for (j = 0; j < cfe->num_iospace; j++) {
			printf("%d: io %d: len %lx start %lx\n",
			    i, j,
			    cfe->iospace[j].length,
			    cfe->iospace[j].start);
		}
	}
#endif

	app = awi_pcmcia_lookup(pa);
	if (app == NULL)
		panic("awi_pcmcia_attach: impossible");

	psc->sc_pf = pa->pf;

	for (cfe = SIMPLEQ_FIRST(&pa->pf->cfe_head); cfe != NULL;
	     cfe = SIMPLEQ_NEXT(cfe, cfe_list)) {
		if (cfe->iftype != PCMCIA_IFTYPE_IO)
			continue;
		if (cfe->num_iospace < 1)
			continue;
		if (cfe->iospace[0].length < AM79C930_IO_SIZE)
			continue;

		if (awi_pcmcia_find(psc, pa, cfe) == 0)
			break;
	}
	if (cfe == NULL) {
		printf(": no suitable CIS info found\n");
		return;
	}

	sc->sc_enabled = 1;
	sc->sc_state = AWI_ST_SELFTEST;
	printf(": %s\n", app->app_name);

	if (pcmcia_mem_alloc(psc->sc_pf, AM79C930_MEM_SIZE, &psc->sc_memh) != 0) {
		printf("%s: unable to allocate memory space; using i/o only\n",
		    sc->sc_dev.dv_xname);
	} else if (pcmcia_mem_map(psc->sc_pf,
	    PCMCIA_WIDTH_MEM8|PCMCIA_MEM_COMMON, AM79C930_MEM_BASE,
	    AM79C930_MEM_SIZE, &psc->sc_memh, &memoff, &psc->sc_mem_window)) {
		printf("%s: unable to map memory space; using i/o only\n",
		    sc->sc_dev.dv_xname);
		pcmcia_mem_free(psc->sc_pf, &psc->sc_memh);
	} else {
		sc->sc_chip.sc_memt = psc->sc_memh.memt;
		sc->sc_chip.sc_memh = psc->sc_memh.memh;
		am79c930_chip_init(&sc->sc_chip, 1);
	}

	sc->sc_chip.sc_bustype = AM79C930_BUS_PCMCIA;

	sc->sc_enable = awi_pcmcia_enable;
	sc->sc_disable = awi_pcmcia_disable;

	/* Read station address. */
	rv = pcmcia_scan_cis(parent, awi_pcmcia_get_enaddr, &pgea);
	if (rv == -1) {
		printf("%s: Couldn't read CIS to get ethernet address\n",
		    sc->sc_dev.dv_xname);
		return;
	} else if (rv == 0) {
		printf("%s: Couldn't get ethernet address from CIS\n",
		    sc->sc_dev.dv_xname);
		return;
	}

#ifdef DIAGNOSTIC
	if (rv != 1) {
		printf("%s: pcmcia_scan_cis returns %d\n", sc->sc_dev.dv_xname,
		    rv);
		panic("awi_pcmcia_attach");
	}
	printf("%s: Ethernet address from CIS: %s\n",
	    sc->sc_dev.dv_xname, ether_sprintf(pgea.enaddr));
#endif

	awi_attach(sc, pgea.enaddr);

	sc->sc_state = AWI_ST_OFF;

	sc->sc_enabled = 0;
	pcmcia_function_disable(psc->sc_pf);
}


int
awi_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct awi_pcmcia_softc *psc = (struct awi_pcmcia_softc *)self;

	/* Unmap our i/o window. */
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);

	/* Free our i/o space. */
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);

	if (psc->sc_mem_window != -1) {
		pcmcia_mem_unmap (psc->sc_pf, psc->sc_mem_window);
		pcmcia_mem_free (psc->sc_pf, &psc->sc_memh);
	}
	
	return awi_detach(&psc->sc_awi);
}

/*
 * XXX copied verbatim from if_mbe_pcmcia.c.
 * this function should be in common pcmcia code..
 */

int
awi_pcmcia_get_enaddr(tuple, arg)
	struct pcmcia_tuple *tuple;
	void *arg;
{
	struct awi_pcmcia_get_enaddr_args *p = arg;
	int i;

	if (tuple->code == PCMCIA_CISTPL_FUNCE) {
		if (tuple->length < 2) /* sub code and ether addr length */
			return (0);

		if ((pcmcia_tuple_read_1(tuple, 0) !=
			PCMCIA_TPLFE_TYPE_LAN_NID) ||
		    (pcmcia_tuple_read_1(tuple, 1) != ETHER_ADDR_LEN))
			return (0);

		for (i = 0; i < ETHER_ADDR_LEN; i++)
			p->enaddr[i] = pcmcia_tuple_read_1(tuple, i + 2);
		return (1);
	}
	return (0);
}
