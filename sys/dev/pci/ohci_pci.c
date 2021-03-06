/*	$NetBSD: ohci_pci.c,v 1.14 1999/10/12 11:21:24 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
 * Carlstedt Research & Technology.
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
 * USB Open Host Controller driver.
 *
 * OHCI spec: http://www.intel.com/design/usb/ohci11d.pdf
 * USB spec: http://www.teleport.com/cgi-bin/mailmerge.cgi/~usb/cgiform.tpl
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

int	ohci_pci_match __P((struct device *, struct cfdata *, void *));
void	ohci_pci_attach __P((struct device *, struct device *, void *));
int	ohci_pci_detach __P((device_ptr_t, int));

struct ohci_pci_softc {
	ohci_softc_t		sc;
	pci_chipset_tag_t	sc_pc;
	bus_size_t		sc_size;
	void 			*sc_ih;		/* interrupt vectoring */
};

struct cfattach ohci_pci_ca = {
	sizeof(struct ohci_pci_softc), ohci_pci_match, ohci_pci_attach,
	ohci_pci_detach, ohci_activate
};

int
ohci_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_SERIALBUS &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_SERIALBUS_USB &&
	    PCI_INTERFACE(pa->pa_class) == PCI_INTERFACE_OHCI)
		return (1);

	return (0);
}

void
ohci_pci_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ohci_pci_softc *sc = (struct ohci_pci_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	char devinfo[256];
	usbd_status r;
	char *vendor;
	char *devname = sc->sc.sc_bus.bdev.dv_xname;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	/* Map I/O registers */
	if (pci_mapreg_map(pa, PCI_CBMEM, PCI_MAPREG_TYPE_MEM, 0,
			   &sc->sc.iot, &sc->sc.ioh, NULL, &sc->sc_size)) {
		printf("%s: can't map mem space\n", devname);
		return;
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
			  OHCI_ALL_INTRS);

	sc->sc_pc = pc;
	sc->sc.sc_bus.dmatag = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", devname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_USB, ohci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt", devname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", devname, intrstr);

	/* Figure out vendor for root hub descriptor. */
	vendor = pci_findvendor(pa->pa_id);
	sc->sc.sc_id_vendor = PCI_VENDOR(pa->pa_id);
	if (vendor)
		strncpy(sc->sc.sc_vendor, vendor,
			sizeof(sc->sc.sc_vendor) - 1);
	else
		sprintf(sc->sc.sc_vendor, "vendor 0x%04x",
			PCI_VENDOR(pa->pa_id));

	r = ohci_init(&sc->sc);
	if (r != USBD_NORMAL_COMPLETION) {
		printf("%s: init failed, error=%d\n", devname, r);
		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *)sc, &sc->sc.sc_bus,
				       usbctlprint);
}

int
ohci_pci_detach(self, flags)
	device_ptr_t self;
	int flags;
{
	struct ohci_pci_softc *sc = (struct ohci_pci_softc *)self;
	int rv;

	rv = ohci_detach(&sc->sc, flags);
	if (rv)
		return (rv);
	if (sc->sc_ih) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = 0;
	}
	if (sc->sc_size) {
		bus_space_unmap(sc->sc.iot, sc->sc.ioh, sc->sc_size);
		sc->sc_size = 0;
	}
	return (0);
}
