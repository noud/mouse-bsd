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

int auvia_match(struct device *, struct cfdata *, void *);
void auvia_attach(struct device *, struct device *, void *);

#define	AUVIA_PLAY_BASE   0x40
#define	AUVIA_RECORD_BASE 0x10

struct auvia_dma {
  struct auvia_dma *next;
  caddr_t addr;
  size_t size;
  bus_dmamap_t map;
  bus_dma_segment_t seg;
  } ;

struct auvia_dma_op {
  uint32_t ptr;
  uint32_t flags;
#define AUVIA_DMAOP_EOL		0x80000000
#define AUVIA_DMAOP_FLAG	0x40000000
#define AUVIA_DMAOP_STOP	0x20000000
#define AUVIA_DMAOP_COUNT(x)	((x)&0x00FFFFFF)
  } ;

struct auvia_softc_chan {
  void (*intr)(void *);
  void *arg;
  struct auvia_dma_op *dma_ops;
  struct auvia_dma *dma_ops_dma;
  u_int16_t dma_op_count;
  u_int16_t reg;
  int base;
  } ;

struct auvia_softc {
  struct device dev;
  char revisioni[8];
  void *ih;
  bus_space_tag_t iot;
  bus_space_handle_t ioh;
  bus_dma_tag_t dmat;
  struct ac97_codec_if *codec_if;
  struct ac97_host_if host_if;
  struct auvia_dma *dmas;
  struct auvia_softc_chan play;
  struct auvia_softc_chan record;
#define AUVIA_NFORMATS 8
  struct audio_format formats[AUVIA_NFORMATS];
  struct audio_encoding_set *encodings;
  } ;

struct cfattach eap_ca
 = { sizeof(struct auvia_softc), auvia_match, auvia_attach };

static const struct audio_hw_if auvia_hw_if; /* forward */

const struct audio_hw_if auvia_hw_if
 = { 0, /* open */
     0, /* close */
     0, /* drain */
     &auvia_query_encoding,
     &auvia_set_params,
     &auvia_round_blocksize,
     0, /* commit_settings */
     0, /* init_output */
     0, /* init_input */
     0, /* start_output */
     0, /* start_input */
     &auvia_halt_output,
     &auvia_halt_input,
     0, /* speaker_ctl */
     &auvia_getdev,
     0, /* setfd */
     &auvia_set_port,
     &auvia_get_port,
     &auvia_query_devinfo,
     &auvia_malloc,
     &auvia_free,
     &auvia_round_buffersize,
     &auvia_mappage,
     &auvia_get_props,
     &auvia_trigger_output,
     &auvia_trigger_input,
     0, /* dev_ioctl */ };

#define AUVIA_FORMATS_4CH_16 2
#define AUVIA_FORMATS_6CH_16 3
#define AUVIA_FORMATS_4CH_8  6
#define AUVIA_FORMATS_6CH_8  7
static const struct audio_format auvia_formats[AUVIA_NFORMATS]
 = { {0, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
       1, AUFMT_MONAURAL, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
       2, AUFMT_STEREO, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
       4, AUFMT_SURROUND4, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
       6, AUFMT_DOLBY_5_1, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR_LE, 8, 8,
       1, AUFMT_MONAURAL, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY | AUMODE_RECORD, AUDIO_ENCODING_ULINEAR_LE, 8, 8,
       2, AUFMT_STEREO, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY, AUDIO_ENCODING_ULINEAR_LE, 8, 8,
       4, AUFMT_SURROUND4, 0, { 8000, 48000 } },
     {0, AUMODE_PLAY, AUDIO_ENCODING_SLINEAR_LE, 8, 8,
       6, AUFMT_DOLBY_5_1, 0, { 8000, 48000 } } };

struct audio_device auvia_device
 = { "AUVIA", "", "auvia" };

int eap_match(struct device *parent, struct cfdata *match, void *aux)
{
 struct pci_attach_args *pa;

 pa = aux;
 return( (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_VIATECH) &&
	 (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_VIATECH_VT8233_AC97) );
}

void auvia_attach(struct device *parent, struct device *self, void *aux)
{
 struct pci_attach_args *pa;
 struct auvia_softc *sc;
 const char *intrstr;
 pci_chipset_tag_t pc;
 pcitag_t pt;
 pci_intr_handle_t ih;
 bus_size_t iosize;
 pcireg_t pr;
 int r;
 const char *revnum;

 pa = aux;
 sc = (struct auvia_softc *) self; /* XXX interface botch */
 intrstr = 0;
 pc = pa->pa_pc;
 pt = pa->pa_tag;
 revnum = 0;
 printf(": Audio controller\n");
 sc->sc_play.sc_base = AUVIA_PLAY_BASE;
 sc->sc_record.sc_base = AUVIA_RECORD_BASE;
 if (pci_mapreg_map(pa,0x10,PCI_MAPREG_TYPE_IO,0,&sc->iot,&sc->ioh,0,&iosize))
  { printf(": can't map i/o space\n");
    return;
  }
 sc->dmat = pa->pa_dmat;
 sc->sc_pc = pc;
 sc->sc_pt = pt;
 r = PCI_REVISION(pa->pa_class);
 snprintf(sc->sc_revision,sizeof(sc->sc_revision),"0x%02X",r);
 switch (r)
  { case VIA_REV_8233C:
       /* 2 rec, 4 pb, 1 multi-pb */
       revnum = "3C";
       break;
    case VIA_REV_8233:
       /* 2 rec, 4 pb, 1 multi-pb, spdif */
       revnum = "3";
       break;
    case VIA_REV_8233A:
       /* 1 rec, 1 multi-pb, spdif */
       revnum = "3A";
       break;
    default:
       break;
  }
 if (r >= VIA_REV_8237)
  { revnum = "7";
  }
 else if (r >= VIA_REV_8235) /* 2 rec, 4 pb, 1 multi-pb, spdif */
  { revnum = "5";
  }
 printf(": VIA Technologies VT823%s AC'97 Audio (rev %s)\n",revnum,sc->sc_revision);
 if (pci_intr_map(pa,&ih))
  { printf(": couldn't map interrupt\n");
    return;
  }
 intrstr = pci_intr_string(pc,ih);
 sc->sc_ih = pci_intr_establish(pc,ih,IPL_AUDIO,&auvia_intr,sc);
 if (sc->sc_ih == 0)
  { printf("%s: couldn't establish interrupt",sc->sc_dev.dv_xname);
    if (intrstr != 0) printf(" at %s",intrstr);
    printf("\n");
    return;
  }
 printf("%s: interrupting at %s\n",&sc->sc_dev.dv_xname[0],intrstr);
 /* disable SBPro compat & others */
 pr = pci_conf_read(pc,pt,AUVIA_PCICONF_JUNK);
 pr &= ~AUVIA_PCICONF_ENABLES; /* clear compat function enables */
 /* XXX what to do about MIDI, FM, joystick? */
 pr |= AUVIA_PCICONF_ACLINKENAB | AUVIA_PCICONF_ACNOTRST |
		AUVIA_PCICONF_ACVSR | AUVIA_PCICONF_ACSGD;
 pr &= ~(AUVIA_PCICONF_ACFM | AUVIA_PCICONF_ACSB);
 pci_conf_write(pc,pt,AUVIA_PCICONF_JUNK,pr);
 sc->host_if.arg = sc;
 sc->host_if.attach = &auvia_attach_codec;
 sc->host_if.read = &auvia_read_codec;
 sc->host_if.write = &auvia_write_codec;
 sc->host_if.reset = &auvia_reset_codec;
 r = ac97_attach(&sc->host_if,self);
 if (r)
  { printf("%s: can't attach codec (error 0x%X)\n",&sc->sc_dev.dv_xname[0],r);
    return;
  }
 /* setup audio_format */
 memcpy(sc->sc_formats,auvia_formats,sizeof(auvia_formats));
 if (! AC97_IS_4CH(sc->codec_if))
  { AUFMT_INVALIDATE(&sc->sc_formats[AUVIA_FORMATS_4CH_8]);
    AUFMT_INVALIDATE(&sc->sc_formats[AUVIA_FORMATS_4CH_16]);
  }
 if (! AC97_IS_6CH(sc->codec_if))
  { AUFMT_INVALIDATE(&sc->sc_formats[AUVIA_FORMATS_6CH_8]);
    AUFMT_INVALIDATE(&sc->sc_formats[AUVIA_FORMATS_6CH_16]);
  }
 if (AC97_IS_FIXED_RATE(sc->codec_if))
  { for (r=0;r<AUVIA_NFORMATS;r++)
     { sc->sc_formats[r].frequency_type = 1;
       sc->sc_formats[r].frequency[0] = 48000;
     }
  }
 if (auconv_create_encodings(sc->sc_formats,AUVIA_NFORMATS,&sc->sc_encodings))
  { (*sc->codec_if->vtbl->detach)(sc->codec_if);
    return;
  }
 audio_attach_mi(&auvia_hw_if,sc,&sc->sc_dev);
}
