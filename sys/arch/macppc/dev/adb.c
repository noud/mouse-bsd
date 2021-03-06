/*	$NetBSD: adb.c,v 1.6 1999/08/16 06:28:09 tsubai Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
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
 *	This product includes software developed by Bradley A. Grantham.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/akbdvar.h>
#include <macppc/dev/viareg.h>

#include "aed.h"

/*
 * Function declarations.
 */
static int	adbmatch __P((struct device *, struct cfdata *, void *));
static void	adbattach __P((struct device *, struct device *, void *));
static int	adbprint __P((void *, const char *));

/*
 * Global variables.
 */
int     adb_polling = 0;	/* Are we polling?  (Debugger mode) */
int     adb_initted = 0;	/* adb_init() has completed successfully */
#ifdef ADB_DEBUG
int	adb_debug = 0;		/* Output debugging messages */
#endif /* ADB_DEBUG */

/*
 * Driver definition.
 */
struct cfattach adb_ca = {
	sizeof(struct adb_softc), adbmatch, adbattach
};

extern int adbHardware;

static int
adbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (ca->ca_nreg < 8)
		return 0;

	if (ca->ca_nintr < 4)
		return 0;

	if (strcmp(ca->ca_name, "via-cuda") == 0)
		return 1;

	if (strcmp(ca->ca_name, "via-pmu") == 0)
		return 1;

	return 0;
}

static void
adbattach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;
{
	struct adb_softc *sc = (struct adb_softc *)self;
	struct confargs *ca = aux;

	ADBDataBlock adbdata;
	struct adb_attach_args aa_args;
	int totaladbs;
	int adbindex, adbaddr;

	extern adb_intr();
	extern volatile u_char *Via1Base;

	ca->ca_reg[0] += ca->ca_baseaddr;

	sc->sc_regbase = mapiodev(ca->ca_reg[0], ca->ca_reg[1]);
	Via1Base = sc->sc_regbase;

	if (strcmp(ca->ca_name, "via-cuda") == 0)
		adbHardware = ADB_HW_CUDA;
	else if (strcmp(ca->ca_name, "via-pmu") == 0)
		adbHardware = ADB_HW_PB;

	adb_polling = 1;
	ADBReInit();

	intr_establish(ca->ca_intr[0], IST_LEVEL, IPL_HIGH, adb_intr, sc);

#ifdef ADB_DEBUG
	if (adb_debug)
		printf("adb: done with ADBReInit\n");
#endif
	totaladbs = CountADBs();

	printf(" irq %d", ca->ca_intr[0]);
	printf(": %d targets\n", totaladbs);

#if NAED > 0
	/* ADB event device for compatibility */
	aa_args.origaddr = 0;
	aa_args.adbaddr = 0;
	aa_args.handler_id = 0;
	(void)config_found(self, &aa_args, adbprint);
#endif

	/* for each ADB device */
	for (adbindex = 1; adbindex <= totaladbs; adbindex++) {
		/* Get the ADB information */
		adbaddr = GetIndADB(&adbdata, adbindex);

		aa_args.origaddr = adbdata.origADBAddr;
		aa_args.adbaddr = adbaddr;
		aa_args.handler_id = adbdata.devType;

		(void)config_found(self, &aa_args, adbprint);
	}

	if (adbHardware == ADB_HW_CUDA)
		adb_cuda_autopoll();
	adb_polling = 0;
}

int
adbprint(args, name)
        void *args;
        const char *name;
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)args;
	int rv = UNCONF;

	if (name) {	/* no configured device matched */
		rv = UNSUPP; /* most ADB device types are unsupported */

		/* print out what kind of ADB device we have found */
		printf("%s addr %d: ", name, aa_args->origaddr);
		switch(aa_args->origaddr) {
#ifdef DIAGNOSTIC
		case 0:
			printf("ADB event device");
			rv = UNCONF;
			break;
		case ADBADDR_SECURE:
			printf("security dongle (%d)", aa_args->handler_id);
			break;
#endif
		case ADBADDR_MAP:
			printf("mapped device (%d)", aa_args->handler_id);
			rv = UNCONF;
			break;
		case ADBADDR_REL:
			printf("relative positioning device (%d)",
			    aa_args->handler_id);
			rv = UNCONF;
			break;
#ifdef DIAGNOSTIC
		case ADBADDR_ABS:
			switch (aa_args->handler_id) {
			case ADB_ARTPAD:
				printf("WACOM ArtPad II");
				break;
			default:
				printf("absolute positioning device (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		case ADBADDR_DATATX:
			printf("data transfer device (modem?) (%d)",
			    aa_args->handler_id);
			break;
		case ADBADDR_MISC:
			switch (aa_args->handler_id) {
			case ADB_POWERKEY:
				printf("Sophisticated Circuits PowerKey");
				break;
			default:
				printf("misc. device (remote control?) (%d)",
				    aa_args->handler_id);
				break;
			}
			break;
		default:
			printf("unknown type device, (handler %d)",
			    aa_args->handler_id);
			break;
#endif /* DIAGNOSTIC */
		}
	} else		/* a device matched and was configured */
                printf(" addr %d: ", aa_args->origaddr);

	return (rv);
}

void
extdms_complete(buffer, compdata, cmd)
	caddr_t buffer, compdata;
	int cmd;
{
	long *p = (long *)compdata;

	*p= -1;
}
