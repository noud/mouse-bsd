/*	$NetBSD: uk.c,v 1.27 1998/12/08 00:19:28 thorpej Exp $	*/

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

/*
 * Dummy driver for a device we can't identify.
 * Originally by Julian Elischer (julian@tfs.com)
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#define	UKUNIT(z)	(minor(z))

struct uk_softc {
	struct device sc_dev;

	struct scsipi_link *sc_link;	/* all the inter level info */
};

int ukmatch __P((struct device *, struct cfdata *, void *));
void ukattach __P((struct device *, struct device *, void *));

struct cfattach uk_scsibus_ca = {
	sizeof(struct uk_softc), ukmatch, ukattach
};
struct cfattach uk_atapibus_ca = {
	sizeof(struct uk_softc), ukmatch, ukattach
};

extern struct cfdriver uk_cd;

/*
 * This driver is so simple it uses all the default services
 */
struct scsipi_device uk_switch = {
	NULL,
	NULL,
	NULL,
	NULL,
};

cdev_decl(uk);

int
ukmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return (1);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
ukattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uk_softc *uk = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("ukattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	uk->sc_link = sc_link;
	sc_link->device = &uk_switch;
	sc_link->device_softc = uk;
	sc_link->openings = 1;

	printf("\n");
	printf("%s: unknown device\n", uk->sc_dev.dv_xname);
}

/*
 * open the device.
 */
int
ukopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit, error;
	struct uk_softc *uk;
	struct scsipi_link *sc_link;

	unit = UKUNIT(dev);
	if (unit >= uk_cd.cd_ndevs)
		return (ENXIO);
	uk = uk_cd.cd_devs[unit];
	if (uk == NULL)
		return (ENXIO);

	sc_link = uk->sc_link;

	SC_DEBUG(sc_link, SDEV_DB1,
	    ("ukopen: dev=0x%x (unit %d (of %d))\n", dev, unit,
		uk_cd.cd_ndevs));

	/*
	 * Only allow one at a time
	 */
	if (sc_link->flags & SDEV_OPEN) {
		printf("%s: already open\n", uk->sc_dev.dv_xname);
		return (EBUSY);
	}

	if ((error = scsipi_adapter_addref(sc_link)) != 0)
		return (error);
	sc_link->flags |= SDEV_OPEN;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	return (0);
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
ukclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct uk_softc *uk = uk_cd.cd_devs[UKUNIT(dev)];

	SC_DEBUG(uk->sc_link, SDEV_DB1, ("closing\n"));

	scsipi_wait_drain(uk->sc_link);

	scsipi_adapter_delref(uk->sc_link);
	uk->sc_link->flags &= ~SDEV_OPEN;

	return (0);
}

/*
 * Perform special action on behalf of the user
 * Only does generic scsi ioctls.
 */
int
ukioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	register struct uk_softc *uk = uk_cd.cd_devs[UKUNIT(dev)];

	return (scsipi_do_ioctl(uk->sc_link, dev, cmd, addr, flag, p));
}
