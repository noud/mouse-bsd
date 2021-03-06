/*	$NetBSD: eap.c,v 1.32 1999/11/02 17:48:01 augustss Exp $	*/
/*      $OpenBSD: eap.c,v 1.6 1999/10/05 19:24:42 csapuntz Exp $ */

/*
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson <augustss@netbsd.org> and Charles M. Hannum.
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
 * Debugging:   Andreas Gustafsson <gson@araneus.fi>
 * Testing:     Chuck Cranor       <chuck@maria.wustl.edu>
 *              Phil Nelson        <phil@cs.wwu.edu>
 *
 * ES1371/AC97:	Ezra Story         <ezy@panix.com>
 */

/*
 * Ensoniq ES1370 + AK4531 and ES1371/ES1373 + AC97
 *
 * Documentation links:
 *
 * http://www.ensoniq.com/multimedia/semi_html/html/es1370.zip
 * ftp://ftp.alsa-project.org/pub/manuals/asahi_kasei/4531.pdf
 * http://www.ensoniq.com/multimedia/semi_html/html/es1371.zip
 * ftp://download.intel.com/pc-supp/platform/ac97/ac97r21.pdf
 */


#include "midi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>
#include <dev/ic/ac97.h>

#include <machine/bus.h>

#define	PCI_CBIO		0x10

#define EAP_ICSC		0x00    /* interrupt / chip select control */
#define  EAP_SERR_DISABLE	0x00000001
#define  EAP_CDC_EN		0x00000002
#define  EAP_JYSTK_EN		0x00000004
#define  EAP_UART_EN		0x00000008
#define  EAP_ADC_EN		0x00000010
#define  EAP_DAC2_EN		0x00000020
#define  EAP_DAC1_EN		0x00000040
#define  EAP_BREQ		0x00000080
#define  EAP_XTCL0		0x00000100
#define  EAP_M_CB		0x00000200
#define  EAP_CCB_INTRM		0x00000400
#define  EAP_DAC_SYNC		0x00000800
#define  EAP_WTSRSEL		0x00003000
#define   EAP_WTSRSEL_5		0x00000000
#define   EAP_WTSRSEL_11	0x00001000
#define   EAP_WTSRSEL_22	0x00002000
#define   EAP_WTSRSEL_44	0x00003000
#define  EAP_M_SBB		0x00004000
#define  EAP_MSFMTSEL		0x00008000
#define  EAP_SET_PCLKDIV(n)	(((n)&0x1fff)<<16)
#define  EAP_GET_PCLKDIV(n)	(((n)>>16)&0x1fff)
#define  EAP_PCLKBITS		0x1fff0000
#define  EAP_XTCL1		0x40000000
#define  EAP_ADC_STOP		0x80000000
#define  E1371_SYNC_RES		(1<<14)

#define EAP_ICSS		0x04	/* interrupt / chip select status */
#define  EAP_I_ADC		0x00000001
#define  EAP_I_DAC2		0x00000002
#define  EAP_I_DAC1		0x00000004
#define  EAP_I_UART		0x00000008
#define  EAP_I_MCCB		0x00000010
#define  EAP_VC			0x00000060
#define  EAP_CWRIP		0x00000100
#define  EAP_CBUSY		0x00000200
#define  EAP_CSTAT		0x00000400
#define  EAP_INTR		0x80000000

#define EAP_UART_DATA		0x08
#define EAP_UART_STATUS		0x09
#define  EAP_US_RXRDY		0x01
#define  EAP_US_TXRDY		0x02
#define  EAP_US_TXINT		0x04
#define  EAP_US_RXINT		0x80
#define EAP_UART_CONTROL	0x09
#define  EAP_UC_CNTRL		0x03
#define  EAP_UC_TXINTEN		0x20
#define  EAP_UC_RXINTEN		0x80
#define EAP_MEMPAGE		0x0c
#define EAP_CODEC		0x10
#define  EAP_SET_CODEC(a,d)	(((a)<<8) | (d))

/* ES1371 Registers */
#define E1371_CODEC		0x14
#define  E1371_CODEC_WIP	(1<<30)
#define  E1371_CODEC_VALID      (1<<31)
#define  E1371_CODEC_READ       (1<<23)
#define  E1371_SET_CODEC(a,d)	(((a)<<16) | (d))
#define E1371_SRC		0x10
#define  E1371_SRC_RAMWE	(1<<24)
#define  E1371_SRC_RBUSY	(1<<23)
#define  E1371_SRC_DISABLE	(1<<22)
#define  E1371_SRC_DISP1	(1<<21)
#define  E1371_SRC_DISP2        (1<<20)
#define  E1371_SRC_DISREC       (1<<19)
#define  E1371_SRC_ADDR(a)	((a)<<25)
#define  E1371_SRC_DATA(d)	(d)
#define  E1371_SRC_DATAMASK	0xffff
#define E1371_LEGACY		0x18

/* ES1371 Sample rate converter registers */
#define ESRC_ADC		0x78
#define ESRC_DAC1		0x74
#define ESRC_DAC2		0x70
#define ESRC_ADC_VOLL		0x6c
#define ESRC_ADC_VOLR		0x6d
#define ESRC_DAC1_VOLL		0x7c
#define ESRC_DAC1_VOLR		0x7d
#define ESRC_DAC2_VOLL		0x7e
#define ESRC_DAC2_VOLR		0x7f
#define  ESRC_TRUNC_N		0x00
#define  ESRC_IREGS		0x01
#define  ESRC_ACF		0x02
#define  ESRC_VFF		0x03
#define ESRC_SET_TRUNC(n)	((n)<<9)
#define ESRC_SET_N(n)		((n)<<4)
#define ESRC_SMF		0x8000
#define ESRC_SET_VFI(n)		((n)<<10)
#define ESRC_SET_ACI(n)		(n)
#define ESRC_SET_ADC_VOL(n)	((n)<<8)
#define ESRC_SET_DAC_VOLI(n)	((n)<<12)
#define ESRC_SET_DAC_VOLF(n)	(n)
#define  SRC_MAGIC ((1<15)|(1<<13)|(1<<11)|(1<<9))


#define EAP_SIC			0x20
#define  EAP_P1_S_MB		0x00000001
#define  EAP_P1_S_EB		0x00000002
#define  EAP_P2_S_MB		0x00000004
#define  EAP_P2_S_EB		0x00000008
#define  EAP_R1_S_MB		0x00000010
#define  EAP_R1_S_EB		0x00000020
#define  EAP_P2_DAC_SEN		0x00000040
#define  EAP_P1_SCT_RLD		0x00000080
#define  EAP_P1_INTR_EN		0x00000100
#define  EAP_P2_INTR_EN		0x00000200
#define  EAP_R1_INTR_EN		0x00000400
#define  EAP_P1_PAUSE		0x00000800
#define  EAP_P2_PAUSE		0x00001000
#define  EAP_P1_LOOP_SEL	0x00002000
#define  EAP_P2_LOOP_SEL	0x00004000
#define  EAP_R1_LOOP_SEL	0x00008000
#define  EAP_SET_P2_ST_INC(i)	((i) << 16)
#define  EAP_SET_P2_END_INC(i)	((i) << 19)
#define  EAP_INC_BITS		0x003f0000

#define EAP_DAC1_CSR		0x24
#define EAP_DAC2_CSR		0x28
#define EAP_ADC_CSR		0x2c
#define  EAP_GET_CURRSAMP(r)	((r) >> 16)

#define EAP_DAC_PAGE		0xc
#define EAP_ADC_PAGE		0xd
#define EAP_UART_PAGE1		0xe
#define EAP_UART_PAGE2		0xf

#define EAP_DAC1_ADDR		0x30
#define EAP_DAC1_SIZE		0x34
#define EAP_DAC2_ADDR		0x38
#define EAP_DAC2_SIZE		0x3c
#define EAP_ADC_ADDR		0x30
#define EAP_ADC_SIZE		0x34
#define  EAP_SET_SIZE(c,s)	(((c)<<16) | (s))

#define EAP_READ_TIMEOUT	5000000
#define EAP_WRITE_TIMEOUT	5000000


#define EAP_XTAL_FREQ 1411200 /* 22.5792 / 16 MHz */

/* AK4531 registers */
#define AK_MASTER_L		0x00
#define AK_MASTER_R		0x01
#define AK_VOICE_L		0x02
#define AK_VOICE_R		0x03
#define AK_FM_L			0x04
#define AK_FM_R			0x05
#define AK_CD_L			0x06
#define AK_CD_R			0x07
#define AK_LINE_L		0x08
#define AK_LINE_R		0x09
#define AK_AUX_L		0x0a
#define AK_AUX_R		0x0b
#define AK_MONO1		0x0c
#define AK_MONO2		0x0d
#define AK_MIC			0x0e
#define AK_MONO			0x0f
#define AK_OUT_MIXER1		0x10
#define  AK_M_FM_L		0x40
#define  AK_M_FM_R		0x20
#define  AK_M_LINE_L		0x10
#define  AK_M_LINE_R		0x08
#define  AK_M_CD_L		0x04
#define  AK_M_CD_R		0x02
#define  AK_M_MIC		0x01
#define AK_OUT_MIXER2		0x11
#define  AK_M_AUX_L		0x20
#define  AK_M_AUX_R		0x10
#define  AK_M_VOICE_L		0x08
#define  AK_M_VOICE_R		0x04
#define  AK_M_MONO2		0x02
#define  AK_M_MONO1		0x01
#define AK_IN_MIXER1_L		0x12
#define AK_IN_MIXER1_R		0x13
#define AK_IN_MIXER2_L		0x14
#define AK_IN_MIXER2_R		0x15
#define  AK_M_TMIC		0x80
#define  AK_M_TMONO1		0x40
#define  AK_M_TMONO2		0x20
#define  AK_M2_AUX_L		0x10
#define  AK_M2_AUX_R		0x08
#define  AK_M_VOICE		0x04
#define  AK_M2_MONO2		0x02
#define  AK_M2_MONO1		0x01
#define AK_RESET		0x16
#define  AK_PD			0x02
#define  AK_NRST		0x01
#define AK_CS			0x17
#define AK_ADSEL		0x18
#define AK_MGAIN		0x19
#define AK_NPORTS               0x20

#define MAX_NPORTS              AK_NPORTS

/* Not sensical for AC97? */
#define VOL_TO_ATT5(v) (0x1f - ((v) >> 3))
#define VOL_TO_GAIN5(v) VOL_TO_ATT5(v)
#define ATT5_TO_VOL(v) ((0x1f - (v)) << 3)
#define GAIN5_TO_VOL(v) ATT5_TO_VOL(v)
#define VOL_0DB 200

/* Futzable parms */
#define EAP_MASTER_VOL		0
#define EAP_VOICE_VOL		1
#define EAP_FM_VOL		2
#define EAP_VIDEO_VOL		2 /* ES1371 */
#define EAP_CD_VOL		3
#define EAP_LINE_VOL		4
#define EAP_AUX_VOL		5
#define EAP_MIC_VOL		6
#define	EAP_RECORD_SOURCE 	7
#define EAP_OUTPUT_SELECT	8
#define	EAP_MIC_PREAMP		9
#define EAP_OUTPUT_CLASS	10
#define EAP_RECORD_CLASS	11
#define EAP_INPUT_CLASS		12

#define MIDI_BUSY_WAIT		100
#define MIDI_BUSY_DELAY		100	/* Delay when UART is busy */

/* Debug */
#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (eapdebug) printf x
#define DPRINTFN(n,x)	if (eapdebug>(n)) printf x
int	eapdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

int	eap_match __P((struct device *, struct cfdata *, void *));
void	eap_attach __P((struct device *, struct device *, void *));
int	eap_intr __P((void *));

struct eap_dma {
	bus_dmamap_t map;
	caddr_t addr;
	bus_dma_segment_t segs[1];
	int nsegs;
	size_t size;
	struct eap_dma *next;
};
#define DMAADDR(p) ((p)->map->dm_segs[0].ds_addr)
#define KERNADDR(p) ((void *)((p)->addr))

struct eap_softc {
	struct device sc_dev;		/* base device */
	void *sc_ih;			/* interrupt vectoring */
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_dma_tag_t sc_dmatag;	/* DMA tag */

	struct eap_dma *sc_dmas;

	void	(*sc_pintr)(void *);	/* dma completion intr handler */
	void	*sc_parg;		/* arg for sc_intr() */
#ifdef DIAGNOSTIC
	char	sc_prun;
#endif

	void	(*sc_rintr)(void *);	/* dma completion intr handler */
	void	*sc_rarg;		/* arg for sc_intr() */
#ifdef DIAGNOSTIC
	char	sc_rrun;
#endif

#if NMIDI > 0
	void	(*sc_iintr)(void *, int); /* midi input ready handler */
	void	(*sc_ointr)(void *);	/* midi output ready handler */
	void	*sc_arg;
#endif

	u_short	sc_port[MAX_NPORTS];	/* mirror of the hardware setting */
	u_int	sc_record_source;	/* recording source mask */
	u_int	sc_output_source;	/* output source mask */
	u_int	sc_mic_preamp;
        char    sc_1371;                /* Using ES1371/AC97 codec */

	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;
};

int	eap_allocmem __P((struct eap_softc *, size_t, size_t, struct eap_dma *));
int	eap_freemem __P((struct eap_softc *, struct eap_dma *));

#define EWRITE1(sc, r, x) bus_space_write_1((sc)->iot, (sc)->ioh, (r), (x))
#define EWRITE2(sc, r, x) bus_space_write_2((sc)->iot, (sc)->ioh, (r), (x))
#define EWRITE4(sc, r, x) bus_space_write_4((sc)->iot, (sc)->ioh, (r), (x))
#define EREAD1(sc, r) bus_space_read_1((sc)->iot, (sc)->ioh, (r))
#define EREAD2(sc, r) bus_space_read_2((sc)->iot, (sc)->ioh, (r))
#define EREAD4(sc, r) bus_space_read_4((sc)->iot, (sc)->ioh, (r))

struct cfattach eap_ca = {
	sizeof(struct eap_softc), eap_match, eap_attach
};

int	eap_open __P((void *, int));
void	eap_close __P((void *));
int	eap_query_encoding __P((void *, struct audio_encoding *));
int	eap_set_params __P((void *, int, int, struct audio_params *, struct audio_params *));
int	eap_round_blocksize __P((void *, int));
int	eap_trigger_output __P((void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *));
int	eap_trigger_input __P((void *, void *, void *, int, void (*)(void *),
	    void *, struct audio_params *));
int	eap_halt_output __P((void *));
int	eap_halt_input __P((void *));
void    eap_write_codec __P((struct eap_softc *, int, int));
int	eap_getdev __P((void *, struct audio_device *));
int	eap_mixer_set_port __P((void *, mixer_ctrl_t *));
int	eap_mixer_get_port __P((void *, mixer_ctrl_t *));
int	eap1371_mixer_set_port __P((void *, mixer_ctrl_t *));
int	eap1371_mixer_get_port __P((void *, mixer_ctrl_t *));
int	eap_query_devinfo __P((void *, mixer_devinfo_t *));
void   *eap_malloc __P((void *, int, size_t, int, int));
void	eap_free __P((void *, void *, int));
size_t	eap_round_buffersize __P((void *, int, size_t));
int	eap_mappage __P((void *, void *, int, int));
int	eap_get_props __P((void *));
void	eap_set_mixer __P((struct eap_softc *sc, int a, int d));
void	eap1371_src_wait __P((struct eap_softc *sc));
void 	eap1371_set_adc_rate __P((struct eap_softc *sc, int rate));
void 	eap1371_set_dac_rate __P((struct eap_softc *sc, int rate, int which));
int	eap1371_src_read __P((struct eap_softc *sc, int a));
void	eap1371_src_write __P((struct eap_softc *sc, int a, int d));
int	eap1371_query_devinfo __P((void *addr, mixer_devinfo_t *dip));

int     eap1371_attach_codec __P((void *sc, struct ac97_codec_if *));
int	eap1371_read_codec __P((void *sc, u_int8_t a, u_int16_t *d));
int	eap1371_write_codec __P((void *sc, u_int8_t a, u_int16_t d));
void    eap1371_reset_codec __P((void *sc));
int     eap1371_get_portnum_by_name __P((struct eap_softc *, char *, char *,
					 char *));
#if NMIDI > 0
void	eap_midi_close __P((void *));
void	eap_midi_getinfo __P((void *, struct midi_info *));
int	eap_midi_open __P((void *, int, void (*)(void *, int),
			   void (*)(void *), void *));
int	eap_midi_output __P((void *, int));
#endif

struct audio_hw_if eap1370_hw_if = {
	eap_open,
	eap_close,
	NULL,
	eap_query_encoding,
	eap_set_params,
	eap_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	eap_halt_output,
	eap_halt_input,
	NULL,
	eap_getdev,
	NULL,
	eap_mixer_set_port,
	eap_mixer_get_port,
	eap_query_devinfo,
	eap_malloc,
	eap_free,
	eap_round_buffersize,
	eap_mappage,
	eap_get_props,
	eap_trigger_output,
	eap_trigger_input,
};

struct audio_hw_if eap1371_hw_if = {
	eap_open,
	eap_close,
	NULL,
	eap_query_encoding,
	eap_set_params,
	eap_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	eap_halt_output,
	eap_halt_input,
	NULL,
	eap_getdev,
	NULL,
	eap1371_mixer_set_port,
	eap1371_mixer_get_port,
	eap1371_query_devinfo,
	eap_malloc,
	eap_free,
	eap_round_buffersize,
	eap_mappage,
	eap_get_props,
	eap_trigger_output,
	eap_trigger_input,
};

#if NMIDI > 0
struct midi_hw_if eap_midi_hw_if = {
	eap_midi_open,
	eap_midi_close,
	eap_midi_output,
	eap_midi_getinfo,
	0,				/* ioctl */
};
#endif

struct audio_device eap_device = {
	"Ensoniq AudioPCI",
	"",
	"eap"
};

int
eap_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *) aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_ENSONIQ)
		return (0);
	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ENSONIQ_AUDIOPCI ||
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ENSONIQ_AUDIOPCI97) {
		return (1);
        }

	return (0);
}

void
eap_write_codec(sc, a, d)
	struct eap_softc *sc;
	int a, d;
{
	int icss, to;

	to = EAP_WRITE_TIMEOUT;
	do {
		icss = EREAD4(sc, EAP_ICSS);
		DPRINTFN(5,("eap: codec %d prog: icss=0x%08x\n", a, icss));
                if (!to--) {
                        printf("eap: timeout writing to codec\n");
                        return;
                }
	} while(icss & EAP_CWRIP);  /* XXX could use CSTAT here */
        EWRITE4(sc, EAP_CODEC, EAP_SET_CODEC(a, d));
}

int
eap1371_read_codec(sc_, a, d)
        void *sc_;
	u_int8_t a;
	u_int16_t *d;
{
	struct eap_softc *sc = sc_;
        int to;
        int cdc;

        to = EAP_WRITE_TIMEOUT;
        do {
                cdc = EREAD4(sc, E1371_CODEC);
                if (!to--) {
                        printf("eap: timeout writing to codec\n");
                        return 1;
                }
        } while (cdc & E1371_CODEC_WIP);

        /* just do it */
	eap1371_src_wait(sc);
        EWRITE4(sc, E1371_CODEC, E1371_SET_CODEC(a, 0) | E1371_CODEC_READ);

	for (to = 0; to < EAP_WRITE_TIMEOUT; to++) {
		if ((cdc = EREAD4(sc, E1371_CODEC)) & E1371_CODEC_VALID)
			break;
	}

	if (to == EAP_WRITE_TIMEOUT) {
		DPRINTF(("eap1371: read codec timeout\n"));
	}

	*d = cdc & 0xffff;

        DPRINTFN(10, ("eap1371: reading codec (%x) = %x\n", a, *d));

	return (0);
}

int
eap1371_write_codec(sc_, a, d)
        void *sc_;
	u_int8_t a;
	u_int16_t d;
{
	struct eap_softc *sc = sc_;
        int to;
        int cdc;

        to = EAP_WRITE_TIMEOUT;
        do {
                cdc = EREAD4(sc, E1371_CODEC);
                if (!to--) {
                        printf("eap: timeout writing to codec\n");
                        return (1);
                }
        } while (cdc & E1371_CODEC_WIP);

        /* just do it */
	eap1371_src_wait(sc);
        EWRITE4(sc, E1371_CODEC, E1371_SET_CODEC(a, d));
        DPRINTFN(10, ("eap1371: writing codec %x --> %x\n", d, a));

        return (0);
}

void
eap1371_src_wait(sc)
	struct eap_softc *sc;
{
        int to;
        int src;

        to = EAP_READ_TIMEOUT;
        do {
                src = EREAD4(sc, E1371_SRC);
                if (!to--) {
                        printf("eap: timeout waiting for sample rate"
                                "converter\n");
                        return;
                }
        } while (src & E1371_SRC_RBUSY);
}

int
eap1371_src_read(sc, a)
	struct eap_softc *sc;
	int a;
{
	int r;

	eap1371_src_wait(sc);
	r = EREAD4(sc, E1371_SRC) & (E1371_SRC_DISABLE | E1371_SRC_DISP1 |
				     E1371_SRC_DISP2 | E1371_SRC_DISREC);
	r |= E1371_SRC_ADDR(a);
	EWRITE4(sc, E1371_SRC, r);
	r = EREAD4(sc, E1371_SRC) & E1371_SRC_DATAMASK;
	return r;
}

void
eap1371_src_write(sc, a, d)
	struct eap_softc *sc;
	int a,d;
{
	int r;

	eap1371_src_wait(sc);
	r = EREAD4(sc, E1371_SRC) & (E1371_SRC_DISABLE | E1371_SRC_DISP1 |
				     E1371_SRC_DISP2 | E1371_SRC_DISREC);
	r |= E1371_SRC_RAMWE | E1371_SRC_ADDR(a) | E1371_SRC_DATA(d);
	EWRITE4(sc, E1371_SRC, r);
}

void
eap1371_set_adc_rate(sc, rate)
	struct eap_softc *sc;
	int rate;
{
	int freq, n, truncm;
	int out;

        /* Whatever, it works, so I'll leave it :) */

        if (rate > 48000)
            rate = 48000;
        if (rate < 4000)
            rate = 4000;
        n = rate / 3000;
        if ((1 << n) & SRC_MAGIC)
                n--;
        truncm = ((21 * n) - 1) | 1;
        freq = ((48000 << 15) / rate) * n;
        if (rate >= 24000) {
                if (truncm > 239)
                        truncm = 239;
		out = ESRC_SET_TRUNC((239 - truncm) / 2);
        } else {
                if (truncm > 119)
                        truncm = 119;
		out = ESRC_SMF | ESRC_SET_TRUNC((119 - truncm) / 2);
        }
 	out |= ESRC_SET_N(n);
        eap1371_src_write(sc, ESRC_ADC+ESRC_TRUNC_N, out);


        out = eap1371_src_read(sc, ESRC_ADC+ESRC_IREGS) & 0xff;
        eap1371_src_write(sc, ESRC_ADC+ESRC_IREGS, out |
			  ESRC_SET_VFI(freq >> 15));
        eap1371_src_write(sc, ESRC_ADC+ESRC_VFF, freq & 0x7fff);
        eap1371_src_write(sc, ESRC_ADC_VOLL, ESRC_SET_ADC_VOL(n));
        eap1371_src_write(sc, ESRC_ADC_VOLR, ESRC_SET_ADC_VOL(n));
}

void
eap1371_set_dac_rate(sc, rate, which)
	struct eap_softc *sc;
	int rate;
	int which;
{
        int dac = (which == 1) ? ESRC_DAC1 : ESRC_DAC2;
	int freq, r;

        /* Whatever, it works, so I'll leave it :) */

        if (rate > 48000)
            rate = 48000;
        if (rate < 4000)
            rate = 4000;
        freq = (rate << 15) / 3000;

        eap1371_src_wait(sc);
        r = EREAD4(sc, E1371_SRC) & (E1371_SRC_DISABLE |
            E1371_SRC_DISP2 | E1371_SRC_DISP1 | E1371_SRC_DISREC);
        r |= (which == 1) ? E1371_SRC_DISP1 : E1371_SRC_DISP2;
        EWRITE4(sc, E1371_SRC, r);
        r = eap1371_src_read(sc, dac + ESRC_IREGS) & 0x00ff;
        eap1371_src_write(sc, dac + ESRC_IREGS, r | ((freq >> 5) & 0xfc00));
        eap1371_src_write(sc, dac + ESRC_VFF, freq & 0x7fff);
        r = EREAD4(sc, E1371_SRC) & (E1371_SRC_DISABLE |
            E1371_SRC_DISP2 | E1371_SRC_DISP1 | E1371_SRC_DISREC);
        r &= ~((which == 1) ? E1371_SRC_DISP1 : E1371_SRC_DISP2);
        EWRITE4(sc, E1371_SRC, r);
}

void
eap_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct eap_softc *sc = (struct eap_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	struct audio_hw_if *eap_hw_if;
	char const *intrstr;
	pci_intr_handle_t ih;
	pcireg_t csr;
	char devinfo[256];
	mixer_ctrl_t ctl;
	int i;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

        /* Flag if we're "creative" */
	sc->sc_1371 = PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_ENSONIQ_AUDIOPCI97;

	/* Map I/O register */
	if (pci_mapreg_map(pa, PCI_CBIO, PCI_MAPREG_TYPE_IO, 0,
	      &sc->iot, &sc->ioh, NULL, NULL)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_dmatag = pa->pa_dmat;

	/* Enable the device. */
	csr = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
		       csr | PCI_COMMAND_MASTER_ENABLE);

	/* Map and establish the interrupt. */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_AUDIO, eap_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	if (!sc->sc_1371) {
		/* Enable interrupts and looping mode. */
		/* enable the parts we need */
		EWRITE4(sc, EAP_SIC, EAP_P2_INTR_EN | EAP_R1_INTR_EN);
		EWRITE4(sc, EAP_ICSC, EAP_CDC_EN);

		/* reset codec */
		/* normal operation */
		/* select codec clocks */
		eap_write_codec(sc, AK_RESET, AK_PD);
		eap_write_codec(sc, AK_RESET, AK_PD | AK_NRST);
		eap_write_codec(sc, AK_CS, 0x0);

		eap_hw_if = &eap1370_hw_if;

		/* Enable all relevant mixer switches. */
		ctl.dev = EAP_OUTPUT_SELECT;
		ctl.type = AUDIO_MIXER_SET;
		ctl.un.mask = 1 << EAP_VOICE_VOL | 1 << EAP_FM_VOL |
			1 << EAP_CD_VOL | 1 << EAP_LINE_VOL | 1 << EAP_AUX_VOL |
			1 << EAP_MIC_VOL;
		eap_hw_if->set_port(sc, &ctl);

		ctl.type = AUDIO_MIXER_VALUE;
		ctl.un.value.num_channels = 1;
		for (ctl.dev = EAP_MASTER_VOL; ctl.dev < EAP_MIC_VOL;
		     ctl.dev++) {
			ctl.un.value.level[AUDIO_MIXER_LEVEL_MONO] = VOL_0DB;
			eap_hw_if->set_port(sc, &ctl);
		}
		ctl.un.value.level[AUDIO_MIXER_LEVEL_MONO] = 0;
		eap_hw_if->set_port(sc, &ctl);
		ctl.dev = EAP_MIC_PREAMP;
		ctl.type = AUDIO_MIXER_ENUM;
		ctl.un.ord = 0;
		eap_hw_if->set_port(sc, &ctl);
		ctl.dev = EAP_RECORD_SOURCE;
		ctl.type = AUDIO_MIXER_SET;
		ctl.un.mask = 1 << EAP_MIC_VOL;
		eap_hw_if->set_port(sc, &ctl);
	} else {
                /* clean slate */
                EWRITE4(sc, EAP_SIC, 0);
                EWRITE4(sc, EAP_ICSC, 0);
                EWRITE4(sc, E1371_LEGACY, 0);

                /* Reset from es1371's perspective */
                EWRITE4(sc, EAP_ICSC, E1371_SYNC_RES);
                delay(20);
                EWRITE4(sc, EAP_ICSC, 0);

                /* must properly reprogram sample rate converter,
                 * or it locks up.  Set some defaults for the life of the
                 * machine, and set up a sb default sample rate.
                 */
                EWRITE4(sc, E1371_SRC, E1371_SRC_DISABLE);
                for (i=0; i<0x80; i++)
                        eap1371_src_write(sc, i, 0);
		eap1371_src_write(sc, ESRC_DAC1+ESRC_TRUNC_N, ESRC_SET_N(16));
		eap1371_src_write(sc, ESRC_DAC2+ESRC_TRUNC_N, ESRC_SET_N(16));
                eap1371_src_write(sc, ESRC_DAC1+ESRC_IREGS, ESRC_SET_VFI(16));
                eap1371_src_write(sc, ESRC_DAC2+ESRC_IREGS, ESRC_SET_VFI(16));
                eap1371_src_write(sc, ESRC_ADC_VOLL, ESRC_SET_ADC_VOL(16));
                eap1371_src_write(sc, ESRC_ADC_VOLR, ESRC_SET_ADC_VOL(16));
		eap1371_src_write(sc, ESRC_DAC1_VOLL, ESRC_SET_DAC_VOLI(1));
		eap1371_src_write(sc, ESRC_DAC1_VOLR, ESRC_SET_DAC_VOLI(1));
		eap1371_src_write(sc, ESRC_DAC2_VOLL, ESRC_SET_DAC_VOLI(1));
		eap1371_src_write(sc, ESRC_DAC2_VOLR, ESRC_SET_DAC_VOLI(1));
                eap1371_set_adc_rate(sc, 22050);
                eap1371_set_dac_rate(sc, 22050, 1);
                eap1371_set_dac_rate(sc, 22050, 2);

                EWRITE4(sc, E1371_SRC, 0);

                /* Reset codec */

		/* Interrupt enable */
		sc->host_if.arg = sc;
		sc->host_if.attach = eap1371_attach_codec;
		sc->host_if.read = eap1371_read_codec;
		sc->host_if.write = eap1371_write_codec;
		sc->host_if.reset = eap1371_reset_codec;

		if (ac97_attach(&sc->host_if) == 0) {
			/* Interrupt enable */
			EWRITE4(sc, EAP_SIC, EAP_P2_INTR_EN | EAP_R1_INTR_EN);
		} else
			return;

		eap_hw_if = &eap1371_hw_if;

		/* Just enable the DAC and master volumes by default */
		ctl.type = AUDIO_MIXER_ENUM;
		ctl.un.ord = 0;  /* off */
		ctl.dev = eap1371_get_portnum_by_name(sc, AudioCoutputs,
		       AudioNmaster, AudioNmute);
		eap1371_mixer_set_port(sc, &ctl);
		ctl.dev = eap1371_get_portnum_by_name(sc, AudioCinputs,
		       AudioNdac, AudioNmute);
		eap1371_mixer_set_port(sc, &ctl);
		ctl.dev = eap1371_get_portnum_by_name(sc, AudioCrecord,
		       AudioNvolume, AudioNmute);
		eap1371_mixer_set_port(sc, &ctl);


		ctl.dev = eap1371_get_portnum_by_name(sc, AudioCrecord,
		       AudioNsource, NULL);
		ctl.type = AUDIO_MIXER_ENUM;
		ctl.un.ord = 0;
		eap1371_mixer_set_port(sc, &ctl);

        }

	audio_attach_mi(eap_hw_if, sc, &sc->sc_dev);

#if NMIDI > 0
	midi_attach_mi(&eap_midi_hw_if, sc, &sc->sc_dev);
#endif
}

int
eap1371_attach_codec(sc_, codec_if)
	void *sc_;
	struct ac97_codec_if  *codec_if;
{
	struct eap_softc *sc = sc_;

	sc->codec_if = codec_if;
	return (0);
}

void
eap1371_reset_codec(sc_)
	void *sc_;
{
	struct eap_softc *sc = sc_;
	u_int32_t icsc = EREAD4(sc, EAP_ICSC);

	EWRITE4(sc, EAP_ICSC, icsc | E1371_SYNC_RES);
	delay(2);
	EWRITE4(sc, EAP_ICSC, icsc & ~E1371_SYNC_RES);
	delay(1);

	return;
}

int
eap_intr(p)
	void *p;
{
	struct eap_softc *sc = p;
	u_int32_t intr, sic;

	intr = EREAD4(sc, EAP_ICSS);
	if (!(intr & EAP_INTR))
		return (0);
	sic = EREAD4(sc, EAP_SIC);
	DPRINTFN(5, ("eap_intr: ICSS=0x%08x, SIC=0x%08x\n", intr, sic));
	if (intr & EAP_I_ADC) {
		/*
		 * XXX This is a hack!
		 * The EAP chip sometimes generates the recording interrupt
		 * while it is still transferring the data.  To make sure
		 * it has all arrived we busy wait until the count is right.
		 * The transfer we are waiting for is 8 longwords.
		 */
		int s, nw, n;
		EWRITE4(sc, EAP_MEMPAGE, EAP_ADC_PAGE);
		s = EREAD4(sc, EAP_ADC_CSR);
		nw = ((s & 0xffff) + 1) >> 2; /* # of words in DMA */
		n = 0;
		while (((EREAD4(sc, EAP_ADC_SIZE) >> 16) + 8) % nw == 0) {
			delay(10);
			if (++n > 100) {
				printf("eapintr: dma fix timeout");
				break;
			}
		}
		/* Continue with normal interrupt handling. */
		EWRITE4(sc, EAP_SIC, sic & ~EAP_R1_INTR_EN);
		EWRITE4(sc, EAP_SIC, sic);
		if (sc->sc_rintr)
			sc->sc_rintr(sc->sc_rarg);
	}
	if (intr & EAP_I_DAC2) {
		EWRITE4(sc, EAP_SIC, sic & ~EAP_P2_INTR_EN);
		EWRITE4(sc, EAP_SIC, sic);
		if (sc->sc_pintr)
			sc->sc_pintr(sc->sc_parg);
	}
#if NMIDI > 0
	if (intr & EAP_I_UART) {
		u_int32_t data;

		if (EREAD1(sc, EAP_UART_STATUS) & EAP_US_RXINT) {
			while (EREAD1(sc, EAP_UART_STATUS) & EAP_US_RXRDY) {
				data = EREAD1(sc, EAP_UART_DATA);
				if (sc->sc_iintr)
					sc->sc_iintr(sc->sc_arg, data);
			}
		}
	}
#endif
	return (1);
}

int
eap_allocmem(sc, size, align, p)
	struct eap_softc *sc;
	size_t size;
	size_t align;
	struct eap_dma *p;
{
	int error;

	p->size = size;
	error = bus_dmamem_alloc(sc->sc_dmatag, p->size, align, 0,
				 p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
				 &p->nsegs, BUS_DMA_NOWAIT);
	if (error)
		return (error);

	error = bus_dmamem_map(sc->sc_dmatag, p->segs, p->nsegs, p->size,
			       &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error)
		goto free;

	error = bus_dmamap_create(sc->sc_dmatag, p->size, 1, p->size,
				  0, BUS_DMA_NOWAIT, &p->map);
	if (error)
		goto unmap;

	error = bus_dmamap_load(sc->sc_dmatag, p->map, p->addr, p->size, NULL,
				BUS_DMA_NOWAIT);
	if (error)
		goto destroy;
	return (0);

destroy:
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
unmap:
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
free:
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
	return (error);
}

int
eap_freemem(sc, p)
	struct eap_softc *sc;
	struct eap_dma *p;
{
	bus_dmamap_unload(sc->sc_dmatag, p->map);
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
	return (0);
}

int
eap_open(addr, flags)
	void *addr;
	int flags;
{
	return (0);
}

/*
 * Close function is called at splaudio().
 */
void
eap_close(addr)
	void *addr;
{
	struct eap_softc *sc = addr;

	eap_halt_output(sc);
	eap_halt_input(sc);

	sc->sc_pintr = 0;
	sc->sc_rintr = 0;
}

int
eap_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return (0);
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 2:
		strcpy(fp->name, AudioEalaw);
		fp->encoding = AUDIO_ENCODING_ALAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 3:
		strcpy(fp->name, AudioEslinear);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 4:
		strcpy(fp->name, AudioEslinear_le);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return (0);
	case 5:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 6:
		strcpy(fp->name, AudioEslinear_be);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 7:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	default:
		return (EINVAL);
	}
}

int
eap_set_params(addr, setmode, usemode, play, rec)
	void *addr;
	int setmode, usemode;
	struct audio_params *play, *rec;
{
	struct eap_softc *sc = addr;
	struct audio_params *p;
	int mode;
	u_int32_t div;

	/*
	 * The es1370 only has one clock, so make the sample rates match.
	 */
	if (!sc->sc_1371) {
	    if (play->sample_rate != rec->sample_rate &&
		usemode == (AUMODE_PLAY | AUMODE_RECORD)) {
	    	if (setmode == AUMODE_PLAY) {
		    rec->sample_rate = play->sample_rate;
		    setmode |= AUMODE_RECORD;
		} else if (setmode == AUMODE_RECORD) {
		    play->sample_rate = rec->sample_rate;
		    setmode |= AUMODE_PLAY;
		} else
		    return (EINVAL);
	    }
	}

	for (mode = AUMODE_RECORD; mode != -1;
	     mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {
		if ((setmode & mode) == 0)
			continue;

		p = mode == AUMODE_PLAY ? play : rec;

		if (p->sample_rate < 4000 || p->sample_rate > 48000 ||
		    (p->precision != 8 && p->precision != 16) ||
		    (p->channels != 1 && p->channels != 2))
			return (EINVAL);

		p->factor = 1;
		p->sw_code = 0;
		switch (p->encoding) {
		case AUDIO_ENCODING_SLINEAR_BE:
			if (p->precision == 16)
				p->sw_code = swap_bytes;
			else
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			if (p->precision != 16)
				p->sw_code = change_sign8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (p->precision == 16) {
				if (mode == AUMODE_PLAY)
					p->sw_code = swap_bytes_change_sign16_le;
				else
					p->sw_code = change_sign16_swap_bytes_le;
			}
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			if (p->precision == 16)
				p->sw_code = change_sign16_le;
			break;
		case AUDIO_ENCODING_ULAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = mulaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_mulaw;
			break;
		case AUDIO_ENCODING_ALAW:
			if (mode == AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = alaw_to_slinear16_le;
			} else
				p->sw_code = ulinear8_to_alaw;
			break;
		default:
			return (EINVAL);
		}
	}

        if (sc->sc_1371) {
		eap1371_set_dac_rate(sc, play->sample_rate, 1);
		eap1371_set_dac_rate(sc, play->sample_rate, 2);
		eap1371_set_adc_rate(sc, rec->sample_rate);
	} else {
                /* Set the speed */
                DPRINTFN(2, ("eap_set_params: old ICSC = 0x%08x\n",
                             EREAD4(sc, EAP_ICSC)));
                div = EREAD4(sc, EAP_ICSC) & ~EAP_PCLKBITS;
                /*
                 * XXX
                 * The -2 isn't documented, but seemed to make the wall
                 * time match
                 * what I expect.  - mycroft
                 */
                if (usemode == AUMODE_RECORD)
                        div |= EAP_SET_PCLKDIV(EAP_XTAL_FREQ /
                                rec->sample_rate - 2);
                else
                        div |= EAP_SET_PCLKDIV(EAP_XTAL_FREQ /
                                play->sample_rate - 2);
                div |= EAP_CCB_INTRM;
                EWRITE4(sc, EAP_ICSC, div);
                DPRINTFN(2, ("eap_set_params: set ICSC = 0x%08x\n", div));
        }

	return (0);
}

int
eap_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	return (blk & -32);	/* keep good alignment */
}

int
eap_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct eap_softc *sc = addr;
	struct eap_dma *p;
	u_int32_t icsc, sic;
	int sampshift;

#ifdef DIAGNOSTIC
	if (sc->sc_prun)
		panic("eap_trigger_output: already running");
	sc->sc_prun = 1;
#endif

	DPRINTFN(1, ("eap_trigger_output: sc=%p start=%p end=%p "
            "blksize=%d intr=%p(%p)\n", addr, start, end, blksize, intr, arg));
	sc->sc_pintr = intr;
	sc->sc_parg = arg;

	icsc = EREAD4(sc, EAP_ICSC);
	EWRITE4(sc, EAP_ICSC, icsc & ~EAP_DAC2_EN);

	sic = EREAD4(sc, EAP_SIC);
	sic &= ~(EAP_P2_S_EB | EAP_P2_S_MB | EAP_INC_BITS);
	sic |= EAP_SET_P2_ST_INC(0) | EAP_SET_P2_END_INC(param->precision * param->factor / 8);
	sampshift = 0;
	if (param->precision * param->factor == 16) {
		sic |= EAP_P2_S_EB;
		sampshift++;
	}
	if (param->channels == 2) {
		sic |= EAP_P2_S_MB;
		sampshift++;
	}
	EWRITE4(sc, EAP_SIC, sic);

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("eap_trigger_output: bad addr %p\n", start);
		return (EINVAL);
	}

	DPRINTF(("eap_trigger_output: DAC2_ADDR=0x%x, DAC2_SIZE=0x%x\n",
		 (int)DMAADDR(p),
		 EAP_SET_SIZE(0, (((char *)end - (char *)start) >> 2) - 1)));
	EWRITE4(sc, EAP_MEMPAGE, EAP_DAC_PAGE);
	EWRITE4(sc, EAP_DAC2_ADDR, DMAADDR(p));
	EWRITE4(sc, EAP_DAC2_SIZE,
		EAP_SET_SIZE(0, (((char *)end - (char *)start) >> 2) - 1));

	EWRITE2(sc, EAP_DAC2_CSR, (blksize >> sampshift) - 1);

	EWRITE4(sc, EAP_ICSC, icsc | EAP_DAC2_EN);

	DPRINTFN(1, ("eap_trigger_output: set ICSC = 0x%08x\n", icsc));

	return (0);
}

int
eap_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct eap_softc *sc = addr;
	struct eap_dma *p;
	u_int32_t icsc, sic;
	int sampshift;

#ifdef DIAGNOSTIC
	if (sc->sc_rrun)
		panic("eap_trigger_input: already running");
	sc->sc_rrun = 1;
#endif

	DPRINTFN(1, ("eap_trigger_input: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n",
	    addr, start, end, blksize, intr, arg));
	sc->sc_rintr = intr;
	sc->sc_rarg = arg;

	icsc = EREAD4(sc, EAP_ICSC);
	EWRITE4(sc, EAP_ICSC, icsc & ~EAP_ADC_EN);

	sic = EREAD4(sc, EAP_SIC);
	sic &= ~(EAP_R1_S_EB | EAP_R1_S_MB);
	sampshift = 0;
	if (param->precision * param->factor == 16) {
		sic |= EAP_R1_S_EB;
		sampshift++;
	}
	if (param->channels == 2) {
		sic |= EAP_R1_S_MB;
		sampshift++;
	}
	EWRITE4(sc, EAP_SIC, sic);

	for (p = sc->sc_dmas; p && KERNADDR(p) != start; p = p->next)
		;
	if (!p) {
		printf("eap_trigger_input: bad addr %p\n", start);
		return (EINVAL);
	}

	DPRINTF(("eap_trigger_input: ADC_ADDR=0x%x, ADC_SIZE=0x%x\n",
		 (int)DMAADDR(p),
		 EAP_SET_SIZE(0, (((char *)end - (char *)start) >> 2) - 1)));
	EWRITE4(sc, EAP_MEMPAGE, EAP_ADC_PAGE);
	EWRITE4(sc, EAP_ADC_ADDR, DMAADDR(p));
	EWRITE4(sc, EAP_ADC_SIZE,
		EAP_SET_SIZE(0, (((char *)end - (char *)start) >> 2) - 1));

	EWRITE2(sc, EAP_ADC_CSR, (blksize >> sampshift) - 1);

	EWRITE4(sc, EAP_ICSC, icsc | EAP_ADC_EN);

	DPRINTFN(1, ("eap_trigger_input: set ICSC = 0x%08x\n", icsc));

	return (0);
}

int
eap_halt_output(addr)
	void *addr;
{
	struct eap_softc *sc = addr;
	u_int32_t icsc;

	DPRINTF(("eap: eap_halt_output\n"));
	icsc = EREAD4(sc, EAP_ICSC);
	EWRITE4(sc, EAP_ICSC, icsc & ~EAP_DAC2_EN);
#ifdef DIAGNOSTIC
	sc->sc_prun = 0;
#endif
	return (0);
}

int
eap_halt_input(addr)
	void *addr;
{
	struct eap_softc *sc = addr;
	u_int32_t icsc;

	DPRINTF(("eap: eap_halt_input\n"));
	icsc = EREAD4(sc, EAP_ICSC);
	EWRITE4(sc, EAP_ICSC, icsc & ~EAP_ADC_EN);
#ifdef DIAGNOSTIC
	sc->sc_rrun = 0;
#endif
	return (0);
}

int
eap_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = eap_device;
	return (0);
}

int
eap1371_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct eap_softc *sc = addr;

	return (sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp));
}

int
eap1371_mixer_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct eap_softc *sc = addr;

	return (sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp));
}

int
eap1371_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	struct eap_softc *sc = addr;

	return (sc->codec_if->vtbl->query_devinfo(sc->codec_if, dip));
}

int
eap1371_get_portnum_by_name(sc, class, device, qualifier)
	struct eap_softc *sc;
	char *class, *device, *qualifier;
{
	return (sc->codec_if->vtbl->get_portnum_by_name(sc->codec_if, class,
             device, qualifier));
}

void
eap_set_mixer(sc, a, d)
	struct eap_softc *sc;
	int a, d;
{
	eap_write_codec(sc, a, d);

        sc->sc_port[a] = d;
        DPRINTFN(1, ("eap_mixer_set_port port 0x%02x = 0x%02x\n", a, d));
}

int
eap_mixer_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct eap_softc *sc = addr;
	int lval, rval, l, r, la, ra;
	int l1, r1, l2, r2, m, o1, o2;

	if (cp->dev == EAP_RECORD_SOURCE) {
		if (cp->type != AUDIO_MIXER_SET)
			return (EINVAL);
		m = sc->sc_record_source = cp->un.mask;
		l1 = l2 = r1 = r2 = 0;
		if (m & (1 << EAP_VOICE_VOL))
			l2 |= AK_M_VOICE, r2 |= AK_M_VOICE;
		if (m & (1 << EAP_FM_VOL))
			l1 |= AK_M_FM_L, r1 |= AK_M_FM_R;
		if (m & (1 << EAP_CD_VOL))
			l1 |= AK_M_CD_L, r1 |= AK_M_CD_R;
		if (m & (1 << EAP_LINE_VOL))
			l1 |= AK_M_LINE_L, r1 |= AK_M_LINE_R;
		if (m & (1 << EAP_AUX_VOL))
			l2 |= AK_M2_AUX_L, r2 |= AK_M2_AUX_R;
		if (m & (1 << EAP_MIC_VOL))
			l2 |= AK_M_TMIC, r2 |= AK_M_TMIC;
		eap_set_mixer(sc, AK_IN_MIXER1_L, l1);
		eap_set_mixer(sc, AK_IN_MIXER1_R, r1);
		eap_set_mixer(sc, AK_IN_MIXER2_L, l2);
		eap_set_mixer(sc, AK_IN_MIXER2_R, r2);
		return (0);
	}
	if (cp->dev == EAP_OUTPUT_SELECT) {
		if (cp->type != AUDIO_MIXER_SET)
			return (EINVAL);
		m = sc->sc_output_source = cp->un.mask;
		o1 = o2 = 0;
		if (m & (1 << EAP_VOICE_VOL))
			o2 |= AK_M_VOICE_L | AK_M_VOICE_R;
		if (m & (1 << EAP_FM_VOL))
			o1 |= AK_M_FM_L | AK_M_FM_R;
		if (m & (1 << EAP_CD_VOL))
			o1 |= AK_M_CD_L | AK_M_CD_R;
		if (m & (1 << EAP_LINE_VOL))
			o1 |= AK_M_LINE_L | AK_M_LINE_R;
		if (m & (1 << EAP_AUX_VOL))
			o2 |= AK_M_AUX_L | AK_M_AUX_R;
		if (m & (1 << EAP_MIC_VOL))
			o1 |= AK_M_MIC;
		eap_set_mixer(sc, AK_OUT_MIXER1, o1);
		eap_set_mixer(sc, AK_OUT_MIXER2, o2);
		return (0);
	}
	if (cp->dev == EAP_MIC_PREAMP) {
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		if (cp->un.ord != 0 && cp->un.ord != 1)
			return (EINVAL);
		sc->sc_mic_preamp = cp->un.ord;
		eap_set_mixer(sc, AK_MGAIN, cp->un.ord);
		return (0);
	}
	if (cp->type != AUDIO_MIXER_VALUE)
		return (EINVAL);
	if (cp->un.value.num_channels == 1)
		lval = rval = cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
	else if (cp->un.value.num_channels == 2) {
		lval = cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
		rval = cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
	} else
		return (EINVAL);
	ra = -1;
	switch (cp->dev) {
	case EAP_MASTER_VOL:
		l = VOL_TO_ATT5(lval);
		r = VOL_TO_ATT5(rval);
		la = AK_MASTER_L;
		ra = AK_MASTER_R;
		break;
	case EAP_MIC_VOL:
		if (cp->un.value.num_channels != 1)
			return (EINVAL);
		la = AK_MIC;
		goto lr;
	case EAP_VOICE_VOL:
		la = AK_VOICE_L;
		ra = AK_VOICE_R;
		goto lr;
	case EAP_FM_VOL:
		la = AK_FM_L;
		ra = AK_FM_R;
		goto lr;
	case EAP_CD_VOL:
		la = AK_CD_L;
		ra = AK_CD_R;
		goto lr;
	case EAP_LINE_VOL:
		la = AK_LINE_L;
		ra = AK_LINE_R;
		goto lr;
	case EAP_AUX_VOL:
		la = AK_AUX_L;
		ra = AK_AUX_R;
	lr:
		l = VOL_TO_GAIN5(lval);
		r = VOL_TO_GAIN5(rval);
		break;
	default:
		return (EINVAL);
	}
	eap_set_mixer(sc, la, l);
	if (ra >= 0) {
		eap_set_mixer(sc, ra, r);
	}
	return (0);
}

int
eap_mixer_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	struct eap_softc *sc = addr;
	int la, ra, l, r;

	switch (cp->dev) {
	case EAP_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_SET)
			return (EINVAL);
		cp->un.mask = sc->sc_record_source;
		return (0);
	case EAP_OUTPUT_SELECT:
		if (cp->type != AUDIO_MIXER_SET)
			return (EINVAL);
		cp->un.mask = sc->sc_output_source;
		return (0);
	case EAP_MIC_PREAMP:
		if (cp->type != AUDIO_MIXER_ENUM)
			return (EINVAL);
		cp->un.ord = sc->sc_mic_preamp;
		return (0);
	case EAP_MASTER_VOL:
		l = ATT5_TO_VOL(sc->sc_port[AK_MASTER_L]);
		r = ATT5_TO_VOL(sc->sc_port[AK_MASTER_R]);
		break;
	case EAP_MIC_VOL:
		if (cp->un.value.num_channels != 1)
			return (EINVAL);
		la = ra = AK_MIC;
		goto lr;
	case EAP_VOICE_VOL:
		la = AK_VOICE_L;
		ra = AK_VOICE_R;
		goto lr;
	case EAP_FM_VOL:
		la = AK_FM_L;
		ra = AK_FM_R;
		goto lr;
	case EAP_CD_VOL:
		la = AK_CD_L;
		ra = AK_CD_R;
		goto lr;
	case EAP_LINE_VOL:
		la = AK_LINE_L;
		ra = AK_LINE_R;
		goto lr;
	case EAP_AUX_VOL:
		la = AK_AUX_L;
		ra = AK_AUX_R;
	lr:
		l = GAIN5_TO_VOL(sc->sc_port[la]);
		r = GAIN5_TO_VOL(sc->sc_port[ra]);
		break;
	default:
		return (EINVAL);
	}
	if (cp->un.value.num_channels == 1)
		cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = (l+r) / 2;
	else if (cp->un.value.num_channels == 2) {
		cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT]  = l;
		cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
	} else
		return (EINVAL);
	return (0);
}

int
eap_query_devinfo(addr, dip)
	void *addr;
	mixer_devinfo_t *dip;
{
	switch (dip->index) {
	case EAP_MASTER_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_VOICE_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_FM_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNfmsynth);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_CD_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_LINE_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_AUX_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNaux);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_MIC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = EAP_MIC_PREAMP;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return (0);
	case EAP_RECORD_SOURCE:
		dip->mixer_class = EAP_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_SET;
		dip->un.s.num_mem = 6;
		strcpy(dip->un.s.member[0].label.name, AudioNmicrophone);
		dip->un.s.member[0].mask = 1 << EAP_MIC_VOL;
		strcpy(dip->un.s.member[1].label.name, AudioNcd);
		dip->un.s.member[1].mask = 1 << EAP_CD_VOL;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = 1 << EAP_LINE_VOL;
		strcpy(dip->un.s.member[3].label.name, AudioNfmsynth);
		dip->un.s.member[3].mask = 1 << EAP_FM_VOL;
		strcpy(dip->un.s.member[4].label.name, AudioNaux);
		dip->un.s.member[4].mask = 1 << EAP_AUX_VOL;
		strcpy(dip->un.s.member[5].label.name, AudioNdac);
		dip->un.s.member[5].mask = 1 << EAP_VOICE_VOL;
		return (0);
	case EAP_OUTPUT_SELECT:
		dip->mixer_class = EAP_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNselect);
		dip->type = AUDIO_MIXER_SET;
		dip->un.s.num_mem = 6;
		strcpy(dip->un.s.member[0].label.name, AudioNmicrophone);
		dip->un.s.member[0].mask = 1 << EAP_MIC_VOL;
		strcpy(dip->un.s.member[1].label.name, AudioNcd);
		dip->un.s.member[1].mask = 1 << EAP_CD_VOL;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = 1 << EAP_LINE_VOL;
		strcpy(dip->un.s.member[3].label.name, AudioNfmsynth);
		dip->un.s.member[3].mask = 1 << EAP_FM_VOL;
		strcpy(dip->un.s.member[4].label.name, AudioNaux);
		dip->un.s.member[4].mask = 1 << EAP_AUX_VOL;
		strcpy(dip->un.s.member[5].label.name, AudioNdac);
		dip->un.s.member[5].mask = 1 << EAP_VOICE_VOL;
		return (0);
	case EAP_MIC_PREAMP:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->prev = EAP_MIC_VOL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNpreamp);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return (0);
	case EAP_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = EAP_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCoutputs);
		return (0);
	case EAP_RECORD_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = EAP_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCrecord);
		return (0);
	case EAP_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = EAP_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCinputs);
		return (0);
	}
	return (ENXIO);
}

void *
eap_malloc(addr, direction, size, pool, flags)
	void *addr;
	int direction;
	size_t size;
	int pool, flags;
{
	struct eap_softc *sc = addr;
	struct eap_dma *p;
	int error;

	p = malloc(sizeof(*p), pool, flags);
	if (!p)
		return (0);
	error = eap_allocmem(sc, size, 16, p);
	if (error) {
		free(p, pool);
		return (0);
	}
	p->next = sc->sc_dmas;
	sc->sc_dmas = p;
	return (KERNADDR(p));
}

void
eap_free(addr, ptr, pool)
	void *addr;
	void *ptr;
	int pool;
{
	struct eap_softc *sc = addr;
	struct eap_dma **pp, *p;

	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (KERNADDR(p) == ptr) {
			eap_freemem(sc, p);
			*pp = p->next;
			free(p, pool);
			return;
		}
	}
}

size_t
eap_round_buffersize(addr, direction, size)
	void *addr;
	int direction;
	size_t size;
{
	return (size);
}

int
eap_mappage(addr, mem, off, prot)
	void *addr;
	void *mem;
	int off;
	int prot;
{
	struct eap_softc *sc = addr;
	struct eap_dma *p;

	if (off < 0)
		return (-1);
	for (p = sc->sc_dmas; p && KERNADDR(p) != mem; p = p->next)
		;
	if (!p)
		return (-1);
	return (bus_dmamem_mmap(sc->sc_dmatag, p->segs, p->nsegs,
				off, prot, BUS_DMA_WAITOK));
}

int
eap_get_props(addr)
	void *addr;
{
	return (AUDIO_PROP_MMAP | AUDIO_PROP_INDEPENDENT |
                AUDIO_PROP_FULLDUPLEX);
}

#if NMIDI > 0
int
eap_midi_open(addr, flags, iintr, ointr, arg)
	void *addr;
	int flags;
	void (*iintr)__P((void *, int));
	void (*ointr)__P((void *));
	void *arg;
{
	struct eap_softc *sc = addr;
	u_int32_t uctrl;

	sc->sc_iintr = iintr;
	sc->sc_ointr = ointr;
	sc->sc_arg = arg;

	EWRITE4(sc, EAP_ICSC, EREAD4(sc, EAP_ICSC) | EAP_UART_EN);
	uctrl = 0;
	if (flags & FREAD)
		uctrl |= EAP_UC_RXINTEN;
#if 0
	/* I don't understand ../midi.c well enough to use output interrupts */
	if (flags & FWRITE)
		uctrl |= EAP_UC_TXINTEN; */
#endif
	EWRITE1(sc, EAP_UART_CONTROL, uctrl);

	return (0);
}

void
eap_midi_close(addr)
	void *addr;
{
	struct eap_softc *sc = addr;

	EWRITE1(sc, EAP_UART_CONTROL, 0);
	EWRITE4(sc, EAP_ICSC, EREAD4(sc, EAP_ICSC) & ~EAP_UART_EN);

	sc->sc_iintr = 0;
	sc->sc_ointr = 0;
}

int
eap_midi_output(addr, d)
	void *addr;
	int d;
{
	struct eap_softc *sc = addr;
	int x;

	for (x = 0; x != MIDI_BUSY_WAIT; x++) {
		if (EREAD1(sc, EAP_UART_STATUS) & EAP_US_TXRDY) {
			EWRITE1(sc, EAP_UART_DATA, d);
			return (0);
		}
		delay(MIDI_BUSY_DELAY);
	}
	return (EIO);
}

void
eap_midi_getinfo(addr, mi)
	void *addr;
	struct midi_info *mi;
{
	mi->name = "AudioPCI MIDI UART";
	mi->props = MIDI_PROP_CAN_INPUT;
}

#endif
