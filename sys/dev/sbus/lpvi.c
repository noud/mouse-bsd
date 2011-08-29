/*
 * Driver for SUNW,lpvi SPARCprinter interface.
 *
 * Not SMP-ready; locks against itself with spl*().
 *
 * This file is in the public domain.
 */

#include <sys/proc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <machine/bus.h>
#include <machine/conf.h>
#include <dev/sbus/sbusvar.h>

#include <dev/sbus/lpviio.h>

/*
 * Device registers.  The registers named with r__* names I don't know
 *  enough about to confidently put names on.  The 8-byte hole that's
 *  called "pad" here may correspond to registers or it may not; if
 *  there's anything there, I have no idea what.
 *
 * Some of the csr bits I have felt confident enough of to put names
 *  on.  The others I don't understand.  I suspect the 0x3 bits of
 *  being interrupt request bits, but am unsure.  CSR__23 I have no
 *  clue about the meaning of.  I routinely see the 0x00400000 bit set
 *  in csr values read from the device, but have no idea what, if
 *  anything, it means.
 *
 * There is an unpleasant property that bytw must be no less than 18.
 *  We ensure this by enforcing a minimum width of 144 pixels.  For
 *  code symmetry, we also define MIN_H, as 1.
 */
#define MIN_W 144
#define MIN_H 1
struct lpvi_device {
  unsigned int csr;
#define CSR___0   0x00000001
#define CSR___1   0x00000002
#define CSR_IE    0x00000010	/* interrupt enable */
#define CSR_RESET 0x00000080	/* pulse high to reset */
#define CSR_DMA   0x00000200	/* enable DMA engine? */
#define CSR__23   0x00800000
#define CSR_IREV  0xf0000000	/* interface revision */
#define CSR_IREV_SHIFT 28
  unsigned int dmaaddr;		/* address for page bitmap DMA */
  unsigned int pad[2];
  unsigned short int r__10;
  unsigned short int bytw;	/* width of page bitmap, bytes */
  unsigned short int wid;	/* width of page bitmap, pixels */
  unsigned short int hgt;	/* height of page bitmap, pixels */
  unsigned short int yoff;	/* top margin, pixels */
  unsigned short int xoff;	/* left margin, pixels */
  unsigned short int r__1c;
  unsigned char r__1e;
  unsigned char r__1f;
  unsigned char cmd;		/* command to printer (PCMD_xxx) */
  unsigned char r__21;
  } ;

/*
 * The device is exclusive-open, but we have to lock against possible
 *  multiple accesses anyway, because multiple processes can have file
 *  descriptors on the device through fork(), sendmsg(), etc.
 */
struct lpvi_softc {
  /*
   * SBus device boilerplate.
   */
  struct device dev; /* XXX interface botch */
  struct sbusdev sbdev;
  /*
   * Pointer to device registers.
   */
  volatile struct lpvi_device *regs;
  /*
   * Some software state flags.
   */
  unsigned int flags;
#define LF_IMG_INV  0x00000001	/* invert image data */
#define LF_MRG_INV  0x00000002	/* invert margin */
#define LF_CDISP    0x00000004	/* display is host-controlled */
#define LF_OPENING  0x00000008	/* someone in lpviopen() */
#define LF_OPEN     0x00000010	/* someone has it open */
#define LF_CMDWAIT  0x00000020	/* someone waiting for command to finish */
#define LF_LOCK     0x00000040	/* someone has it locked */
#define LF_LOCKWANT 0x00000080	/* someone blocked waiting for lock */
#define LF_PRINTING 0x00000100	/* page print in process */
#define LF_CMDACK   0x00000200	/* command sent but no ack interrupt yet */
  /*
   * Flag bits that survive closing and re-opening.
   */
#define LF__LONGTERM (LF_IMG_INV|LF_MRG_INV|LF_CDISP)
  /*
   * Verbosity of driver messages.  This holds zero or more of the
   *  LPVIMSG_* bits, as used by the LPVIIOC_{G,S}MSGS ioctls.
   */
  unsigned int msgs;
  /*
   * DPI and paper size parameters.  dpi is 300 or 400, and papersize
   *  is 0 through 7.  Both are set to -1 briefly during lpviopen(),
   *  but should otherwise always be valid.  pagesize points to an
   *  element of the pagesizes array (vide infra) appropriate to dpi
   *  and papersize.
   */
  int dpi;
  int papersize;
  const struct lpvi_pagesize *pagesize;
  /*
   * Current page setup.  usetup is what was set with LPVIIOC_SSETUP;
   *  csetup is the current setup in use.  The lp_default values both
   *  track whether the default page setup is in effect (see the
   *  comment on the lpvi_pagesetup structure in lpviio.h).  Page setup
   *  values survive close/reopen.  If lp_default is set, the other
   *  elements of usetup are meaningless, but the other elements of
   *  csetup describe the default page layout for the current dpi and
   *  papersize values.
   */
  struct lpvi_pagesetup usetup;
  struct lpvi_pagesetup csetup;
  /*
   * The printer passes data back by interrupting and providing a byte
   *  of data in a register.  datan and datastate control the state
   *  machine that handles this data stream.  datan is the number of
   *  bytes received in this state so far; datastate says what sort of
   *  data the driver expects.  If datastate is DS_NONE, no data bytes
   *  (or no more data bytes) are expected, and datan is meaningless.
   */
  int datan;
  int datastate;	/* we expect... */
#define DS_NONE   0	/* nothing */
#define DS_PING   1	/* status byte */
#define DS_QPAPER 2	/* response to tray query */
#define DS_QDPI   3	/* response to dpi query */
  /*
   * bus_dma and bus_space glue
   */
  bus_dma_tag_t dmatag;
  bus_dma_segment_t dmaseg;
  bus_dmamap_t dmamap;
  /*
   * Page buffer.  Because the card can't do scatter/gather DMA, the
   *  page bitmap has to be contiguous from the point of view of the
   *  card's DMA engine.  I think we might be able to DMA directly out
   *  of the user's buffer, if it's virtually contiguous (thanks to
   *  either dvma or the iommu), but this way we place no restrictions
   *  on the user buffer.  We allocate this at attach time; if you have
   *  an lpvi, this driver requires that you be prepared to dedicate
   *  2.626- megs of memory to page bitmaps.  (The largest page bitmap
   *  possible is 3928x5608, which is 491 bytes per row times 5608
   *  rows, or 2753528 bytes - a hair under 2.625969 megabytes.)
   */
  char *pbuf;
  } ;

/* Test to see whether a command is in progress. */
#define CMD_IN_PROGRESS(sc) \
	(((sc)->flags & LF_CMDACK) || ((sc)->datastate != DS_NONE))

/*
 * The values we use for the cmd register.
 */
#define PCMD_DISP_AUTO 0xc0 /* printer drives display */
#define PCMD_DISP_HOST 0xc2 /* host drives display */
#define PCMD_SET300    0xce /* set to 300dpi */
#define PCMD_SET400    0xcf /* set to 400dpi */
#define PCMD_QDPI      0xdb /* query dpi setting */
#define PCMD_QPAPER    0xf7 /* query paper tray presence/size */
#define PCMD_PRINT     0xfe /* print a page */
#define PCMD_PING      0xff /* check presence/communication */
/*
 * Other values I know of, and what I know of their functionality.  "No
 *  response" means that no data bytes are expected back; they all
 *  respond with an acknowledgement interrupt.  In cases where status
 *  bitmasks are given with short names for the bits, I don't have more
 *  elaborate descriptions of what they mean; all I have is the short
 *  names.  Most of them do not list meanings for all possible bits; I
 *  have no idea what, if anything, the unlisted bits mean.
 *
 *	0xf6	Query page count.
 *		Reponds with three bytes.  The first one seems to be
 *		some kind of format byte, and is 0xff.  The other two
 *		are the current page count, in BCD(!), in units of
 *		hundreds of pages, with the higher two digits first.
 *		For example, if the three bytes, are 0xff 0x72 0x18, in
 *		that order, the page count is in the range
 *		[721800..721899].  I know of no way to get the low two
 *		digits.
 *
 *	0xf2	Select main paper tray.  No response.
 *
 *	0xf1	Select manual-feed paper.  No response.
 *
 *	0xde	Query failure status.
 *		Responds with one byte.  The 0xf0 bits encode failure
 *		status.
 *			0x80: xerofail
 *			0x40: efuser
 *			0x20: eros
 *			0x10: emotor
 *
 *	0xda	Query jam status.
 *		Responds with one byte.  The 0x18 bits encode paper jam
 *		status.
 *			0x10: misfeed
 *			0x08: xitjam
 *
 *	0xd9	Query paper/tray missing status.
 *		Responds with one byte.  The 0x18 bits encode why there
 *		is no paper.
 *			0x10: nopapr
 *			0x08: notray
 *
 *	0xd5	Query for drum/toner status.
 *		Responds with one byte.  The 0x60 bits encode
 *		drum/toner low status.
 *			0x40: toner low
 *			0x20: drum low
 *		I don't understand how the drum can be "low", but
 *		perhaps that just means I don't know the print engine
 *		in question.
 *
 *	0xd4	Query drum/toner failure status.
 *		Responds with one byte.  The 0x78 bits encode
 *		drum/toner failure status.  (I have some reason to
 *		think "deve" means "developer", ie, toner.)
 *			0x40: edevex
 *			0x20: edrumx
 *			0x10: nodeve
 *			0x08: nodrum
 *
 * In addition, commands 0xc0 and 0xc2 appear to control the display,
 *  with 0x00, 0x01, 0x80, and 0x81 being related.  The printer has a
 *  two-digit display.  My best guess is that 0xc0 causes the display
 *  to be driven by the printer internally, while 0xc2 causes it to be
 *  driven by what the host has set.  The host sets what is to be
 *  displayed in the latter case by sending four bytes; the first byte
 *  is 0x80 or 0x00, the second byte is data for digit A, the third is
 *  0x81 or 0x01, and the fourth is data for digit B.  I'm not sure
 *  what the 0x80 bit means in the first and third bytes, though I have
 *  some reason to think it means "blink".  I'm also not sure which is
 *  digit A and which is digit B (I'm not even certain that it's as
 *  simple as digit A being one of the two digits and digit B being the
 *  other), nor do I know the encoding of the second and fourth bytes.
 */

/*
 * The present driver is very simplistic; it supports at most one lpvi.
 *  Change these variables and the add_unit() / lookup_unit() code to
 *  fix this.
 */
static int lpvi_unit = -1;
static struct lpvi_softc *lpvi_sc = 0;

/*
 * The possible page sizes.  The printer supports two resolutions and
 *  eight paper sizes (well, seven paper sizes, plus "tray not
 *  present"), hence the [2][8] indexing.  Values in the struct are in
 *  pixels; the width and height values in the comments are in inches.
 */
static const struct lpvi_pagesize pagesizes[2][8]
				/* name         width   x height   */
 = { /* 300dpi */
     { { 2944, 4208 },		/* none         9.81333 x 14.02666 */
       { 2944, 4208 },		/* b4 sef       9.81333 x 14.02666 */
       { 2392, 3416 },		/* a4 sef       7.97333 x 11.38666 */
       { 2464, 3208 },		/* letter sef   8.21333 x 10.69333 */
       { 2394, 1656 },		/* a5 lef       7.98000 x  5.52000 */
       { 2056, 2943 },		/* b5 sef       6.85333 x  9.81000 */
       { 2464, 4112 },		/* legal 14"    8.21333 x 13.70666 */
       { 2464, 3808 } },	/* legal 13"    8.21333 x 12.69333 */
     /* 400dpi */
     { { 3928, 5608 },		/* none         9.82    x 14.02    */
       { 3928, 5608 },		/* b4 sef       9.82    x 14.02    */
       { 3184, 4552 },		/* a4 sef       7.96    x 11.38    */
       { 3280, 4280 },		/* letter sef   8.20    x 10.70    */
       { 3184, 2208 },		/* a5 lef       7.96    x  5.52    */
       { 2744, 3928 },		/* b5 sef       6.86    x  9.82    */
       { 3280, 5480 },		/* legal 14"    8.20    x 13.70    */
       { 3280, 5080 } } };	/* legal 13"    8.20    x 12.70    */

/*
 * The largest width and height from the above table.  Given what we
 *  use these for, we mostly care that ((MAX_W+7)>>3)*MAX_H is the
 *  largest value any of the above entries calls for; fortunately, the
 *  largest width and the largest height occur in the same entry, so
 *  there is no question what the right numbers are.
 */
#define MAX_W 3928
#define MAX_H 5608

/*
 * Utility routine, called whenever we might be in the process of
 *  printing a page and have either finished or aborted the print.
 *  Pulled out into a separate routine to ensure it's done uniformly in
 *  the multiple places it needs doing.
 */
static void endprint(struct lpvi_softc *sc, const char *tag)
{
 int s;

 s = splhigh();
 if (sc->flags & LF_PRINTING)
  { sc->flags &= ~LF_PRINTING;
    wakeup(sc);
    if (tag && (sc->msgs & LPVIMSG_INTR)) printf(" [%s]",tag);
    bus_dmamap_sync(sc->dmatag,sc->dmamap,0,
		sc->dmamap->dm_segs[0].ds_len,BUS_DMASYNC_POSTWRITE);
    bus_dmamap_unload(sc->dmatag,sc->dmamap);
  }
 splx(s);
}

/*
 * Set the 0x3 bits in r__10 according to the inversion flags.
 */
static void setinv(struct lpvi_softc *sc)
{
 if (sc->flags & LF_IMG_INV) sc->regs->r__10 |= 1; else sc->regs->r__10 &= ~1;
 if (sc->flags & LF_MRG_INV) sc->regs->r__10 |= 2; else sc->regs->r__10 &= ~2;
}

/*
 * Reset the hardware.
 *
 * I have no idea what the CSR__23 bit does, or what the magic numbers
 *  5 and 0x27 mean.
 */
static void lpvireset(struct lpvi_softc *sc, int enbint)
{
 sc->regs->csr |= CSR_RESET;
 sc->regs->csr &= ~CSR_RESET;
 sc->regs->csr = enbint ? CSR__23|CSR_IE : CSR__23;
 setinv(sc);
 sc->regs->r__1c = 5;
 sc->regs->r__1e = 0x27;
 endprint(sc,0);
}

/*
 * Loads usetup into csetup, converting if necessary based on the
 *  current resolution.  Also clips csetup to pagesize, and enforces
 *  the minimum width and height values.
 */
static void load_csetup(struct lpvi_softc *sc)
{
 int end;

 if (sc->usetup.lp_default)
  { sc->csetup.lp_dpi = sc->dpi;
    sc->csetup.lp_wid = sc->pagesize->lp_wid;
    sc->csetup.lp_hgt = sc->pagesize->lp_hgt;
    sc->csetup.lp_xoff = 0;
    sc->csetup.lp_yoff = 0;
    sc->csetup.lp_default = 1;
    return;
  }
 if (sc->usetup.lp_dpi == sc->dpi)
  { sc->csetup = sc->usetup;
  }
 else
  { sc->csetup.lp_dpi = sc->dpi;
    end = ((sc->usetup.lp_xoff + sc->usetup.lp_wid) * sc->dpi) / sc->usetup.lp_dpi;
    sc->csetup.lp_xoff = (sc->usetup.lp_xoff * sc->dpi) / sc->usetup.lp_dpi;
    sc->csetup.lp_wid = end - sc->csetup.lp_xoff;
    end = ((sc->usetup.lp_yoff + sc->usetup.lp_hgt) * sc->dpi) / sc->usetup.lp_dpi;
    sc->csetup.lp_yoff = (sc->usetup.lp_yoff * sc->dpi) / sc->usetup.lp_dpi;
    sc->csetup.lp_hgt = end - sc->csetup.lp_yoff;
    sc->csetup.lp_default = 0;
  }
 if (sc->csetup.lp_wid < MIN_W) sc->csetup.lp_wid = MIN_W;
 if (sc->csetup.lp_hgt < MIN_H) sc->csetup.lp_hgt = MIN_H;
 if (sc->csetup.lp_xoff+sc->csetup.lp_wid > sc->pagesize->lp_wid)
  { if (sc->csetup.lp_xoff+MIN_W > sc->pagesize->lp_wid)
     { sc->csetup.lp_xoff = sc->pagesize->lp_wid - MIN_W;
       sc->csetup.lp_wid = MIN_W;
     }
    else
     { sc->csetup.lp_wid = sc->pagesize->lp_wid - sc->csetup.lp_xoff;
     }
  }
 if (sc->csetup.lp_yoff+sc->csetup.lp_hgt > sc->pagesize->lp_hgt)
  { if (sc->csetup.lp_yoff+MIN_H > sc->pagesize->lp_hgt)
     { sc->csetup.lp_yoff = sc->pagesize->lp_hgt - MIN_H;
       sc->csetup.lp_hgt = MIN_H;
     }
    else
     { sc->csetup.lp_hgt = sc->pagesize->lp_hgt - sc->csetup.lp_yoff;
     }
  }
}

/*
 * Update the pagesize pointer based on the dpi and papersize values.
 *  The reason this function tolerates "impossible" values for dpi and
 *  papersize is that it's called from lpvi_hard when the dpi and
 *  papersize are first loaded, in lpviopen, and one is updated before
 *  the other.
 */
static void update_pagesize(struct lpvi_softc *sc)
{
 int dpix;
 int psz;

 switch (sc->dpi)
  { case 300: dpix = 0; break;
    case 400: dpix = 1; break;
    default: return; break;
  }
 psz = sc->papersize;
 if ((psz < 0) || (psz > 7)) return;
 sc->pagesize = &pagesizes[dpix][psz];
 load_csetup(sc);
}

/*
 * Hard interrupt routine.
 *
 * Most of the possible error conditions are handled poorly if at all.
 *  I don't have broken printers to test them with, and little to no
 *  documentation on what the right actions to take are.
 *
 * When comments say "tray setting", they mean not paper size, but
 *  rather "main tray" versus "auxiliary tray" versus "manual feed".
 *  I've seen some indications that an auxiliary tray is a possibility
 *  and other indications that it isn't; I certainly don't have one for
 *  my printer as far as I can tell.
 *
 * If LF_CMDWAIT is set and the command is done, we clear it and
 *  wakeup().
 *
 * Note that the datastate/datan state machine runs even if LF_CMDWAIT
 *  is clear, so that if we signaled out of a command, we don't
 *  misprocess bytes coming back from the printer as a result.
 */
static int lpvi_hard(void *scvp)
{
 struct lpvi_softc *sc;
 volatile struct lpvi_device *r;
 unsigned int csr;
 unsigned short int v10;
 unsigned short int serstat;

 sc = scvp;
 r = sc->regs;
 csr = r->csr;
 if (! (csr & (CSR___0|CSR___1))) return(0);
 v10 = r->r__10;
 if (csr & CSR___1)
  { if (sc->msgs & LPVIMSG_INTR) printf("lpvi_hard: dma error (csr %08x)\n",csr);
    return(1);
  }
 serstat = r->r__1c;
 if (sc->msgs & LPVIMSG_INTR) printf("lpvi_hard: csr %08x v10 %04x serstat %04x:",csr,v10,serstat);
 if (serstat & 0x0700)
  { if (sc->msgs & LPVIMSG_INTR) printf(" [serial error]");
    r->r__1c = serstat | 0x0700;
  }
 if (v10 & 0x0400)
  { unsigned char pstat;
    int dstate;
    pstat = r->r__1f;
    if (sc->msgs & LPVIMSG_INTR) printf(" [pstat %02x]",pstat);
    /*
     * Most of these cases set datastate to DS_NONE, so do that here
     *  and let the exceptions set it themselves.
     */
    dstate = sc->datastate;
    sc->datastate = DS_NONE;
    switch (dstate)
     { case DS_NONE:
	  if (sc->msgs & LPVIMSG_INTR) printf(" [gratuitous]");
	  /* fall through */
       case DS_PING:
	  if ((pstat & 0xf0) != 0x40)
	   { sc->papersize = (pstat >> 1) & 7;
	     update_pagesize(sc);
	   }
	  switch (pstat >> 4)
	   { case 0x4:
		/* tray setting in 0x06 bits; ignore for now */
		sc->datastate = DS_QPAPER;
		sc->datan = 1;
		break;
	     case 0x8:
		if (sc->msgs & LPVIMSG_INTR) printf(" [engcold]");
		if (0)
		 {
	     case 0xa:
		   if (sc->msgs & LPVIMSG_INTR) printf(" [misfeed]");
		 }
		if (0)
		 {
	     case 0xb:
		   if (sc->msgs & LPVIMSG_INTR) printf(" [ilckopen]");
		 }
		if (0)
		 {
	     case 0xe:
		   if (sc->msgs & LPVIMSG_INTR) printf(" [emotor]");
		 }
		if (0)
		 {
	     case 0xf:
		   if (sc->msgs & LPVIMSG_INTR) printf(" [badprint?]");
		 }
		endprint(sc,"abort print");
		break;
	     case 0xc:
		endprint(sc,"printed");
		break;
	     default:
		if (sc->msgs & LPVIMSG_INTR) printf(" [unrecognized]");
		break;
	   }
	  break;
       case DS_QPAPER:
	  switch (sc->datan)
	   { case 0:
		/* tray setting in 0x06 bits; ignore for now */
		sc->datastate = DS_QPAPER;
		sc->datan = 1;
		break;
	     case 1:
		sc->papersize = (pstat >> 1) & 7;
		update_pagesize(sc);
		break;
	   }
	  break;
       case DS_QDPI:
	  switch ((pstat >> 3) & 3)
	   { case 1:
		sc->dpi = 300;
		break;
	     case 2:
		sc->dpi = 400;
		break;
	     default:
		if (sc->msgs & LPVIMSG_INTR) printf(" [bad dpi value]");
		break;
	   }
	  update_pagesize(sc);
	  break;
       default:
	  if (sc->msgs & LPVIMSG_INTR) printf(" [datastate=%d?]",dstate);
	  break;
     }
  }
 if (v10 & 0x0200)
  { /* This appears to be a DMA-done interrupt */
    if (sc->msgs & LPVIMSG_INTR) printf(" [0200 cleanup]");
    r->csr &= ~CSR_DMA;
    r->r__10 |= 0x0200;
  }
 if (v10 & 0x1000)
  { /* This appears to be an ack for a command in the cmd register. */
    if (sc->msgs & LPVIMSG_INTR) printf(" [1000 cleanup]");
    r->r__10 |= 0x1000;
    if (sc->flags & LF_CMDACK) sc->flags &= ~LF_CMDACK;
  }
 if ((sc->flags & LF_CMDWAIT) && !CMD_IN_PROGRESS(sc))
  { sc->flags &= ~LF_CMDWAIT;
    wakeup(sc);
    if (sc->msgs & LPVIMSG_INTR) printf(" [done]");
  }
 if (sc->msgs & LPVIMSG_INTR) printf("\n");
 return(1);
}

/*
 * add_unit() and lookup_unit() manage the mapping betwen device minor
 *  numbers and struct lpvi_softcs.  At the moment they are idiotically
 *  simple.  If you want to support multiple lpvis in a single machine,
 *  you should be able to do it by frobbing this code and the lpvi_unit
 *  and lpvi_sc variables.
 */

static void add_unit(int unit, struct lpvi_softc *sc)
{
 lpvi_unit = unit;
 lpvi_sc = sc;
}

static struct lpvi_softc *lookup_unit(int u)
{
 return((u==lpvi_unit)?lpvi_sc:0);
}

/*
 * Send a command to the printer.  See the cmd register and the PCMD_
 *  defines (and the comments nearby) for more.  The wakeup for the
 *  tsleep here is in lpvi_hard().  Note that we can be signaled out
 *  of; if this happens, we clear LF_CMDWAIT but don't bash datastate,
 *  so that when the printer responds the response is parsed correctly.
 *
 * Return value is an errno.
 */
static int lpvicmd(struct lpvi_softc *sc, unsigned char cmd, int state)
{
 volatile struct lpvi_softc *vsc;
 int s;
 int e;

 vsc = sc;
 if (sc->msgs & LPVIMSG_CMD)
  { printf("lpvicmd %02x ",cmd);
    switch (state)
     { case DS_NONE:   printf("NONE");   break;
       case DS_PING:   printf("PING");   break;
       case DS_QPAPER: printf("QPAPER"); break;
       case DS_QDPI:   printf("QDPI");   break;
       default: printf("%d",state); break;
     }
    printf("\n");
  }
 s = splhigh();
 if (CMD_IN_PROGRESS(vsc))
  { vsc->flags |= LF_CMDWAIT;
    while (vsc->flags & LF_CMDWAIT)
     { e = tsleep(sc,PZERO|PCATCH,"lpvicmd",10*hz);
       if (e)
	{ vsc->flags &= ~LF_CMDWAIT;
	  splx(s);
	  if (e == EWOULDBLOCK) e = ETIMEDOUT;
	  return(e);
	}
     }
  }
 sc->regs->cmd = cmd;
 vsc->datan = 0;
 vsc->datastate = state;
 vsc->flags |= LF_CMDWAIT | LF_CMDACK;
 while (vsc->flags & LF_CMDWAIT)
  { e = tsleep(sc,PZERO|PCATCH,"lpvicmd",10*hz);
    if (e)
     { vsc->flags &= ~LF_CMDWAIT;
       splx(s);
       if (e == EWOULDBLOCK) e = ETIMEDOUT;
       return(e);
     }
  }
 splx(s);
 return(0);
}

/*
 * Lock a unit.  The high-level driver entry points all lock, so that
 *  we don't have to worry about fine-grained serialization of
 *  operations.  Arguably this is the Wrong Thing....
 */
static int lpvi_lock(volatile struct lpvi_softc *vsc)
{
 int s;
 int e;

 s = splhigh();
 while (vsc->flags & LF_LOCK)
  { vsc->flags |= LF_LOCKWANT;
    e = tsleep((void *)vsc,PZERO|PCATCH,"lpvilock",0);
    if (e) return(e);
  }
 vsc->flags |= LF_LOCK;
 splx(s);
 return(0);
}

/*
 * Release the lock lpvi_lock() took.
 */
static void lpvi_unlock(volatile struct lpvi_softc *vsc)
{
 int s;

 s = splhigh();
 if (! (vsc->flags & LF_LOCK)) panic("lpvi_unlock: not locked");
 vsc->flags &= ~LF_LOCK;
 if (vsc->flags & LF_LOCKWANT)
  { vsc->flags &= ~LF_LOCKWANT;
    wakeup((void *)vsc);
  }
 splx(s);
}

/*
 * Fetch the dpi setting from the printer.  If we don't get it (printer
 *  error such as timeout, signal, whatever), use what's in the softc,
 *  for lack of anything better.  Return value is an errno.
 */
static int getdpi(struct lpvi_softc *sc)
{
 return(lpvicmd(sc,PCMD_QDPI,DS_QDPI));
}

/*
 * Like getdpi() but for paper size.
 */
static int getpapersize(struct lpvi_softc *sc)
{
 return(lpvicmd(sc,PCMD_QPAPER,DS_QPAPER));
}

/*
 * Set resolution.  Returns an errno.
 */
static int newresolution(struct lpvi_softc *sc, int res)
{
 volatile struct lpvi_softc *vsc;
 unsigned char cmd;
 int e;

 vsc = sc;
 switch (res)
  { case 300:
       cmd = PCMD_SET300;
       if (0)
	{
    case 400:
	  cmd = PCMD_SET400;
	}
       e = lpvicmd(sc,cmd,DS_NONE);
       if (e) return(e);
       vsc->dpi = res;
       update_pagesize(sc);
       break;
    default:
       return(EINVAL);
       break;
  }
 return(e);
}

/*
 * Install a new page setup structure.  Returns an errno.  Mostly just
 *  handling defaulting, but there are also a few limits to enforce and
 *  suchlike.  Note that we permit page setups that are not usable with
 *  the current paper size; userland may prompt a human for a new paper
 *  size before printing.  We let load_csetup clip as necessary.
 */
static int newpagesetup(struct lpvi_softc *sc, struct lpvi_pagesetup *ps)
{
 struct lpvi_pagesetup new;
 int e;

 if (ps->lp_default)
  { sc->usetup.lp_default = 1;
    load_csetup(sc);
    return(0);
  }
 if ( (ps->lp_dpi != LPVI_NOCHANGE) &&
      (ps->lp_wid == LPVI_NOCHANGE) &&
      (ps->lp_hgt == LPVI_NOCHANGE) &&
      (ps->lp_xoff == LPVI_NOCHANGE) &&
      (ps->lp_yoff == LPVI_NOCHANGE) ) return(newresolution(sc,ps->lp_dpi));
 new.lp_dpi = (ps->lp_dpi == LPVI_NOCHANGE) ? sc->dpi : ps->lp_dpi;
 new.lp_wid = (ps->lp_wid == LPVI_NOCHANGE) ? sc->csetup.lp_wid : ps->lp_wid;
 new.lp_hgt = (ps->lp_hgt == LPVI_NOCHANGE) ? sc->csetup.lp_hgt : ps->lp_hgt;
 new.lp_xoff = (ps->lp_xoff == LPVI_NOCHANGE) ? sc->csetup.lp_xoff : ps->lp_xoff;
 new.lp_yoff = (ps->lp_yoff == LPVI_NOCHANGE) ? sc->csetup.lp_yoff : ps->lp_yoff;
 new.lp_default = 0;
 switch (new.lp_dpi)
  { case 300:
    case 400:
       break;
    default:
       return(EINVAL);
       break;
  }
 if ( (new.lp_wid < MIN_W) ||
      (new.lp_hgt < MIN_H) ||
      (new.lp_xoff+new.lp_wid > MAX_W) ||
      (new.lp_yoff+new.lp_hgt > MAX_H) ) return(EINVAL);
 if (new.lp_dpi != sc->dpi)
  { e = newresolution(sc,new.lp_dpi);
    if (e) return(e);
  }
 sc->usetup = new;
 load_csetup(sc);
 return(0);
}

/*
 * Device-driver interfaces.  Mostly pretty boring.
 */

/*
 * About the only thing of note here is that we reset and ping the
 *  printer on open.
 */
int lpviopen(dev_t dev, int flag, int mode, struct proc *p)
{
 struct lpvi_softc *sc;
 volatile struct lpvi_softc *vsc;
 int e;
 int s;

 sc = lookup_unit(minor(dev));
 if (sc == 0) return(ENXIO);
 vsc = sc;
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviopen\n");
 s = splhigh();
 if (vsc->flags & (LF_OPEN|LF_OPENING))
  { splx(s);
    return(EBUSY);
  }
 if (vsc->flags & ~LF__LONGTERM) panic("%s impossible flags",&sc->dev.dv_xname[0]);
 vsc->flags |= LF_OPENING | LF_LOCK;
 splx(s);
 lpvireset(sc,1);
 e = lpvicmd(sc,PCMD_PING,DS_PING);
 switch (e)
  { case 0:
       break;
    case EINTR:
    case ERESTART:
       sc->flags &= LF__LONGTERM;
       return(e);
       break;
    case EWOULDBLOCK:
       sc->flags &= LF__LONGTERM;
       return(ETIMEDOUT);
       break;
    default:
       sc->flags &= LF__LONGTERM;
       return(EIO);
       break;
  }
 lpvireset(sc,1);
 sc->papersize = -1;
 sc->dpi = -1;
 sc->pagesize = 0;
 e = getdpi(sc);
 if (e == 0) e = getpapersize(sc);
 if ((e == 0) && (sc->pagesize == 0)) e = EIO;
 if (e)
  { sc->flags &= LF__LONGTERM;
    return(e);
  }
 sc->flags = (sc->flags | LF_OPEN) & ~(LF_OPENING | LF_LOCK);
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviopen: ok\n");
 return(0);
}

/*
 * Nothing noteworthy here: reset and disable interrupts.
 */
int lpviclose(dev_t dev, int flag, int mode, struct proc *p)
{
 struct lpvi_softc *sc;

 sc = lookup_unit(minor(dev));
 if (sc == 0) panic("closing nonexistent lpvi");
 if (! (sc->flags & LF_OPEN)) panic("closing unopened lpvi");
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviclose\n");
 lpvireset(sc,0);
 sc->flags &= LF__LONGTERM;
 flag=flag;mode=mode;p=p;
 return(0);
}

/*
 * We don't do read(); this isn't a scanner. :-)
 */
int lpviread(dev_t dev, struct uio *uio, int ioflag)
{
 struct lpvi_softc *sc;

 sc = lookup_unit(minor(dev));
 if (sc == 0) panic("reading from nonexistent lpvi");
 if (! (sc->flags & LF_OPEN)) panic("reading from unopened lpvi");
 if (uio->uio_rw != UIO_READ) panic("lpviread non-read uio");
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviread\n");
 ioflag=ioflag; /* suppress "argument ioflag not used" */
 return(EIO);
}

/*
 * write() is the interface to sending a page to the printer.  Arguably
 *  we should permit building up a page with multiple writes(), using
 *  uio->uio_offset to place them in the page bitmap and shipping the
 *  page off to the printer only when it's done.  Perhaps someday.
 *
 * Printing a page seems to involve doing four things: setting the DMA
 *  address, setting a CSR bit, writing into an otherwise unused
 *  register, and sending a magic command.  It seems to me that except
 *  for the DMA address, any one of those would have been sufficient,
 *  but nooooo....  (Arguably, they could all have been rolled into
 *  one; there's no reason in principle that loading the DMA address
 *  couldn't also be the "go" signal.)
 */
int lpviwrite(dev_t dev, struct uio *uio, int ioflag)
{
 struct lpvi_softc *sc;
 volatile struct lpvi_softc *vsc;
 int e;
 int bw;
 int n;
 char *pbuf;
 int s;

 sc = lookup_unit(minor(dev));
 if (sc == 0) panic("writing to nonexistent lpvi");
 if (! (sc->flags & LF_OPEN)) panic("writing to unopened lpvi");
 if (uio->uio_rw != UIO_WRITE) panic("lpviwrite non-write uio");
 vsc = sc;
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviwrite\n");
 e = lpvi_lock(sc);
 if (e) return(e);
 s = splhigh();
 /* this can happen if we signaled out of a previous write */
 while (vsc->flags & LF_PRINTING)
  { e = tsleep(sc,PZERO|PCATCH,"lpviwrite1",0);
    if (e)
     { splx(s);
       lpvi_unlock(sc);
       return(e);
     }
  }
 splx(s);
 do
  { if (sc->csetup.lp_wid > MAX_W)
     { if (sc->msgs & LPVIMSG_EPNT) printf("lpvi: paper width %d > MAX_W %d\n",sc->pagesize->lp_wid,MAX_W);
       e = EIO;
     }
    if (sc->csetup.lp_hgt > MAX_H)
     { if (sc->msgs & LPVIMSG_EPNT) printf("lpvi: paper height %d > MAX_H %d\n",sc->pagesize->lp_hgt,MAX_H);
       e = EIO;
     }
    if (e) break;
    bw = (sc->csetup.lp_wid + 7) >> 3;
    n = bw * sc->csetup.lp_hgt;
    if (uio->uio_resid < n)
     { e = EMSGSIZE; /* XXX needs translation */
       break;
     }
    /*
     * The lpvi's DMA engine seems to do something funky I don't
     *  understand if the transfer ends on or one byte before a page
     *	boundary (depending, it seems, on the high bit of the
     *	address(!)).  So make sure this doesn't happen.  I suspect
     *	using 2047 may be overly conservative; it perhaps should be
     *	4095.  I'm being somewhat paranoid.
     *
     * This is why the +16 when allocating pbuf.
     */
    pbuf = sc->pbuf;
    switch (2047 & (unsigned int)(pbuf+n))
     { case 0:
       case 2047:
	  pbuf += 16;
	  break;
     }
    /*
     * I have no idea why, but observed fact is that the first data
     *  byte is skipped.  So we uiomove to one byte beyond the address
     *	corresponding to what we stuff in regs->dmaaddr.  It may be
     *	that the DMA engine increments before fetching or something
     *	stupid like that, or it may be something subtler.
     *
     * It might be good enough to just subtract 1 before loading
     *  dmaaddr - but if the DMA engine actually fetches the "first
     *	byte", even if it gets ignored later, we could (in principle)
     *	get spurious DMA errors if we did that.
     *
     * This is why the +1 when allocating pbuf.
     */
    e = uiomove(pbuf+1,n,uio);
    if (e) break;
    /* The sync and unload calls to pair with these are in endprint(). */
    e = bus_dmamap_load(sc->dmatag,sc->dmamap,pbuf,n,0,BUS_DMA_WAITOK);
    if (e) break;
    if (sc->dmamap->dm_nsegs > 1) panic("lpviwrite: multiple segs");
    bus_dmamap_sync(sc->dmatag,sc->dmamap,0,
		sc->dmamap->dm_segs[0].ds_len,BUS_DMASYNC_PREWRITE);
    vsc->flags |= LF_PRINTING;
    sc->regs->bytw = bw;
    sc->regs->wid = sc->csetup.lp_wid;
    sc->regs->hgt = sc->csetup.lp_hgt;
    sc->regs->yoff = sc->csetup.lp_yoff;
    sc->regs->xoff = sc->csetup.lp_xoff;
    sc->regs->dmaaddr = sc->dmamap->dm_segs[0].ds_addr;
    sc->regs->csr |= CSR_DMA;
    sc->regs->r__21 = 0;
    lpvicmd(sc,PCMD_PRINT,DS_NONE);
    e = 0;
    s = splhigh();
    /* The wakeup for this tsleep is in endprint(). */
    while (vsc->flags & LF_PRINTING)
     { e = tsleep(sc,PZERO|PCATCH,"lpviwrite2",0);
       if (e)
	{ e = EINTR;
	  break;
	}
     }
    splx(s);
    if (e) break;
  } while (0);
 lpvi_unlock(sc);
 ioflag=ioflag;
 return(e);
}

/*
 * Interface to userland beyond "print a page".  Mostly pretty boring.
 */
int lpviioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 struct lpvi_softc *sc;
 volatile struct lpvi_softc *vsc;
 int s;
 int e;
 int v;

 sc = lookup_unit(minor(dev));
 if (sc == 0) panic("ioctl on nonexistent lpvi");
 vsc = sc;
 if (sc->msgs & LPVIMSG_EPNT) printf("lpviioctl\n");
 s = splhigh();
 if (! (vsc->flags & LF_OPEN)) panic("ioctl on unopened lpvi");
 e = lpvi_lock(vsc);
 splx(s);
 if (e) return(e);
 switch (cmd)
  { case LPVIIOC_GRES:
    case LPVIIOC_GSIZE:
    case LPVIIOC_GMSGS:
    case LPVIIOC_GSETUP:
    case LPVIIOC_GFLAGS:
       if (! (flag & FREAD)) e = EBADF;
       break;
    case LPVIIOC_RESET:
    case LPVIIOC_SRES:
    case LPVIIOC_SMSGS:
    case LPVIIOC_SSETUP:
    case LPVIIOC_SFLAGS:
    case LPVIIOC_SDISP:
       if (! (flag & FWRITE)) e = EBADF;
       break;
  }
 if (e)
  { lpvi_unlock(vsc);
    return(e);
  }
 switch (cmd)
  { case LPVIIOC_PING:
       e = lpvicmd(sc,PCMD_PING,DS_PING);
       break;
    case LPVIIOC_RESET:
       lpvireset(sc,1);
       vsc->flags &= ~LF_CMDWAIT;
       e = getdpi(sc);
       if (e == 0) e = getpapersize(sc);
       break;
    case LPVIIOC_GRES:
       if (vsc->dpi < 0) e = getdpi(sc);
       if ((e == 0) && (vsc->dpi < 0)) e = EIO;
       if (e == 0)
	{ v = vsc->dpi;
	  *(int *)data = v;
	}
       break;
    case LPVIIOC_SRES:
       e = newresolution(sc,*(int *)data);
       break;
    case LPVIIOC_GSIZE:
       *(struct lpvi_pagesize *)data = *vsc->pagesize;
       break;
    case LPVIIOC_GMSGS:
       *(unsigned int *)data = sc->msgs;
       break;
    case LPVIIOC_SMSGS:
       sc->msgs = *(unsigned int *)data;
       break;
    case LPVIIOC_GSETUP:
       *(struct lpvi_pagesetup *)data = sc->csetup;
       break;
    case LPVIIOC_SSETUP:
       e = newpagesetup(sc,(struct lpvi_pagesetup *)data);
       break;
    case LPVIIOC_GFLAGS:
	{ unsigned int f;
	  f = 0;
	  if (sc->flags & LF_IMG_INV) f |= LPVIF_IMAGE_INV;
	  if (sc->flags & LF_MRG_INV) f |= LPVIF_MARGIN_INV;
	  *(unsigned int *)data = f;
	}
       break;
    case LPVIIOC_SFLAGS:
	{ unsigned int f;
	  f = *(unsigned int *)data;
	  if (f & LPVIF_IMAGE_INV) sc->flags |= LF_IMG_INV; else sc->flags &= ~LF_IMG_INV;
	  if (f & LPVIF_MARGIN_INV) sc->flags |= LF_MRG_INV; else sc->flags &= ~LF_MRG_INV;
	  setinv(sc);
	}
       break;
    case LPVIIOC_SDISP:
	{ unsigned int v;
	  v = *(unsigned int *)data;
	  if (v & LPVID_CUSTOM)
	   { if (! (sc->flags & LF_CDISP))
	      { e = lpvicmd(sc,PCMD_DISP_HOST,DS_NONE);
		if (e) break;
		sc->flags |= LF_CDISP;
	      }
	     e = lpvicmd(sc,0x00,DS_NONE)
		 ?: lpvicmd(sc,(v>>LPVID_A_SHF)&LPVID_DATAMASK,DS_NONE)
		 ?: lpvicmd(sc,0x01,DS_NONE)
		 ?: lpvicmd(sc,(v>>LPVID_B_SHF)&LPVID_DATAMASK,DS_NONE);
	   }
	  else
	   { e = lpvicmd(sc,PCMD_DISP_AUTO,DS_NONE);
	     sc->flags &= ~LF_CDISP;
	   }
	}
       break;
    default:
       flag=flag; p=p; /* shut up "argument unused" errors */
       e = ENOTTY;
       break;
  }
 lpvi_unlock(vsc);
 return(e);
}

/*
 * We don't support poll()/select().
 */
int lpvipoll(dev_t dev, int events, struct proc *p)
{
 struct lpvi_softc *sc;

 sc = lookup_unit(minor(dev));
 if (sc == 0) panic("polling nonexistent lpvi");
 if (! (sc->flags & LF_OPEN)) panic("polling unopened lpvi");
 if (sc->msgs & LPVIMSG_EPNT) printf("lpvipoll\n");
 p=p; return(events);
}

/*
 * Autoconfig glue.
 *
 * We blindly assume we are attached "at sbus".
 */

static int lpvi_match(struct device *parent, struct cfdata *cf, void *aux)
{
 struct sbus_attach_args *sa;

 sa = aux;
 if (strcmp(sa->sa_name,"SUNW,lpvi")) return(0);
 return(1);
}

/*
 * The major thing of note here is that, as described above (on the
 *  pbuf member of struct lpvi_softc), we allocate a page buffer at
 *  attach time.  We arguably should defer this to open() time, but on
 *  a machine where the DMA view of memory is a physical view (are
 *  there any such that could have an lpvi? hm, pci-sbus bridges?), we
 *  could have trouble getting contiguous memory then.  We also
 *  allocate the dmamap for DMA at attach time, though we could in
 *  theory defer that too, perhaps even as far as write() time.
 */
static void lpvi_attach(struct device *parent, struct device *self, void *aux)
{
 struct sbus_attach_args *sa;
 struct lpvi_softc *sc;
 bus_space_handle_t bh;
 int err;
 int irev;

 sa = aux;
 sc = (void *) self;
 /* Map device registers */
 err = sbus_bus_map(sa->sa_bustag,sa->sa_slot,sa->sa_offset,sa->sa_size,
			BUS_SPACE_MAP_LINEAR,0,&bh);
 if (err)
  { printf(": cannot map registers (err %d)\n",err);
    return;
  }
 sc->regs = (void *) bh;
 sc->dmatag = sa->sa_dmatag;
 sc->flags = 0;
 /* Reset the hardware and get the interface rev */
 lpvireset(sc,0);
 irev = (sc->regs->csr >> CSR_IREV_SHIFT) & (CSR_IREV >> CSR_IREV_SHIFT);
 printf(": interface rev %d\n",irev);
 /* Set up the bus-dma goop and allocate the page buffer. */
 err = bus_dmamap_create(sc->dmatag,((MAX_W+7)>>3)*MAX_H,1,
			((MAX_W+7)>>3)*MAX_H,0,BUS_DMA_NOWAIT,&sc->dmamap);
 if (err)
  { printf("%s: can't create DMA map (err %d)\n",&sc->dev.dv_xname[0],err);
    return;
  }
 /* See lpviwrite() for why the +16 and +1. */
 sc->pbuf = malloc((((MAX_W+7)>>3)*MAX_H)+16+1,M_DEVBUF,M_NOWAIT);
 if (sc->pbuf == 0)
  { printf("%s: can't allocate page buffer (err %d)\n",&sc->dev.dv_xname[0],err);
    bus_dmamap_destroy(sc->dmatag,sc->dmamap);
    return;
  }
 /* Cool, let's go for it. */
 sbus_establish(&sc->sbdev,&sc->dev);
 bus_intr_establish(sa->sa_bustag,sa->sa_pri,0,lpvi_hard,sc);
 add_unit(sc->dev.dv_unit,sc);
 sc->msgs = 0;
 sc->usetup.lp_default = 1;
 sc->csetup.lp_default = 1;
}

/*
 * The autoconfig glue struct.
 */
struct cfattach lpvi_ca
 = { sizeof(struct lpvi_softc), lpvi_match, lpvi_attach };
