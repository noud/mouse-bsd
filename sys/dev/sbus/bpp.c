/*	$NetBSD: bpp.c,v 1.3 1999/11/21 15:01:50 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/conf.h>
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/ic/lsi64854reg.h>
#include <dev/ic/lsi64854var.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sbus/bppreg.h>

#define BPPPRI (PZERO+1)

#define splbpp()	spltty()	/* XXX */

#if 0
struct bpp_param {
	int	bpp_dss;		/* data setup to strobe */
	int	bpp_dsw;		/* data strobe width */
	int	bpp_outputpins;		/* Select/Autofeed/Init pins */
	int	bpp_inputpins;		/* Error/Select/Paperout pins */
};
#endif

struct hwstate {
	u_int16_t	hw_hcr;		/* Hardware config register */
	u_int16_t	hw_ocr;		/* Operation config register */
	u_int8_t	hw_tcr;		/* Transfer Control register */
	u_int8_t	hw_or;		/* Output register */
	u_int16_t	hw_irq;		/* IRQ; polarity bits only */
};

struct bpp_softc {
	struct lsi64854_softc	sc_lsi64854;	/* base device */
	struct sbusdev	sc_sd;			/* sbus device */

	size_t		sc_bufsz;		/* temp buffer */
	caddr_t		sc_buf;

	int		sc_error;		/* bottom-half error */
	int		sc_flags;
#define BPP_OPEN	0x01		/* Device is open */
#define BPP_XCLUDE	0x02		/* Exclusive-open mode */
#define BPP_ASYNC	0x04		/* Asynchronous I/O mode */
#define BPP_LOCKED	0x08		/* DMA in progress */
#define BPP_WANT	0x10		/* Waiting for DMA */

	struct selinfo	sc_rsel;
	struct selinfo	sc_wsel;
	struct proc	*sc_asyncproc;	/* Process to notify if async */

	/* Hardware state */
	struct hwstate		sc_hwdefault;
	struct hwstate		sc_hwcurrent;
};

static void	bpp_disable_intr(struct bpp_softc *);
static int	bpp_tcr_wait	(struct bpp_softc *, unsigned int,
				    unsigned int, const char *);
static int	bppmatch	(struct device *, struct cfdata *, void *);
static void	bppattach	(struct device *, struct device *, void *);
static int	bppintr		(void *);
static void	bpp_setparams	(struct bpp_softc *, struct hwstate *);

struct cfattach bpp_ca = {
	sizeof(struct bpp_softc), bppmatch, bppattach
};

extern struct cfdriver bpp_cd;
#define BPPUNIT(dev)	(minor(dev) & 0x1f)
#define BPPFLAGS(dev)	(minor(dev) & ~0x1f)

#define BPPF_RAW 0x20

int
bppmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("SUNW,bpp", sa->sa_name) == 0);
}

static void
bpp_disable_intr(sc)
	struct bpp_softc *sc;
{
	u_int16_t irq;
	struct lsi64854_softc *lsc;

	irq = (sc->sc_hwdefault.hw_irq | BPP_ALLIRQ) & ~BPP_ALLEN;
	lsc = &sc->sc_lsi64854;
	bus_space_write_2(lsc->sc_bustag, lsc->sc_regs, L64854_REG_ICR, irq);
}

void
bppattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct bpp_softc *dsc = (void *)self;
	struct lsi64854_softc *sc = &dsc->sc_lsi64854;
	bus_space_handle_t h;
	struct hwstate *hw;
	int burst, sbusburst;
	int node;

	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;
	node = sa->sa_node;

	/* Map device registers */
	if (bus_space_map2(sa->sa_bustag,
			   sa->sa_slot,
			   sa->sa_offset,
			   sa->sa_size,
			   BUS_SPACE_MAP_LINEAR,
			   0, &sc->sc_regs) != 0) {
		printf("%s: cannot map registers\n", self->dv_xname);
		return;
	}

	/*
	 * Get transfer burst size from PROM and plug it into the
	 * controller registers. This is needed on the Sun4m; do
	 * others need it too?
	 */
	sbusburst = ((struct sbus_softc *)parent)->sc_burst;
	if (sbusburst == 0)
		sbusburst = SBUS_BURST_32 - 1; /* 1->16 */

	burst = getpropint(node, "burst-sizes", -1);
	if (burst == -1)
		/* take SBus burst sizes */
		burst = sbusburst;

	/* Clamp at parent's burst sizes */
	burst &= sbusburst;
	sc->sc_burst = (burst & SBUS_BURST_32) ? 32 :
		       (burst & SBUS_BURST_16) ? 16 : 0;

	/* Join the Sbus device family */
	dsc->sc_sd.sd_reset = (void *)0;
	sbus_establish(&dsc->sc_sd, self);

	/* Initialize the DMA channel */
	sc->sc_channel = L64854_CHANNEL_PP;
	lsi64854_attach(sc);

	/* Establish interrupt handler */
	if (sa->sa_nintr) {
		sc->sc_intrchain = bppintr;
		sc->sc_intrchainarg = dsc;
		(void)bus_intr_establish(sa->sa_bustag, sa->sa_pri, 0,
					 lsi64854_pp_intr, sc);
	}

	/* Allocate buffer XXX - should actually use dmamap_uio() */
	dsc->sc_bufsz = 1024;
	dsc->sc_buf = malloc(dsc->sc_bufsz, M_DEVBUF, M_NOWAIT);

	/* XXX read default state */
	h = sc->sc_regs;
	hw = &dsc->sc_hwdefault;

	hw->hw_hcr = bus_space_read_2(sc->sc_bustag, h, L64854_REG_HCR);
	hw->hw_ocr = bus_space_read_2(sc->sc_bustag, h, L64854_REG_OCR);
	hw->hw_tcr = bus_space_read_1(sc->sc_bustag, h, L64854_REG_TCR);
	hw->hw_or = bus_space_read_1(sc->sc_bustag, h, L64854_REG_OR);

	/* Turn off interrupts; we don't need or want them */
	bpp_disable_intr(dsc);
}

void
bpp_setparams(sc, hw)
	struct bpp_softc *sc;
	struct hwstate *hw;
{
	u_int16_t irq;
	bus_space_tag_t t = sc->sc_lsi64854.sc_bustag;
	bus_space_handle_t h = sc->sc_lsi64854.sc_regs;

	bus_space_write_2(t, h, L64854_REG_HCR, hw->hw_hcr);
	bus_space_write_2(t, h, L64854_REG_OCR, hw->hw_ocr);
	bus_space_write_1(t, h, L64854_REG_TCR, hw->hw_tcr);
	bus_space_write_1(t, h, L64854_REG_OR, hw->hw_or);

	/* Only change IRP settings in interrupt status register */
	irq = bus_space_read_2(t, h, L64854_REG_ICR);
	irq &= ~BPP_ALLIRP;
	irq |= (hw->hw_irq & BPP_ALLIRP);
	bus_space_write_2(t, h, L64854_REG_ICR, irq);
}

/* returns true on failure */
static int
bpp_tcr_wait(sc, set, clr, wchan)
	struct bpp_softc *sc;
	unsigned int set;
	unsigned int clr;
	const char *wchan;
{
	int spin;
	u_int8_t tcr;
	bus_space_tag_t t;
	bus_space_handle_t h;

	t = sc->sc_lsi64854.sc_bustag;
	h = sc->sc_lsi64854.sc_regs;
	for (spin = 100; spin > 0; spin--) {
		tcr = bus_space_read_1(t, h, L64854_REG_TCR);
		if (((tcr & set) == set) && ((tcr & clr) == 0))
			break;
	}
	while (((tcr & set) != set) || ((tcr & clr) != 0)) {
		if (tsleep((caddr_t)sc, BPPPRI|PCATCH, wchan, 1) == EINTR)
			return (1);
		tcr = bus_space_read_1(t, h, L64854_REG_TCR);
	}
	return(0);
}

int
bppopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	int unit = BPPUNIT(dev);
	unsigned short int dflags = BPPFLAGS(dev);
	struct bpp_softc *sc;
	struct lsi64854_softc *lsi;
	u_int8_t tcr;
	int s, err;

	if (unit >= bpp_cd.cd_ndevs)
		return (ENXIO);
	sc = bpp_cd.cd_devs[unit];

	if ((sc->sc_flags & (BPP_OPEN|BPP_XCLUDE)) == (BPP_OPEN|BPP_XCLUDE))
		return (EBUSY);

 if (dflags & BPPF_RAW) return(0);

	lsi = &sc->sc_lsi64854;

	/* Set default parameters */
	sc->sc_hwcurrent = sc->sc_hwdefault;
	s = splbpp();
	bpp_setparams(sc, &sc->sc_hwdefault);
	splx(s);

	bpp_disable_intr(sc);

	/* Assert INIT for 100ms, and assert SELECT */
	bus_space_write_1(lsi->sc_bustag, lsi->sc_regs, L64854_REG_OR,
	    BPP_OR_SLCTIN|BPP_OR_INIT);
	err = tsleep((caddr_t)sc, BPPPRI|PCATCH, "bppopen1", hz/10);
	if (err && (err != EWOULDBLOCK))
		return(err);

	/* Deassert INIT, wait until ready */
	bus_space_write_1(lsi->sc_bustag, lsi->sc_regs, L64854_REG_OR,
	    BPP_OR_SLCTIN);
	if (bpp_tcr_wait(sc, 0, BPP_TCR_BUSY, "bppopen2"))
		return (EINTR);

	/* Clear direction control bit (is this actually necessary?) */
	tcr = bus_space_read_1(lsi->sc_bustag, lsi->sc_regs, L64854_REG_TCR);
	tcr &= ~BPP_TCR_DIR;
	bus_space_write_1(lsi->sc_bustag, lsi->sc_regs, L64854_REG_TCR, tcr);

	return (0);
}

int
bppclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct bpp_softc *sc = bpp_cd.cd_devs[BPPUNIT(dev)];
	struct lsi64854_softc *lsi = &sc->sc_lsi64854;

	/* Deassert SELECT */
	bus_space_write_2(lsi->sc_bustag, lsi->sc_regs, L64854_REG_OR, 0);

	sc->sc_asyncproc = NULL;
	sc->sc_flags = 0;
	return (0);
}

int
bppread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return (ENXIO);
}

int
bppwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct bpp_softc *sc = bpp_cd.cd_devs[BPPUNIT(dev)];
	struct lsi64854_softc *lsi = &sc->sc_lsi64854;
	int error = 0;
	int s;

 if (BPPFLAGS(dev) & BPPF_RAW)
  { unsigned char c[2];
    unsigned char tcr;
    unsigned short int ocr;
    if (uio->uio_resid < 2) return(EINVAL);
    uiomove(&c[0],2,uio);
    c[1] &= 0x0f /* LPC_STROBE | LPC_AUTOLF | LPC_NINIT | LPC_SELECT */;
    tcr = bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR);
    tcr &= ~BPP_TCR_DIR;
    if (c[1] & 1/*LPC_STROBE*/) tcr |= BPP_TCR_DS; else tcr &= ~BPP_TCR_DS;
    bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR,tcr);
    ocr = bus_space_read_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR);
    ocr &= ~( BPP_OCR_SRST | BPP_OCR_ACK_OP | BPP_OCR_BUSY_OP | BPP_OCR_EN_DIAG |
	      BPP_OCR_ACK_DSEL | BPP_OCR_BUSY_DSEL | BPP_OCR_DS_DSEL );
    bus_space_write_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR,ocr);
    bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,c[0]);
    bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OR,(c[1]&6)|((c[1]>>3)&1));
    return(0);
  }
	/*
	 * Wait until free.
	 */
	s = splbpp();
	while ((sc->sc_flags & BPP_LOCKED) != 0) {
		if ((flags & IO_NDELAY) != 0) {
			splx(s);
			return (EWOULDBLOCK);
		}

		sc->sc_flags |= BPP_WANT;
		error = tsleep(sc->sc_buf, PZERO|PCATCH, "bppwrite", 0);
		if (error != 0) {
			splx(s);
			return (error);
		}
	}
	sc->sc_flags |= BPP_LOCKED;
	splx(s);

	/*
	 * Move data from user space into our private buffer
	 * and ship it out, a byte at a time (sigh).
	 */
	while (uio->uio_resid > 0) {
		caddr_t bp = sc->sc_buf;
		size_t len = min(sc->sc_bufsz, uio->uio_resid);
		int x;

		if ((error = uiomove(bp, len, uio)) != 0)
			break;
		x = 0;
		while (len > 0) {
			if (bpp_tcr_wait(sc, 0, BPP_TCR_BUSY|BPP_TCR_ACK,
			    "bppwr1"))
				goto out;
			bus_space_write_1(lsi->sc_bustag, lsi->sc_regs,
				L64854_REG_DR, ((unsigned char *)bp)[x]);
			bus_space_write_1(lsi->sc_bustag, lsi->sc_regs,
				L64854_REG_TCR, 0);
			if (bpp_tcr_wait(sc, BPP_TCR_BUSY, 0, "bppwr2"))
				goto out;
			bus_space_write_1(lsi->sc_bustag, lsi->sc_regs,
				L64854_REG_TCR, BPP_TCR_DS);
			x++;
			len--;
		}
	}

out:
	s = splbpp();
	sc->sc_flags &= ~BPP_LOCKED;
	if ((sc->sc_flags & BPP_WANT) != 0) {
		sc->sc_flags &= ~BPP_WANT;
		wakeup(sc->sc_buf);
	}
	splx(s);
	return (error);
}

/* move to header: */
#define BPPIOCSPARAM	_IOW('P', 0x1, struct hwstate)
#define BPPIOCGPARAM	_IOR('P', 0x2, struct hwstate)
struct rrblk {
  unsigned int len;
  unsigned char *buf;
  } ;
#define BPPIOCRAWREAD	_IOW('P', 0x3, struct rrblk)
#define BPPIOCPSREAD	_IOW('P', 0x4, char *)

static int bpp_rawread(dev_t dev, struct bpp_softc *sc, const struct rrblk *rbp)
{
 struct lsi64854_softc *lsi;
 int i;
 int n;
 unsigned char tcr;
 unsigned char ir;
 unsigned short int ocr;
 unsigned char buf[256];

 if (! (BPPFLAGS(dev) & BPPF_RAW)) return(ENOTTY);
 if (rbp->len < 1) return(0);
 if (rbp->len > 256) return(EMSGSIZE);
 n = rbp->len;
 i = copyin(rbp->buf,&buf[0],n);
 if (i) return(i);
 lsi = &sc->sc_lsi64854;
 tcr = bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR);
 tcr &= ~BPP_TCR_DIR;
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR,tcr);
 ocr = bus_space_read_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR);
 ocr &= ~( BPP_OCR_SRST | BPP_OCR_ACK_OP | BPP_OCR_BUSY_OP | BPP_OCR_EN_DIAG |
	   BPP_OCR_ACK_DSEL | BPP_OCR_BUSY_DSEL | BPP_OCR_DS_DSEL );
 bus_space_write_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR,ocr|BPP_OCR_SRST);
 bus_space_write_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR,ocr);
 for (i=0;i<n;i++)
  { bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,buf[i]);
    tcr = bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR);
    ir = bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_IR);
    buf[i] = (((tcr & 0x06) << 5) | ((ir & 0x07) << 3)) ^ 0xf8;
  }
 return(copyout(&buf[0],rbp->buf,n));
}

static void sendrecv(struct lsi64854_softc *lsi, unsigned char s, unsigned char *rp)
{
 unsigned char r;
 int i;

 r = 0;
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xf3);
 for (i=0;i<8;i++)
  { DELAY(10);
    bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xf8|((s&1)<<1));
    DELAY(10);
    r >>= 1;
    if (! (bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_IR) & BPP_IR_PE)) r |= 0x80;
    bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xf9|((s&1)<<1));
    s >>= 1;
  }
 DELAY(10);
 if (rp) *rp = r;
}

static int getack(struct lsi64854_softc *lsi)
{
 int i;

 for (i=100;i>0;i--)
  { if (bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR) & BPP_TCR_ACK) break;
    DELAY(1);
  }
 if (! i)
  { printf("bpp getack: no response\n");
    return(1);
  }
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xf3);
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xfb);
 for (i=100;i>0;i--)
  { if (! (bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR) & BPP_TCR_ACK)) break;
    DELAY(1);
  }
 if (! i) printf("bpp getack: ACK stuck high\n");
 return(0);
}

static int bpp_psread(dev_t dev, struct bpp_softc *sc, char * const *rbpp)
{
 struct lsi64854_softc *lsi;
 int i;
 int n;
 unsigned char tcr;
 unsigned short int ocr;
 unsigned char buf[256];

 if (! (BPPFLAGS(dev) & BPPF_RAW)) return(ENOTTY);
 lsi = &sc->sc_lsi64854;
 tcr = bus_space_read_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR);
 tcr &= ~BPP_TCR_DIR;
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_TCR,tcr);
 ocr = bus_space_read_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR);
 ocr &= ~( BPP_OCR_SRST | BPP_OCR_ACK_OP | BPP_OCR_BUSY_OP | BPP_OCR_EN_DIAG |
	   BPP_OCR_ACK_DSEL | BPP_OCR_BUSY_DSEL | BPP_OCR_DS_DSEL );
 bus_space_write_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR,ocr|BPP_OCR_SRST);
 bus_space_write_2(lsi->sc_bustag,lsi->sc_regs,L64854_REG_OCR,ocr);
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xf3);
 sendrecv(lsi,0x01,0);
 if (getack(lsi)) return(EHOSTDOWN);
 sendrecv(lsi,0x42,&buf[1]);
 switch (buf[1])
  { case 0x41: n = 3; break;
    case 0x23: n = 7; break;
    case 0x73: n = 7; break;
    case 0x53: n = 7; break;
    default: n = 0; break;
  }
 buf[0] = n + 1;
 i = 2;
 for (;n>0;n--)
  { if (getack(lsi)) return(EHOSTDOWN);
    sendrecv(lsi,0xff,&buf[i++]);
  }
 bus_space_write_1(lsi->sc_bustag,lsi->sc_regs,L64854_REG_DR,0xff);
 return(copyout(&buf[0],*rbpp,buf[0]+1));
}

int
bppioctl(dev, cmd, data, flag, p)
	dev_t	dev;
	u_long	cmd;
	caddr_t	data;
	int	flag;
	struct	proc *p;
{
	struct bpp_softc *sc = bpp_cd.cd_devs[BPPUNIT(dev)];
	struct hwstate *hw, *chw;
	int error = 0;
	int s;

	switch(cmd) {
	case BPPIOCRAWREAD:
		return(bpp_rawread(dev,sc,(const void *)data));
		break;
	case BPPIOCPSREAD:
		return(bpp_psread(dev,sc,(const void *)data));
		break;
	case BPPIOCSPARAM:
		chw = &sc->sc_hwcurrent;
		hw = (struct hwstate *)data;

		/*
		 * Extract and store user-settable bits.
		 */
#define _bpp_set(reg,mask) do {		\
	chw->reg &= ~(mask);		\
	chw->reg |= (hw->reg & (mask));	\
} while (0)
		_bpp_set(hw_hcr, BPP_HCR_DSS_MASK|BPP_HCR_DSW_MASK);
		_bpp_set(hw_ocr, BPP_OCR_USER);
		_bpp_set(hw_tcr, BPP_TCR_USER);
		_bpp_set(hw_or,  BPP_OR_USER);
		_bpp_set(hw_irq, BPP_IRQ_USER);
#undef _bpp_set

		/* Apply settings */
		s = splbpp();
		bpp_setparams(sc, chw);
		splx(s);
		break;
	case BPPIOCGPARAM:
		*((struct hwstate *)data) = sc->sc_hwcurrent;
		break;
	case TIOCEXCL:
		s = splbpp();
		sc->sc_flags |= BPP_XCLUDE;
		splx(s);
		break;
	case TIOCNXCL:
		s = splbpp();
		sc->sc_flags &= ~BPP_XCLUDE;
		splx(s);
		break;
	case FIOASYNC:
		s = splbpp();
		if (*(int *)data) {
			if (sc->sc_asyncproc != NULL)
				error = EBUSY;
			else
				sc->sc_asyncproc = p;
		} else
			sc->sc_asyncproc = NULL;
		splx(s);
		break;
	default:
		break;
	}

	return (error);
}

int
bpppoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct bpp_softc *sc = bpp_cd.cd_devs[BPPUNIT(dev)];
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		/* read is not yet implemented */
	}

	if (events & (POLLOUT | POLLWRNORM)) {
		if ((sc->sc_flags & BPP_LOCKED) == 0)
			revents |= (POLLOUT | POLLWRNORM);
	}

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(p, &sc->sc_rsel);
		if (events & (POLLOUT | POLLWRNORM))
			selrecord(p, &sc->sc_wsel);
	}

	return (revents);
}

int
bppintr(arg)
	void *arg;
{
	struct bpp_softc *sc = arg;
	struct lsi64854_softc *lsi = &sc->sc_lsi64854;
	u_int16_t irq;

	irq = bus_space_read_2(lsi->sc_bustag, lsi->sc_regs, L64854_REG_ICR);
	/* Ack all interrupts */
	bus_space_write_2(lsi->sc_bustag, lsi->sc_regs, L64854_REG_ICR,
			  irq | BPP_ALLIRQ);

	/* Did our device interrupt? */
	if ((irq & BPP_ALLIRQ) == 0)
		return (0);

	/* We shouldn't be *getting* any interrupts! */
	printf("bpp: unexpected interrupt\n");
	bpp_disable_intr(sc);

	return(1);
}
