/*	$NetBSD: wdsc.c,v 1.15 1999/02/20 00:12:02 scw Exp $	*/

/*
 * Copyright (c) 1996 Steve Woodford
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 *  @(#)wdsc.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/autoconf.h>

#include <mvme68k/dev/dmavar.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/sbicreg.h>
#include <mvme68k/dev/sbicvar.h>
#include <mvme68k/dev/wdscreg.h>

void    wdsc_pcc_attach __P((struct device *, struct device *, void *));
int     wdsc_pcc_match  __P((struct device *, struct cfdata *, void *));

void    wdsc_enintr     __P((struct sbic_softc *));
int     wdsc_dmago      __P((struct sbic_softc *, char *, int, int));
int     wdsc_dmanext    __P((struct sbic_softc *));
void    wdsc_dmastop    __P((struct sbic_softc *));
int     wdsc_dmaintr    __P((void *));
int     wdsc_scsiintr   __P((void *));

struct scsipi_device wdsc_scsidev = {
    NULL,       /* use default error handler */
    NULL,       /* do not have a start functio */
    NULL,       /* have no async handler */
    NULL,       /* Use default done routine */
};

struct cfattach wdsc_pcc_ca = {
	sizeof(struct sbic_softc), wdsc_pcc_match, wdsc_pcc_attach
};

extern struct cfdriver wdsc_cd;


/*
 * Match for SCSI devices on the onboard WD33C93 chip
 */
int
wdsc_pcc_match(pdp, cf, auxp)
    struct device *pdp;
	struct cfdata *cf;
    void *auxp;
{
    struct pcc_attach_args *pa = auxp;

    if (strcmp(pa->pa_name, wdsc_cd.cd_name))
	return (0);

    pa->pa_ipl = cf->pcccf_ipl;
    return (1);
}

/*
 * Attach the wdsc driver
 */
void
wdsc_pcc_attach(pdp, dp, auxp)
    struct device *pdp, *dp;
    void *auxp;
{
    struct sbic_softc   *sc = (struct sbic_softc *)dp;
    struct pcc_attach_args *pa = auxp;
    static int          attached = 0;
    int tmp;

    if ( attached )
        panic("wdsc: Driver already attached!");

    attached = 1;

    sc->sc_enintr  = wdsc_enintr;
    sc->sc_dmago   = wdsc_dmago;
    sc->sc_dmanext = wdsc_dmanext;
    sc->sc_dmastop = wdsc_dmastop;
    sc->sc_dmacmd  = 0;

    sc->sc_adapter.scsipi_cmd = sbic_scsicmd;
    sc->sc_adapter.scsipi_minphys = sbic_minphys;

    sc->sc_link.scsipi_scsi.channel        = SCSI_CHANNEL_ONLY_ONE;
    sc->sc_link.adapter_softc  = sc;
    sc->sc_link.scsipi_scsi.adapter_target = 7;
    sc->sc_link.adapter        = &sc->sc_adapter;
    sc->sc_link.device         = &wdsc_scsidev;
    sc->sc_link.openings       = 2;
    sc->sc_link.scsipi_scsi.max_target     = 7;
    sc->sc_link.scsipi_scsi.max_lun = 7;
    sc->sc_link.type = BUS_SCSI;

    printf(": WD33C93 SCSI, target %d\n",
		sc->sc_link.scsipi_scsi.adapter_target);

    sc->sc_cregs = (volatile void *)sys_pcc;
    sc->sc_sbicp = (sbic_regmap_p) PCC_VADDR(pa->pa_offset);

    /*
     * Eveything is a valid dma address.
     */
    sc->sc_dmamask = 0;

    /*
     * The onboard WD33C93 of the '147 is usually clocked at 10MHz...
     * (We use 10 times this for accuracy in later calculations)
     */
    sc->sc_clkfreq = 100;

    /*
     * Initialise the hardware
     */
    sbicinit(sc);

    /*
     * Fix up the interrupts
     */
    sc->sc_ipl = pa->pa_ipl & PCC_IMASK;

    sys_pcc->scsi_int = sc->sc_ipl | PCC_ICLEAR;
    sys_pcc->dma_int  = sc->sc_ipl | PCC_ICLEAR;
    sys_pcc->dma_csr  = 0;

    pccintr_establish(PCCV_SCSID, wdsc_dmaintr,  sc->sc_ipl, sc);
    pccintr_establish(PCCV_SCSIP, wdsc_scsiintr, sc->sc_ipl, sc);

    sys_pcc->scsi_int = sc->sc_ipl | PCC_IENABLE | PCC_ICLEAR;

    /*
     * Attach all scsi units on us, watching for boot device
     * (see dk_establish).
     */
    tmp = bootpart;
    if (PCC_PADDR(pa->pa_offset) != bootaddr)
	bootpart = -1;		/* invalid flag to dk_establish */
    (void)config_found(dp, &sc->sc_link, scsiprint);
    bootpart = tmp;		/* restore old value */
}

/*
 * Enable DMA interrupts
 */
void
wdsc_enintr(dev)
    struct sbic_softc *dev;
{
    volatile struct pcc *pc = dev->sc_cregs;

    dev->sc_flags |= SBICF_INTR;

    pc->dma_int = dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR;
}

/*
 * Prime the hardware for a DMA transfer
 */
int
wdsc_dmago(dev, addr, count, flags)
    struct sbic_softc *dev;
    char *addr;
    int count, flags;
{
    volatile struct pcc *pc = dev->sc_cregs;

    /*
     * Set up the command word based on flags
     */
    if ( (flags & DMAGO_READ) == 0 )
        dev->sc_dmacmd = DMAC_CSR_ENABLE | DMAC_CSR_WRITE;
    else
        dev->sc_dmacmd = DMAC_CSR_ENABLE;

    dev->sc_flags |= SBICF_INTR;
    dev->sc_tcnt   = dev->sc_cur->dc_count << 1;

    /*
     * Prime the hardware.
     * Note, it's probably not necessary to do this here, since dmanext
     * is called just prior to the actual transfer.
     */
    pc->dma_csr   = 0;
    pc->dma_int   = dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR;
    pc->dma_daddr = (unsigned long)dev->sc_cur->dc_addr;
    pc->dma_bcnt  = (unsigned long)dev->sc_tcnt | (1 << 24);
    pc->dma_csr   = dev->sc_dmacmd;

    return(dev->sc_tcnt);
}

/*
 * Prime the hardware for the next DMA transfer
 */
int
wdsc_dmanext(dev)
    struct sbic_softc *dev;
{
    volatile struct pcc *pc = dev->sc_cregs;

    if ( dev->sc_cur > dev->sc_last ) {
        /*
         * Shouldn't happen !!
         */
        printf("wdsc_dmanext at end !!!\n");
        wdsc_dmastop(dev);
        return(0);
    }

    dev->sc_tcnt = dev->sc_cur->dc_count << 1;

    /*
     * Load the next DMA address
     */
    pc->dma_csr   = 0;
    pc->dma_int   = dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR;
    pc->dma_daddr = (unsigned long)dev->sc_cur->dc_addr;
    pc->dma_bcnt  = (unsigned long)dev->sc_tcnt | (1 << 24);
    pc->dma_csr   = dev->sc_dmacmd;

    return(dev->sc_tcnt);
}

/*
 * Stop DMA, and disable interrupts
 */
void
wdsc_dmastop(dev)
    struct sbic_softc *dev;
{
    volatile struct pcc *pc = dev->sc_cregs;
    int                 s;

    s = splbio();

    pc->dma_csr    = 0;
    pc->dma_int    = dev->sc_ipl | PCC_ICLEAR;

    splx(s);
}

/*
 * Come here following a DMA interrupt
 */
int
wdsc_dmaintr(arg)
    void *arg;
{
    struct sbic_softc *dev = arg;
    volatile struct pcc *pc = dev->sc_cregs;
    int                 found = 0;

    /*
     * Really a DMA interrupt?
     */
    if ( (pc->dma_int & 0x80) == 0 )
        return(0);

    /*
     * Was it a completion interrupt?
     * XXXSCW Note: Support for other DMA interrupts is required, eg. buserr
     */
    if ( pc->dma_csr & DMAC_CSR_DONE ) {
        ++found;

        pc->dma_int = dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR;
    }

    return(found);
}

/*
 * Come here for SCSI interrupts
 */
int
wdsc_scsiintr(arg)
    void *arg;
{
    struct sbic_softc *dev = arg;
    volatile struct pcc *pc = dev->sc_cregs;
    int                 found;

    /*
     * Really a SCSI interrupt?
     */
    if ( (pc->scsi_int & 0x80) == 0 )
        return(0);

    /*
     * Go handle it
     */
    found = sbicintr(dev);

    /*
     * Acknowledge and clear the interrupt
     */
    pc->scsi_int = dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR;

    return(found);
}
