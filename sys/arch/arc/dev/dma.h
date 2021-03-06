/*	$NetBSD: dma.h,v 1.5 2000/01/23 21:01:53 soda Exp $	*/
/*	$OpenBSD: dma.h,v 1.3 1997/04/19 17:19:51 pefo Exp $	*/

/*
 * Copyright (c) 1996 Per Fogelstrom
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
 *      This product includes software developed by Per Fogelstrom.
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

/*
 *	The R4030 system has four dma channels capable of scatter/gather
 *	and full memory addressing. The maximum transfer length is 1Mb.
 *	Dma snopes the L2 cache so no precaution is required. However
 *	if L1 cache is cached 'write back' the processor is responible
 *	for flushing/invalidating it.
 *
 *	The dma mapper has up to 4096 page descriptors.
 */

#define	PICA_TL_BASE	0xa0008000	/* Base of tl register area */
#define	PICA_TL_SIZE	0x00008000	/* Size of tl register area */

/*
 *  Hardware dma registers.
 */
typedef volatile struct {
	int		dma_mode;
	int		pad1;
	int		dma_enab;
	int		pad2;
	int		dma_count;
	int		pad3;
	vm_offset_t	dma_addr;
	int		pad4;
} DmaReg, *pDmaReg;

#define	R4030_DMA_MODE_40NS	0x00	/* Device dma timing */
#define	R4030_DMA_MODE_80NS	0x01	/* Device dma timing */
#define	R4030_DMA_MODE_120NS	0x02	/* Device dma timing */
#define	R4030_DMA_MODE_160NS	0x03	/* Device dma timing */
#define	R4030_DMA_MODE_200NS	0x04	/* Device dma timing */
#define	R4030_DMA_MODE_240NS	0x05	/* Device dma timing */
#define	R4030_DMA_MODE_280NS	0x06	/* Device dma timing */
#define	R4030_DMA_MODE_320NS	0x07	/* Device dma timing */
#define	R4030_DMA_MODE_8		0x08	/* Device 8 bit  */
#define	R4030_DMA_MODE_16	0x10	/* Device 16 bit */
#define	R4030_DMA_MODE_32	0x18	/* Device 32 bit */
#define	R4030_DMA_MODE_INT	0x20	/* Interrupt when done */
#define	R4030_DMA_MODE_BURST	0x40	/* Burst mode (Rev 2 only) */
#define R4030_DMA_MODE_FAST	0x80	/* Fast dma cycle (Rev 2 only) */
#define R4030_DMA_MODE		0xff	/* Mode register bits */
#define DMA_DIR_WRITE		0x100	/* Software direction status */
#define DMA_DIR_READ		0x000	/* Software direction status */

#define	R4030_DMA_ENAB_RUN	0x01	/* Enable dma */
#define	R4030_DMA_ENAB_READ	0x00	/* Read from device */
#define	R4030_DMA_ENAB_WRITE	0x02	/* Write to device */
#define	R4030_DMA_ENAB_TC_IE	0x100	/* Terminal count int enable */
#define	R4030_DMA_ENAB_ME_IE	0x200	/* Memory error int enable */
#define	R4030_DMA_ENAB_TL_IE	0x400	/* Translation limit int enable */

#define	R4030_DMA_COUNT_MASK	0x00fffff /* Byte count mask */
#define	R4030_DMA_PAGE_NUM	0xffff000 /* Address page number */
#define	R4030_DMA_PAGE_OFFS	0x0000fff /* Address page offset */
#define	R4030_DMA_PAGE_SIZE	0x0001000 /* Address page size */


/*
 *  Dma TLB entry
 */

typedef union dma_pte {
	struct {
	    vm_offset_t	lo_addr;	/* Low part of translation addr */
	    vm_offset_t	hi_addr;	/* High part of translation addr */
	} entry;
	struct bbb {
	    union dma_pte *next;	/* Next free translation entry */
	    int		size;		/* Number of consecutive free entrys */
	} queue;
} dma_pte_t;

/*
 *  Structure used to control dma.
 */

typedef struct dma_softc {
	struct device	sc_dev;		/* use as a device */
	struct esp_softc *sc_esp;
	vm_offset_t	dma_va;		/* Viritual address for transfer */
	int		req_va;		/* Original request va */
	vm_offset_t	next_va;	/* Value to program into dma regs */
	int		next_size;	/* Value to program into dma regs */
	int		mode;		/* Mode register value and direction */
	dma_pte_t	*pte_base;	/* Pointer to dma tlb array */
	int		pte_size;	/* Size of pte allocated pte array */
	pDmaReg		dma_reg;	/* Pointer to dma registers */
	int		sc_active;	/* Active flag */
	char		**sc_dmaaddr;	/* Pointer to dma address in dev */
	int		*sc_dmalen;	/* Pointer to len counter in dev */
	void (*reset)(struct dma_softc *);	/* Reset routine pointer */
	void (*enintr)(struct dma_softc *);	/* Int enab routine pointer */
	void (*map)(struct dma_softc *, char *, size_t, int);
						/* Map a dma viritual area */
	void (*start)(struct dma_softc *, caddr_t, size_t, int);
						/* Start routine pointer */
	int (*isintr)(struct dma_softc *);	/* Int check routine pointer */
	int (*intr)(struct dma_softc *);	/* Interrupt routine pointer */
	void (*end)(struct dma_softc *);	/* Interrupt routine pointer */
} dma_softc_t;

#define	DMA_TO_DEV	0
#define	DMA_FROM_DEV	1

#define	dma_page_offs(x)	((int)(x) & R4030_DMA_PAGE_OFFS)
#define dma_page_round(x)	(((int)(x) + R4030_DMA_PAGE_OFFS) & R4030_DMA_PAGE_NUM)

#define	DMA_RESET(r)		((r->reset)(r))
#define	DMA_START(a, b, c, d)	((a->start)(a, b, c, d))
#define	DMA_MAP(a, b, c, d)	((a->map)(a, b, c, d))
#define	DMA_INTR(r)		((r->intr)(r))
#define	DMA_DRAIN(r)
#define	DMA_END(r)		((r->end)(r))

void picaDmaInit __P((void));
void picaDmaTLBAlloc __P((dma_softc_t *));
void picaDmaTLBFree __P((dma_softc_t *));
void picaDmaTLBMap __P((dma_softc_t *));
void picaDmaMap __P((struct dma_softc *, char *, size_t, int));
void picaDmaStart __P((struct dma_softc *, char *, size_t, int));
void picaDmaFlush __P((struct dma_softc *, char *, size_t, int));
void asc_dma_init __P((struct dma_softc *));
void fdc_dma_init __P((struct dma_softc *));
void sn_dma_init __P((struct dma_softc *, int));
