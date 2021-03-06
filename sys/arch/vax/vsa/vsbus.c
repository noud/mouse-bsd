/*	$NetBSD: vsbus.c,v 1.21 2000/01/24 02:40:35 matt Exp $ */
/*
 * Copyright (c) 1996, 1999 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *	This product includes software developed at Ludd, University of
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/device.h>
#include <sys/dkstat.h>
#include <sys/disklabel.h>
#include <sys/syslog.h>
#include <sys/stat.h>

#include <vm/vm.h>

#define	_VAX_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/nexus.h>

#include <machine/uvax.h>
#include <machine/ka410.h>
#include <machine/ka420.h>
#include <machine/ka43.h>

#include <machine/vsbus.h>

#include "ioconf.h"

int	vsbus_match	__P((struct device *, struct cfdata *, void *));
void	vsbus_attach	__P((struct device *, struct device *, void *));
int	vsbus_print	__P((void *, const char *));
int	vsbus_search	__P((struct device *, struct cfdata *, void *));

void	ka410_attach	__P((struct device *, struct device *, void *));
void	ka43_attach	__P((struct device *, struct device *, void *));

static struct vax_bus_dma_tag vsbus_bus_dma_tag = {
	0,
	0,
	0,
	0,
	0,
	0,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

extern struct vax_bus_space vax_mem_bus_space;

struct	vsbus_softc {
	struct	device sc_dev;
#if 0
	volatile struct vs_cpu *sc_cpu;
#endif
	u_char	*sc_intmsk;	/* Mask register */
	u_char	*sc_intclr;	/* Clear interrupt register */
	u_char	*sc_intreq;	/* Interrupt request register */
	u_char	sc_mask;	/* Interrupts to enable after autoconf */
	struct vax_bus_dma_tag sc_dmatag;
};

struct	cfattach vsbus_ca = {
	sizeof(struct vsbus_softc), vsbus_match, vsbus_attach
};

uint32_t *vsbus_iomap;

int
vsbus_print(aux, name)
	void *aux;
	const char *name;
{
	struct vsbus_attach_args *va = aux;

	printf(" csr 0x%lx vec %o ipl %x maskbit %d", va->va_paddr,
	    va->va_cvec & 511, va->va_br, va->va_maskno - 1);
	return(UNCONF);
}

int
vsbus_match(parent, cf, aux)
	struct	device	*parent;
	struct cfdata	*cf;
	void	*aux;
{
	if (vax_bustype == VAX_VSBUS)
		return 1;
	return 0;
}

void
vsbus_attach(parent, self, aux)
	struct	device	*parent, *self;
	void	*aux;
{
	struct	vsbus_softc *sc = (void *)self;
	vaddr_t temp;

	printf("\n");

	sc->sc_dmatag = vsbus_bus_dma_tag;

	switch (vax_boardtype) {
	case VAX_BTYP_49:
		temp = vax_map_physmem(0x25c00000, 1);
		sc->sc_intreq = (char *)temp + 12;
		sc->sc_intclr = (char *)temp + 12;
		sc->sc_intmsk = (char *)temp + 8;
		break;

	case VAX_BTYP_48:
	case VAX_BTYP_46:
		/* FALL THROUGH */

	default:
		temp = vax_map_physmem(VS_REGS, 1);
		sc->sc_intreq = (char *)temp + 15;
		sc->sc_intclr = (char *)temp + 15;
		sc->sc_intmsk = (char *)temp + 12;
		break;
	}

	/*
	 * First: find which interrupts we won't care about.
	 * There are interrupts that interrupt on a periodic basic
	 * that we don't want to interfere with the rest of the
	 * interrupt probing.
	 */
	*sc->sc_intmsk = 0;
	*sc->sc_intclr = 0xff;
	DELAY(1000000); /* Wait a second */
	sc->sc_mask = *sc->sc_intreq;
	printf("%s: interrupt mask %x\n", self->dv_xname, sc->sc_mask);
	/*
	 * now check for all possible devices on this "bus"
	 */
	config_search(vsbus_search, self, NULL);

	/* Autoconfig finished, enable interrupts */
	*sc->sc_intmsk = ~sc->sc_mask;
}

int
vsbus_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	vsbus_softc *sc = (void *)parent;
	struct	vsbus_attach_args va;
	int i, vec, br;
	u_char c;

	va.va_paddr = cf->cf_loc[0];
	va.va_addr = vax_map_physmem(va.va_paddr, 1);
	va.va_dmat = &sc->sc_dmatag;
	va.va_iot = &vax_mem_bus_space;

	*sc->sc_intmsk = 0;
	*sc->sc_intclr = 0xff;
	scb_vecref(0, 0); /* Clear vector ref */

	i = (*cf->cf_attach->ca_match) (parent, cf, &va);
	vax_unmap_physmem(va.va_addr, 1);
	c = *sc->sc_intreq & ~sc->sc_mask;
	if (i == 0)
		goto forgetit;
	if (i > 10)
		c = sc->sc_mask; /* Fooling interrupt */
	else if (c == 0)
		goto forgetit;

	*sc->sc_intmsk = c;
	DELAY(100);
	*sc->sc_intmsk = 0;
	va.va_maskno = ffs((u_int)c);
	i = scb_vecref(&vec, &br);
	if (i == 0)
		goto fail;
	if (vec == 0)
		goto fail;

	va.va_br = br;
	va.va_cvec = vec;

	config_attach(parent, cf, &va, vsbus_print);
	return 0;

fail:
	printf("%s%d at %s csr %x %s\n",
	    cf->cf_driver->cd_name, cf->cf_unit, parent->dv_xname,
	    cf->cf_loc[0], (i ? "zero vector" : "didn't interrupt"));
forgetit:
	return 0;
}

/*
 * Sets a new interrupt mask. Returns the old one.
 * Works like spl functions.
 */
unsigned char
vsbus_setmask(mask)
	unsigned char mask;
{
	struct vsbus_softc *sc = vsbus_cd.cd_devs[0];
	unsigned char ch;

	ch = *sc->sc_intmsk;
	*sc->sc_intmsk = mask;
	return ch;
}

/*
 * Clears the interrupts in mask.
 */
void
vsbus_clrintr(mask)
	unsigned char mask;
{
	struct vsbus_softc *sc = vsbus_cd.cd_devs[0];

	*sc->sc_intclr = mask;
}

/*
 * Copy data from/to a user process' space from the DMA area.
 * Use the physical memory directly.
 */
void
vsbus_copytoproc(p, from, to, len)
	struct proc *p;
	caddr_t from, to;
	int len;
{
	struct pte *pte;
	paddr_t pa;

	pte = uvtopte(TRUNC_PAGE(to), (&p->p_addr->u_pcb));
	if ((vaddr_t)to & PGOFSET) {
		int cz = ROUND_PAGE(to) - (vaddr_t)to;

		pa = (pte->pg_pfn << VAX_PGSHIFT) | (NBPG - cz) | KERNBASE;
		bcopy(from, (caddr_t)pa, min(cz, len));
		from += cz;
		to += cz;
		len -= cz;
		pte += 8; /* XXX */
	}
	while (len > 0) {
		pa = (pte->pg_pfn << VAX_PGSHIFT) | KERNBASE;
		bcopy(from, (caddr_t)pa, min(NBPG, len));
		from += NBPG;
		to += NBPG;
		len -= NBPG;
		pte += 8; /* XXX */
	}
}

void
vsbus_copyfromproc(p, from, to, len)
	struct proc *p;
	caddr_t from, to;
	int len;
{
	struct pte *pte;
	paddr_t pa;

	pte = uvtopte(TRUNC_PAGE(from), (&p->p_addr->u_pcb));
	if ((vaddr_t)from & PGOFSET) {
		int cz = ROUND_PAGE(from) - (vaddr_t)from;

		pa = (pte->pg_pfn << VAX_PGSHIFT) | (NBPG - cz) | KERNBASE;
		bcopy((caddr_t)pa, to, min(cz, len));
		from += cz;
		to += cz;
		len -= cz;
		pte += 8; /* XXX */
	}
	while (len > 0) {
		pa = (pte->pg_pfn << VAX_PGSHIFT) | KERNBASE;
		bcopy((caddr_t)pa, to, min(NBPG, len));
		from += NBPG;
		to += NBPG;
		len -= NBPG;
		pte += 8; /* XXX */
	}
}
