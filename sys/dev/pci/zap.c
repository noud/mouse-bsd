/* XXX include-file bug workarounds */
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <machine/cpu.h> /* for delay() */

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

/* Constants dependent on the hardware */

/* Number of card slots */
#define CARDS 4

/* Which PCI mapping registers map which space */
#define IO_MAPREG PCI_MAPREG_START
#define MEM_MAPREG (PCI_MAPREG_START+4)

/* Registers in IO space */
#define IOREG_CTL 0x00
#define IOREG_CTL_RESET1 0x01
#define IOREG_CTL_RESET2 0x06
#define IOREG_FRESHVER 0xcc
#define IOREG_CARDSEL 0xc8
#define IOREG_TEST_LO 0xc4
#define IOREG_TEST_HI 0xc8 /* equals _CARDSEL - WTF? */
#define IOREG_SYNC 0xc0
#define IOREG_SYNC_HALF_FSYNC 0x01
#define IOREG_AUX_CTL 0x02
#define IOREG_AUX_CTL_MAGIC 0xdf /* some kind of direction bits */
#define IOREG_AUX_INTRSTAT 0x06
#define IOREG_AUX_DATA_W 0x03
#define IOREG_AUX_DATA_R 0x07
#define IOREG_AUX_DATA_SELECT 0x04 /* chip select */
#define IOREG_AUX_DATA_CLOCK  0x08 /* bit-banged clock */
#define IOREG_AUX_DATA_DATI   0x10 /* input data (host -> chip) */
#define IOREG_AUX_DATA_DATO   0x20 /* output data (chip -> host) */
#define IOREG_DMA_WRITE_START 0x08
#define IOREG_DMA_WRITE_INTR 0x0c
#define IOREG_DMA_WRITE_END 0x10
#define IOREG_DMA_READ_START 0x18
#define IOREG_DMA_READ_INTR 0x1c
#define IOREG_DMA_READ_END 0x20
#define IOREG_AUX_FUNCTION 0x2b
#define IOREG_AUX_FUNCTION_MAGIC 0x04
#define IOREG_AUX_SERCTL 0x2d
#define IOREG_AUX_SERCTL_BIGENDIAN 0xc1
#define IOREG_AUX_FSCDELAY 0x2f
/* Per-card registers */
#define CREG_DIGITAL_LOOP 8
#define CREG_DIGITAL_LOOP_OFF 0
#define CREG_DC_DC_ENB 14
#define CREG_DC_DC_ENB_ON  0x00
#define CREG_DC_DC_ENB_OFF 0x10
#define CREG_MAX_LOOP_I 71
#define CREG_CAL_BASE 96
#define CREG_CAL_N 12
/* Per-card indirect registers */
#define CIND_FILTERBASE 35
#define CIND_FILTERCOUNT 5
#define CIND_SCRATCHPAD 97
/* "No Freshmaker" value for FRESHVER */
#define FRESHMAKER_MISSING 0x59
/* At and above this version, use IOREG_TEST_HI to test, else _LO */
#define FRESHMAKER_TEST_HI 0x70
/* Number of calibration registers */

/* End constants dependent on the hardware */

typedef struct enum {
		 CT_FXO = 1,
		 CT_FXS
		 } CARDTYPE;

typedef struct zap_softc SOFTC;
typedef struct zap_card CARD;

struct zap_card {
  int n;
  int present;
  CARDTYPE type;
  unsigned char calregs[CREG_CAL_N];
  } ;

struct zap_softc {
  struct device dev; /* XXX interface botch */
  bus_space_tag_t iot;
  bus_space_handle_t ioh;
  unsigned int auxshadow;
  int curcard;
  CARD cards[CARDS];
  } ;

static int zap_probe(struct device *parent, struct cfdata *match, void *aux)
{
 struct pci_attach_args *pa;

 pa = aux;
 if ( (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_DIGIUM) &&
      (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_DIGIUM_TDM400P) ) return(1);
 return(0);
}

static void aux_write(SOFTC *sc)
{
 bus_space_write_1(sc->iot,sc->ioh,IOREG_AUX_DATA_W,sc->auxshadow);
}

static void select_card(SOFTC *sc, int c)
{
 if (c != sc->curcard)
  { sc->curcard = c;
    bus_space_write_1(sc->iot,sc->ioh,IOREG_CARDSEL,1<<c);
  }
}

static void aux_bis_nw(SOFTC *sc, unsigned char bits)
{
 sc->auxshadow |= bits;
}

static void aux_bic_nw(SOFTC *sc, unsigned char bits)
{
 sc->auxshadow &= ~bits;
}

static void aux_bis(SOFTC *sc, unsigned char bits)
{
 aux_bis_nw(sc,bits);
 aux_write(sc);
}

static void aux_bic(SOFTC *sc, unsigned char bits)
{
 aux_bic_nw(sc,bits);
 aux_write(sc);
}

static void send_8(SOFTC *sc, unsigned char v)
{
 unsigned char bit;

 aux_bis(sc,IOREG_AUX_DATA_CLOCK);
 aux_bic(sc,IOREG_AUX_DATA_SELECT);
 for (bit=0x80;bit;bit>>=1)
  { if (v & bit) aux_bis_nw(sc,IOREG_AUX_DATA_DATI); else aux_bic_nw(sc,IOREG_AUX_DATA_DATI);
    aux_bic(sc,IOREG_AUX_DATA_CLOCK);
    aix_bis(sc,OIREG_AUX_DATA_CLOCK);
  }
 aux_bis(sc,IOREG_AUX_DATA_SELECT);
}

static unsigned char get_8(SOFTC *sc)
{
 unsigned char v;
 unsigned char bit;

 aux_bis(sc,IOREG_AUX_DATA_CLOCK);
 aux_bic(sc,IOREG_AUX_DATA_SELECT);
 v = 0;
 for (bit=0x80;bit;bit>>=1)
  { aux_bic(sc,IOREG_AUX_DATA_CLOCK);
    if (bus_space_read_1(sc->iot,sc->ioh,IOREG_AUX_DATA_R) & IOREG_AUX_DATA_DATO) v |= bit;
    aix_bis(sc,OIREG_AUX_DATA_CLOCK);
  }
 aux_bis(sc,IOREG_AUX_DATA_SELECT);
 aux_bic(sc,IOREG_AUX_DATA_CLOCK);
 return(v);
}

static unsigned char get_card_reg(SOFTC *sc, int c, int r)
{
 select_card(sc,c);
 switch (sc->cards[c].type)
  { case CT_FXS:
       send_8(sc,r|0x80);
       break;
    case CT_FXO:
       send_8(sc,0x60);
       send_8(sc,r&0x7f);
  }
 return(get_8(sc));
}

#define PROSLIC_OK   1 /* fine */
#define PROSLIC_NOT  2 /* "insane" - probably not a proslic */
#define PROSLIC_FAIL 3 /* looks there but failed */
static int init_proslic(SOFTC *sc, int cno, int fast, int manual, int sane)
{
 unsigned short int filters[CIND_FILTERCOUNT];
 int i;

 if (!sane && !proslic_sane(sc,cno)) return(PROSLIC_NOT);
 if (sane) set_card_reg(sc,cno,CREG_DC_DC_ENB,CREG_DC_DC_ENB_OFF);
 if (init_proslic_indirect(sc,cno))
  { printf("%s: module %d ProSLIC indirect init failed\n",&sc->dev.dv_xname[0],cno);
    return(PROSLIC_FAIL);
  }
 set_card_indirect(sc,cno,CIND_SCRATCHPAD,0);
 set_card_reg(sc,cno,CREG_DIGITAL_LOOP,CREG_DIGITAL_LOOP_OFF);
 /* "Revision C optimization" - what are the magic numbers? */
 set_card_reg(sc,cno,108,0xeb);
 /* "Disable automatic VBat switching" */
 set_card_reg(sc,cno,67,0x17);
 /* "Turn off Q7" */
 set_card_reg(sc,cno,66,1);
 for (i=0;i<CIND_FILTERCOUNT;i++)
  { filters[i] = get_card_indirect(sc,cno,CIND_FILTERBASE+i);
    set_card_indirect(sc,cno,CIND_FILTERBASE+i,0x8000);
  }
 if (init_proslic_powerup(sc,cno,fast))
  { printf("%s: module %d ProSLIC initial powerup failed\n",&sc->dev.dv_xname[0],cno);
    return(PROSLIC_FAIL);
  }
 if (fast)
  { for (i=0;i<CREG_CAL_N;i++)
     { set_card_reg(sc,cno,CREG_CAL_BASE+i,sc->cards[cno].calregs[i];
     }
  }
 else
  { if (init_proslic_leakcheck(sc,cno))
     { printf("%s: module %d ProSLIC leak check failed\n",&sc->dev.dv_xname[0],cno);
       return(PROSLIC_FAIL);
     }
    if (init_proslic_powerup(sc,cno,fast))
     { printf("%s: module %d ProSLIC final powerup failed\n",&sc->dev.dv_xname[0],cno);
       return(PROSLIC_FAIL);
     }
    /* Why calibrate twice?  That's what the doc says it needs. */
    if (manual)
     { if (init_proslic_calibrate_manual(sc,cno))
	{ if (init_proslic_calibrate_manual(sc,cno))
	   { printf("%s: module %d ProSLIC manual calibration failed\n",&sc->dev.dv_xname[0],cno);
	     return(PROSLIC_FAIL);
	   }
	  printf("%s: module %d ProSLIC manual calibration needed second try\n",&sc->dev.dv_xname[0],cno);
	}
     }
    else
     { if (init_proslic_calibrate_auto(sc,cno))
	{ if (init_proslic_calibrate_auto(sc,cno))
	   { printf("%s: module %d ProSLIC auto calibration failed\n",&sc->dev.dv_xname[0],cno);
	     return(PROSLIC_FAIL);
	   }
	  printf("%s: module %d ProSLIC auto calibration needed second try\n",&sc->dev.dv_xname[0],cno);
	}
     }
    /* "Perform DC-DC calibration" */
    set_card_reg(sc,cno,93,0x99);
    i = get_card_reg(sc,cno,107);
    if ((i < 2) || (i > 13))
     { printf("%s: module %d DC-DC has unusual value %#02x\n",&sc->dev.dv_xname[0],cno,i);
       set_card_reg(sc,cno,107,8);
     }
    for (i=0;i<CREG_CAL_N;i++)
     { sc->cards[cno].calregs[i] = get_card_reg(sc,cno,CREG_CAL_BASE+i);
     }
  }
 for (i=0;i<CIND_FILTERCOUNT;i++)
  { set_card_indirect(sc,cno,CIND_FILTERBASE+i,filters[i]);
  }
 if (verify_proslic_indirect(sc,cno))
  { printf("%s: module %d ProSLIC indirect verify failed\n",&sc->dev.dv_xname[0],cno);
    return(PROSLIC_FAIL);
  }
 set_card_reg(sc,cno,1,0x28);		/* "U-Law 8-bit interface" */
 set_card_reg(sc,cno,2,(3-cno)<<3);	/* "Tx Start count low byte 0" */
 set_card_reg(sc,cno,3,0);		/* "Tx Start count high byte 0" */
 set_card_reg(sc,cno,4,(3-cno)<<3);	/* "Rx Start count low byte 0" */
 set_card_reg(sc,cno,5,0);		/* "Rx Start count high byte 0" */
 set_card_reg(sc,cno,18,0xff);		/* "clear all interrupt" */
 set_card_reg(sc,cno,19,0xff);
 set_card_reg(sc,cno,20,0xff);
 set_card_reg(sc,cno,73,0x04);
 set_card_reg(sc,cno,64,0x01);
 return(PROSLIC_OK);
}

static void zap_attach(struct device *parent, struct device *self, void *aux)
{
 struct pci_attach_args *pa;
 SOFTC *sc;
 unsigned int sub_id;
 const char *rev;
 unsigned int ver;
 int cno;
 CARD *c;

 parent = parent;
 pa = aux;
 sc = (void *) self;
 sub_id = pci_conf_read(pa->pa_pc,pa->pa_tag,PCI_SUBSYS_ID_REG);
 switch (sub_id & 0xffff)
  { case 0xa159:
    case 0xe159:
       rev = "prototype";
       break;
    case 0xb100:
       rev = "rev E/F";
       break;
    case 0xb1d9:
    case 0xb118:
    case 0xb119:
       rev = "rev I";
       break;
    case 0xa9fd:
    case 0xa8fd:
    case 0xa800:
    case 0xa801:
    case 0xa908:
    case 0xa901:
       rev = "rev H";
       break;
    default:
       printf(": unknown rev %04x\n",sub_id&0xffff);
       return;
  }
 printf(": %s\n",rev);
#ifdef ZAP_DUMP_MAPREGS
  { int r;
    printf("%s\n",
	(pa->pa_flags & PCI_FLAGS_IO_ENABLED) ? "IO enabled"
					      : "IO not enabled" );
    printf("%s\n",
	(pa->pa_flags & PCI_FLAGS_MEM_ENABLED) ? "memory enabled"
					       : "memory not enabled" );
    for (r=PCI_MAPREG_START;r<PCI_MAPREG_END;r+=4)
     { unsigned int mr;
       mr = pci_conf_read(pa->pa_pc,pa->pa_tag,r);
       printf("%02x: %08x",r,mr);
       switch (PCI_MAPREG_TYPE(mr))
	{ case PCI_MAPREG_TYPE_MEM:
	     printf(": mem");
	     switch (PCI_MAPREG_MEM_TYPE(mr))
	      { case PCI_MAPREG_MEM_TYPE_32BIT:
		   printf("32");
		   break;
		case PCI_MAPREG_MEM_TYPE_32BIT_1M:
		   printf("32/1M");
		   break;
		case PCI_MAPREG_MEM_TYPE_64BIT:
		   printf("64");
		   break;
		default:
		   printf("?");
		   break;
	      }
	     printf(", %s, addr %08x size %08x\n",
		PCI_MAPREG_MEM_PREFETCHABLE(mr) ? "prefetchable"
						: "not prefetchable",
		PCI_MAPREG_MEM_ADDR(mr),
		PCI_MAPREG_MEM_SIZE(mr) );
	     break;
	  case PCI_MAPREG_TYPE_IO:
	     printf(": io, addr %08x size %08x\n",
		PCI_MAPREG_IO_ADDR(mr),
		PCI_MAPREG_IO_SIZE(mr) );
	     break;
	}
     }
  }
#endif /* ifdef ZAP_DUMP_MAPREGS */
 if (pci_mapreg_map(pa,IO_MAPREG,PCI_MAPREG_TYPE_IO,0,&sc->iot,&sc->ioh,0,0))
  { printf("%s: can't map I/O space\n",&sc->dev.dv_xname[0]);
    return;
  }
 sc->curcard = -1;
 bus_space_write_1(sc->iot,sc->ioh,IOREG_CTL,IOREG_CTL_RESET1);
 bus_space_read_1(sc->iot,sc->ioh,IOREG_CTL);
 ver = bus_space_read_1(sc->iot,sc->ioh,IOREG_FRESHVER);
 if (ver == FRESHMAKER_MISSING)
  { printf("%s: no Freshmaker\n",&sc->dev.dv_xname[0]);
  }
 else
  { int w;
    int r;
    int o;
    printf("%s: Freshmaker ver %02x\n",&sc->dev.dv_xname[0],ver);
    o = (ver >= FRESHMAKER_TEST_HI) ? IOREG_TEST_HI : IOREG_TEST_LO;
    for (w=0;w<255;w++)
     { bus_space_write_1(sc->iot,sc->ioh,o,w);
       r = bus_space_read_1(sc->iot,sc->ioh,o);
       if (r != w)
	{ printf("%s: Freshmaker test failed (wrote %02x read %02x)\n",
		&sc->dev.dv_xname[0],w,r);
	  return;
	}
     }
    bus_space_write_1(sc->iot,sc->ioh,IOREG_SYNC,IOREG_SYNC_HALF_FSYNC);
    bus_space_read_1(sc->iot,sc->ioh,IOREG_SYNC);
  }
 bus_space_write_1(sc->iot,sc->ioh,IOREG_CTL,IOREG_CTL_RESET2);
 sc->auxshadow = IOREG_AUX_DATA_SELECT | IOREG_AUX_DATA_CLOCK | IOREG_AUX_DATA_DATI;
 aux_write(sc);
 bus_space_write_1(sc->iot,sc->ioh,IOREG_AUX_CTL,IOREG_AUX_CTL_MAGIC);
 bus_space_write_1(sc->iot,sc->ioh,IOREG_AUX_FUNCTION,IOREG_AUX_FUNCTION_MAGIC);
 delay(250000);
 /* "Back to normal, with automatic DMA wrap around" */
 bus_space_write_1(sc->iot,sc->ioh,IOREG_CTL,0x31);
 /* "Make sure serial port and DMA are out of reset" */
 bus_space_write_1(sc->iot,sc->ioh,IOREG_CTL,
	bus_space_read_1(sc->iot,sc->ioh,IOREG_CTL)&~IOREG_CTL_RESET2);
 bus_space_write_1(sc->iot,sc->ioh,IOREG_AUX_SERCTL,IOREG_AUX_SERCTL_BIGENDIAN);
 bus_space_write_1(sc->iot,sc->ioh,IOREG_AUX_FSCDELAY,0);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_WRITE_START,???);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_WRITE_INTR,???);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_WRITE_END,???);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_READ_START,???);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_READ_INTR,???);
 bus_space_write_4(sc->iot,sc->ioh,IOREG_DMA_READ_END,???);
 bus_space_write_1(sc->iot,sc->ioh,IOREG_INTRSTAT,0xff);
 delay(250000);
 sc->present = 0;
 for (cno=0;cno<CARDS;cno++)
  { int rv;
    c = &sc->cards[cno];
    c->n = cno;
    c->present = 0;
    rv = init_proslic(sc,cno,0,0,0);
    if (rv == 0)
     { c->present = 1;
       c->type = CT_FXS;
       rv = get_card_reg(sc,c,CREG_MAX_LOOP_I);
       printf("%s: module %d is auto FXS/DPO, loop current %dmA\n",&sc->dev.dv_xname[0],cno,(rv*3)+20);
     }
  }
 bus_space_write_1(sc->iot,sc->ioh,IOREG_CTL,
	bus_space_read_1(sc->iot,sc->ioh,IOREG_CTL)&~IOREG_CTL_RESET1);
}

struct cfattach zap_ca
 = { sizeof(SOFTC), &zap_probe, &zap_attach };
