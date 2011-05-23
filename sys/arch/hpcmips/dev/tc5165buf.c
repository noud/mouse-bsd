/*	$NetBSD: tc5165buf.c,v 1.4 2000/01/16 21:47:01 uch Exp $ */

/*
 * Copyright (c) 1999, 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Device driver for TOSHIBA TC5165BFTS, PHILIPS 74ALVC16241/245 
 * buffer chip
 */

#include "opt_tx39_debug.h"
#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>

#include <hpcmips/dev/tc5165bufvar.h>
#include <hpcmips/dev/skbdvar.h>

#define TC5165_ROW_MAX		16
#define TC5165_COLUMN_MAX	8

#ifdef TC5165DEBUG
#define	DPRINTF(arg) printf arg
#else
#define	DPRINTF(arg)
#endif

struct tc5165buf_chip {
	bus_space_tag_t		scc_cst;
	bus_space_handle_t	scc_csh;
	u_int16_t		scc_buf[TC5165_COLUMN_MAX];
	int			scc_enabled;
	int			scc_queued;
	
	struct skbd_controller	scc_controller;
};

struct tc5165buf_softc {
	struct device		sc_dev;
	struct tc5165buf_chip	*sc_chip;
	tx_chipset_tag_t	sc_tc;
	void			*sc_ih;
};

int	tc5165buf_match	__P((struct device*, struct cfdata*, void*));
void	tc5165buf_attach __P((struct device*, struct device*, void*));
int	tc5165buf_intr __P((void*));
int	tc5165buf_poll __P((void*));
void	tc5165buf_soft __P((void*));
void	tc5165buf_ifsetup __P((struct tc5165buf_chip*));

int	tc5165buf_input_establish __P((void*, int (*) __P((void*, int, int)),
				      void (*) __P((void*)), void*));

struct tc5165buf_chip tc5165buf_chip;

struct cfattach tc5165buf_ca = {
	sizeof(struct tc5165buf_softc), tc5165buf_match, tc5165buf_attach
};

int
tc5165buf_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
tc5165buf_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct cs_attach_args *ca = aux;
	struct tc5165buf_softc *sc = (void*)self;
	struct skbd_attach_args saa;

	printf(": ");
	sc->sc_tc = ca->ca_tc;
	sc->sc_chip = &tc5165buf_chip;
	
	sc->sc_chip->scc_cst = ca->ca_csio.cstag;

	if (bus_space_map(sc->sc_chip->scc_cst, ca->ca_csio.csbase, 
			  ca->ca_csio.cssize, 0, &sc->sc_chip->scc_csh)) {
		printf("can't map i/o space\n");
		return;
	}

	sc->sc_chip->scc_enabled = 0;

	if (ca->ca_irq1 != -1) {
		sc->sc_ih = tx_intr_establish(sc->sc_tc, ca->ca_irq1,
					      IST_EDGE, IPL_TTY, 
					      tc5165buf_intr, sc);
		printf("interrupt mode");
	} else {
		sc->sc_ih = tx39_poll_establish(sc->sc_tc, 1, IPL_TTY, 
						tc5165buf_intr, sc);
		printf("polling mode");
	}

	if (!sc->sc_ih) {
		printf(" can't establish interrupt\n");
		return;
	}

	printf("\n");

	/* setup upper interface */
	tc5165buf_ifsetup(sc->sc_chip);
	
	saa.saa_ic = &sc->sc_chip->scc_controller;

	config_found(self, &saa, skbd_print);
}

void
tc5165buf_ifsetup(scc)
	struct tc5165buf_chip *scc;
{
	scc->scc_controller.skif_v		= scc;

	scc->scc_controller.skif_establish	= tc5165buf_input_establish;
	scc->scc_controller.skif_poll		= tc5165buf_poll;
}

int
tc5165buf_cnattach(addr)
	paddr_t addr;
{
	struct tc5165buf_chip *scc = &tc5165buf_chip;
	
	scc->scc_csh = MIPS_PHYS_TO_KSEG1(addr);

	tc5165buf_ifsetup(scc);

	skbd_cnattach(&scc->scc_controller);

	return 0;
}

int
tc5165buf_input_establish(ic, inputfunc, inputhookfunc, arg)
	void *ic;
	int (*inputfunc) __P((void*, int, int));
	void (*inputhookfunc) __P((void*));
	void *arg;
{
	struct tc5165buf_chip *scc = ic;

	/* setup lower interface */

	scc->scc_controller.sk_v = arg;

	scc->scc_controller.sk_input = inputfunc;
	scc->scc_controller.sk_input_hook = inputhookfunc;
	
	scc->scc_enabled = 1;

	return 0;
}

int
tc5165buf_intr(arg)
	void *arg;
{
	struct tc5165buf_softc *sc = arg;
	struct tc5165buf_chip *scc = sc->sc_chip;

	if (!scc->scc_enabled || scc->scc_queued)
		return 0;
	
	scc->scc_queued = 1;
	timeout(tc5165buf_soft, scc, 1);
	
	return 0;
}

int
tc5165buf_poll(arg)
	void *arg;
{
	struct tc5165buf_chip *scc = arg;

	if (!scc->scc_enabled)
		return POLL_CONT;

	tc5165buf_soft(arg);

	return POLL_CONT;
}

void
tc5165buf_soft(arg)
	void *arg;
{
	struct tc5165buf_chip *scc = arg;
	bus_space_tag_t t = scc->scc_cst;
	bus_space_handle_t h = scc->scc_csh;
	u_int16_t mask, rpat, edge;
	int i, j, type, val;
	skbd_tag_t controller;
	int s;

	controller = &scc->scc_controller;
	skbd_input_hook(controller);

	/* clear scanlines */
	(void)bus_space_read_2(t, h, 0);
	delay(3);

	for (i = 0; i < TC5165_COLUMN_MAX; i++) {
		rpat = bus_space_read_2(t, h, 2 << i);
		delay(3);
		(void)bus_space_read_2(t, h, 0);
		delay(3);
		if ((edge = (rpat ^ scc->scc_buf[i]))) {
			scc->scc_buf[i] = rpat;
			for (j = 0, mask = 1; j < TC5165_ROW_MAX; 
			     j++, mask <<= 1) {
				if (mask & edge) {
					type = mask & rpat ? 1 : 0;
					val = j * TC5165_COLUMN_MAX + i;
					DPRINTF(("%d %d\n", j, i));
					skbd_input(controller, type, val);
				}
			}
		}
	}

	s = spltty();
	scc->scc_queued = 0;
	splx(s);
}
