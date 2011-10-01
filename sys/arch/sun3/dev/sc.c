#include <sys/types.h> /* XXX */
#include <sys/device.h> /* XXX */
#include <scsi/scsi_all.h> /* XXX */

#include <sys/buf.h>
#include <sys/errno.h>
#include <machine/psl.h>
#include <machine/param.h>
#include <scsi/scsiconf.h>
#include <machine/autoconf.h>

struct sc_regs {
  /* Data register.  Contains the ID of the SCSI "target", or		 */
  /*  controller, for the SELECT phase. Also, leftover odd bytes are	 */
  /*  left here after a read.						 */
  unsigned char data;
  /* The other half of the data register which we never use.		 */
  unsigned char pad1;
  /* Command and status blocks are passed in and out via this.		 */
  unsigned char cmdstat;
  /* The other half of cmdstat - never contains useful information.	 */
  unsigned char pad2;
  /* The SCSI interface control register. */
  unsigned short int ctrl;
#define SC_PARITY_ERROR      0x8000 /* RO: was parity error on scsi bus */
#define SC_BUS_ERROR         0x4000 /* RO: was bus error on scsi bus */
#define SC_ODD_LENGTH        0x2000 /* RO: data register holds odd byte */
#define SC_INTERRUPT_REQUEST 0x1000 /* RO: would like to interrupt */
#define SC_REQUEST           0x0800 /* RO: set by ctlr to start handshake */
#define SC_MESSAGE           0x0400 /* RO: set by ctlr during message phase */
#define SC_COMMAND           0x0200 /* RO: set during cmd/stat/msg phases */
#define SC_INPUT             0x0100 /* RO: set means device set data/cmdstat */
#define SC_PARITY            0x0080 /* Used to test parity-checking hardware */
#define SC_BUSY              0x0040 /* RO: set by ctlr when it's selected */
#define SC_SELECT            0x0020 /* RW: set by host to select a ctlr */
#define SC_RESET             0x0010 /* RW: set by host to reset scsi bus */
#define SC_PARITY_ENABLE     0x0008 /* RW: enable parity checking */
#define SC_WORD_MODE         0x0004 /* RW: send 2 bytes at a time */
#define SC_DMA_ENABLE        0x0002 /* RW: use DMA */
#define SC_INTERRUPT_ENABLE  0x0001 /* RW: interrupt on completion */
  unsigned short pad3;
  /* Target address for DMA.						 */
  unsigned int dma_addr;
  /* Number of bytes for DMA, stored in complemented form; device	 */
  /*  increments to -1.  If this is 0 or 1 after a transfer then there	 */
  /*  was a DMA overrun.						 */
  unsigned short int dma_count;
  unsigned char pad4;
  /* VME interrupt vector.						 */
  unsigned char vector;
  } ;

#define SC_OPENINGS 2 /* ??? */

/* XXX interface botch - first element of this structure must be a
   "struct device" - really should hold a pointer to the softc in the
   struct device, rather than relying on pointer punning */
struct sc_softc {
  struct device dev;
  volatile struct sc_regs *regs;
  struct scsi_link link;
  struct scsi_xfer *cmdq[SC_OPENINGS];
  struct scsi_xfer *curcmd;
  unsigned int cmdhand;
  unsigned char *dataptr;
  int datalen;
  int xferlen;
  unsigned long int dma_addr;
  unsigned long int dma_seg;
  int dma_seglen;
  unsigned char status;
  unsigned char message;
  unsigned int reqsense : 1; /* got a CHECK - device wants mode sense */
  unsigned int dmaip : 1; /* DMA in progress */
  } ;

static char scsi_name[] = "sc";

/* data structures, forward so initializations can name functions */
struct cfdriver sccd;
struct scsi_adapter sc_adapter;
struct scsi_device sc_dev;

/* sc_startcmd calls sc_interrupted calls sc_cmd_done calls sc_startcmd */
static void sc_startcmd(struct sc_softc *);

/*
 * When waiting for something to happen on the scsi bus, we loop, doing
 *  delay(SCSI_WAIT_DELAY) each time.  The Sprite driver says "[t]he
 *  largest wait time is when a controller is being selected.  This
 *  delay is called the Bus Abort delay and is about 250 milliseconds",
 *  but looping 25000 times with a delay of 10 each time turns out to
 *  be too short for some things.
 */
#define SCSI_WAIT_LOOPS_UNBUSY     25000 /* waiting for !BUSY bus */
#define SCSI_WAIT_LOOPS_SELECT     25000 /* selecting a device */
#define SCSI_WAIT_LOOPS_STATSTART  25000 /* waiting for first status byte */
#define SCSI_WAIT_LOOPS_STATBYTE   10000 /* waiting for other status bytes */
#define SCSI_WAIT_LOOPS_MESSAGE    10000 /* waiting for message byte */
#define SCSI_WAIT_LOOPS_COMMAND     1000 /* sending a command byte */
#define SCSI_WAIT_LOOPS_POLL      750000 /* waiting for completion */
#define SCSI_WAIT_LOOPS_CMDEND    750000 /* waiting for COMMAND phase to end */
#define SCSI_WAIT_DELAY 10 /* arg to delay(), each time around */

/* Do a SCSI wait loop. */
#define SCSI_WAIT_WHILE(var,what,cond)\
	for ((var)=SCSI_WAIT_LOOPS_##what;((var)>0)&&(cond);(var)--) \
		delay(SCSI_WAIT_DELAY);
#define SCSI_WAIT_UNTIL(var,what,cond)\
	for ((var)=SCSI_WAIT_LOOPS_##what;((var)>0)&&!(cond);(var)--) \
		delay(SCSI_WAIT_DELAY);
/* Check if a SCSI wait loop timed out. */
#define SCSI_WAIT_TIMEOUT(var,what) ((var)<1)
/* How many times we delay()ed in a loop. */
#define SCSI_WAIT_LOOPCOUNT(var,what) (SCSI_WAIT_LOOPS_##what - (var))

volatile int sc_do_debug = 0;
#define SCDB_DB1   0x0001
#define SCDB_DB2   0x0002
#define SCDB_DB3   0x0004
#define SCDB_DB4   0x0008
#define SCDB_DDB   0x0010
#define SCDB_REG   0x0020
#define SCDB_SENSE 0x0040
#ifdef DDB
volatile int sc_abort_poll = 0;
volatile int sc_abort_pio = 0;
#endif
#define OPT_DMA 1
#define OPT_INT 2
volatile int sc_options = OPT_DMA;

static void sc_reset(struct sc_softc *sc)
{
 sc->regs->ctrl = SC_RESET;
 delay(1000);
 sc->regs->ctrl = 0;
}

static void sc_dma_alloc(struct sc_softc *sc)
{
 int i;
 unsigned long int addr;

 /* Can we do misaligned DMA? */
 if (((unsigned long int)sc->dataptr) & 1) panic("sc_dma_alloc: misaligned\n");
 /* sun3 dvma_mapin() likes to have addr/len page-aligned */
 addr = (unsigned long int) sc->dataptr;
 sc->dma_seglen = sc->datalen;
 if (addr & PGOFSET)
  { sc->dma_seglen += (addr & PGOFSET);
    addr -= (addr & PGOFSET);
  }
 sc->dma_seglen = (sc->dma_seglen + PGOFSET) & ~PGOFSET;
 sc->dma_seg = (unsigned long int) dvma_mapin((void *)addr,sc->dma_seglen);
 sc->dma_addr = sc->dma_seg + ((unsigned long int)sc->dataptr - addr);
#ifdef SCSIDEBUG
 if (sc->link.flags & SDEV_DB4)
  { printf("sc_dma_alloc: dataptr %08lx datalen %x\n",(unsigned long int)sc->dataptr,sc->datalen);
    printf("              addr %08lx dma_seglen %x dma_seg %08lx dma_addr %08lx\n",addr,sc->dma_seglen,sc->dma_seg,sc->dma_addr);
  }
#endif
}

static void sc_dma_free(struct sc_softc *sc)
{
 dvma_mapout((void *)sc->dma_seg,sc->dma_seglen);
 sc->dma_seg = 0;
}

static void sc_cmd_done(struct sc_softc *sc)
{
 struct scsi_xfer *xs;

 xs = sc->curcmd;
 if (sc->dma_seg) sc_dma_free(sc);
 if (xs->error != XS_DRIVER_STUFFUP)
  { switch (sc->status)
     { case 0:
#ifdef SCSIDEBUG
	  if (sc->link.flags & SDEV_DB4)
	   { printf("sc_cmd_done: status 0, reqsense %d\n",sc->reqsense);
	   }
#endif
	  if (sc->reqsense && (sc->link.device == &sc_dev))
	   { xs->error = XS_SENSE;
	   }
	  xs->resid = sc->datalen;
	  break;
       case SCSI_CHECK:
	  if (sc->reqsense)
	   { /* Sense command also asked for sense? */
	     printf("sc: sense stuffup\n");
	     if (sc_do_debug & SCDB_SENSE)
	      { /* try pretending it returned 0 instead */
		xs->error = XS_SENSE;
		xs->resid = sc->datalen;
	      }
	     else
	      { xs->error = XS_DRIVER_STUFFUP;
	      }
	   }
	  else
	   { sc->reqsense = 1;
#ifdef SCSIDEBUG
	     if (sc->link.flags & SDEV_DB4) printf("sc: device requested sense\n");
#endif
	     sc_startcmd(sc); /* redo */
	     return;
	   }
	  break;
       case SCSI_BUSY:
#ifdef SCSIDEBUG
	  if (sc->link.flags & SDEV_DB4) printf("sc_cmd_done: status BUSY\n");
#endif
	  xs->error = XS_BUSY;
	  break;
       default:
	  printf("sc_cmd_done: status %d?\n",sc->status);
	  xs->error = XS_DRIVER_STUFFUP;
	  break;
     }
  }
 xs->flags |= ITSDONE;
 sc->curcmd = 0;
 sc->reqsense = 0;
 if (sc->dmaip) panic("sc_cmd_done: dma still in progress");
#ifdef SCSIDEBUG
 if (sc->link.flags & SDEV_DB4)
  { printf("sc_cmd_done: xs->resid %d\n",(int)xs->resid);
  }
#endif
 scsi_done(xs);
 sc_startcmd(sc);
}

static void sc_scribble(struct sc_softc *sc)
{
#ifdef SCSIDEBUG
 unsigned char *bp;
 int i;

 if (sc->link.flags & SDEV_DB4)
  { bp = (void *) sc->dataptr;
    for (i=0;i<sc->datalen;i++)
     { *bp++ = 0xa0 | (i & 0xf);
     }
  }
#endif
}

static void sc_interrupted(struct sc_softc *sc)
{
 volatile struct sc_regs *regs;
 int i;

 regs = sc->regs;
 if (regs->ctrl & SC_BUS_ERROR) panic("sc: bus error");
 if (sc->dmaip)
  { int resid;
#ifdef SCSIDEBUG
    unsigned int found_addr;
    unsigned short int found_count;
    void *found_dataptr;
    int found_datalen;
    found_addr = regs->dma_addr;
    found_count = regs->dma_count;
    found_dataptr = sc->dataptr;
    found_datalen = sc->datalen;
#endif
    sc->dmaip = 0;
    resid = (signed short int) ~regs->dma_count;
    sc->dataptr += sc->datalen - resid;
    if (regs->ctrl & SC_ODD_LENGTH)
     { if (sc->curcmd->flags & SCSI_DATA_IN)
	{ resid --;
	  *sc->dataptr = regs->data;
	}
       else
	{ resid ++;
	  sc->dataptr --;
	}
     }
    sc->xferlen = sc->datalen - resid;
    sc->datalen = resid;
#ifdef SCSIDEBUG
    if (sc->link.flags & SDEV_DB4)
     { printf("sc_interrupted: find dma_addr %08x dma_count %04x\n",found_addr,(int)found_count);
       printf("                resid %x now dataptr %08lx datalen %x\n",resid,(unsigned long int)sc->dataptr,sc->datalen);
       printf("                find dataptr %08lx datalen %x\n",(unsigned long int)found_dataptr,found_datalen);
       printf("DMA buffer now:");
       if (found_datalen > 64) found_datalen = 64;
       for (i=0;i<found_datalen;i++) printf(" %02x",((unsigned char *)found_dataptr)[i]);
       printf("\n");
     }
#endif
    sc_dma_free(sc);
  }
 SCSI_WAIT_UNTIL(i,STATSTART,regs->ctrl&SC_INTERRUPT_REQUEST);
 if (SCSI_WAIT_TIMEOUT(i,STATSTART))
  { printf("sc_interrupted: timeout waiting for completion (ctrl = 0x%04x)\n",0xffff&(int)regs->ctrl);
    sc->status = 0;
    sc->message = 0;
  }
 else
  { SCSI_WAIT_UNTIL(i,STATBYTE,regs->ctrl&SC_REQUEST);
    if (SCSI_WAIT_TIMEOUT(i,STATBYTE))
     { printf("sc_interrupted: can't get status (ctrl = 0x%04x), hope it's zero\n",0xffff&(int)regs->ctrl);
       sc->status = 0;
       sc->message = 0;
     }
    else
     { sc->status = regs->cmdstat;
       while (1)
	{ SCSI_WAIT_UNTIL(i,STATBYTE,regs->ctrl&SC_REQUEST);
	  if (SCSI_WAIT_TIMEOUT(i,STATBYTE))
	   { printf("sc_interrupted: timeout fetching status/message (ctrl = 0x%04x) [status=%d]\n",regs->ctrl,sc->status);
	     sc->message = 0;
	     break;
	   }
	  if (regs->ctrl & SC_MESSAGE)
	   { sc->message = regs->cmdstat;
#ifdef SCSIDEBUG
	     if (sc->link.flags & SDEV_DB4)
	      { printf("sc_interrupted: status=%d message=%d\n",sc->status,sc->message);
	      }
#endif
	     break;
	   }
	  regs->cmdstat;
	}
     }
  }
 sc_cmd_done(sc);
}

static void sc_printcmd(struct scsi_xfer *xs, const unsigned char *cmdptr, int cmdlen)
{
#ifdef SCSIDEBUG
 if (xs->sc_link->flags & SDEV_DB4)
  { int i;
    printf("sc cmd [%d:%d]:",xs->sc_link->target,xs->sc_link->lun);
    for (i=0;i<cmdlen;i++) printf(" %02x",cmdptr[i]);
    printf("\n");
  }
#endif
}

static void sc_pio_in(struct sc_softc *sc)
{
 volatile struct sc_regs *regs;
 unsigned int overrun;
 unsigned int nread;
 unsigned char d;
 int i;
 int loops;
 unsigned short int c;

 regs = sc->regs;
 i = 0;
 overrun = 0;
 nread = 0;
 while (1)
  { loops = 0;
#ifdef DDB
    sc_abort_pio = 0;
#endif
    while (1)
     { c = regs->ctrl;
       if (c & SC_COMMAND) goto out;
       if (c & SC_REQUEST) break;
#ifdef SCSIDEBUG
       if (sc->link.flags & SDEV_DB4) printf("sc_pio_in: ctrl %04x\n",(unsigned int)c);
#endif
       if (loops++ == 100000)
	{ printf("PIO-IN taking a long time"
#ifdef DDB
		", ddb sc_abort_pio nonzero to abort request"
#endif
		"\n");
	}
       if (sc_abort_pio) goto out;
     }
    d = regs->data;
#ifdef SCSIDEBUG
    if (sc->link.flags & SDEV_DB4) printf("sc_pio_in: got data %02x\n",(int)d);
#endif
    if (i < sc->datalen)
     { sc->dataptr[i++] = d;
     }
    else
     { overrun ++;
     }
  }
out:;
 sc->datalen -= i;
 sc->dataptr += i;
 sc->xferlen = i;
#ifdef SCSIDEBUG
 if (sc->link.flags & SDEV_DB4) printf("sc_pio_in: done, ctrl %04x overrun %u\n",(unsigned int)c,overrun);
#endif
}

static void sc_pio_out(struct sc_softc *sc)
{
 volatile struct sc_regs *regs;
 unsigned int overrun;
 unsigned int nwrite;
 unsigned char d;
 int i;
 int loops;
 unsigned short int c;

 regs = sc->regs;
 i = 0;
 overrun = 0;
 nwrite = 0;
 while (1)
  { loops = 0;
#ifdef DDB
    sc_abort_pio = 0;
#endif
    while (1)
     { c = regs->ctrl;
       if (c & SC_COMMAND) goto out;
       if (c & SC_REQUEST) break;
#ifdef SCSIDEBUG
       if (sc->link.flags & SDEV_DB4) printf("sc_pio_out: ctrl %04x\n",(unsigned int)c);
#endif
       if (loops++ == 100000)
	{ printf("PIO-OUT taking a long time"
#ifdef DDB
		", ddb sc_abort_pio nonzero to abort request"
#endif
		"\n");
	}
       if (sc_abort_pio) goto out;
     }
    if (i < sc->datalen)
     { d = sc->dataptr[i++];
     }
    else
     { d = 0;
       overrun ++;
     }
    regs->data = d;
#ifdef SCSIDEBUG
    if (sc->link.flags & SDEV_DB4) printf("sc_pio_out: put data %02x\n",(int)d);
#endif
  }
out:;
 sc->datalen -= i;
 sc->dataptr += i;
 sc->xferlen = i;
#ifdef SCSIDEBUG
 if (sc->link.flags & SDEV_DB4) printf("sc_pio_out: done, ctrl %04x overrun %u\n",(unsigned int)c,overrun);
#endif
}

static void sc_startcmd(struct sc_softc *sc)
{
 struct scsi_xfer *xs;
 int i;
 volatile struct sc_regs *regs;
 unsigned char *cmdptr;
 int cmdlen;
 unsigned int loopcnt[10];

#ifdef DIAGNOSTIC
 if ((getsr() & PSL_IPL) < PSL_IPL2) panic("sc_startcmd: bad pl");
#endif
#if defined(SCSIDEBUG) && defined(DDB)
 if (sc->link.flags & SDEV_DB4) Debugger();
#endif
 if (sc->reqsense)
  { xs = sc->curcmd;
  }
 else
  { unsigned int h;
    h = sc->cmdhand;
    for (i=0;i<SC_OPENINGS;i++)
     { h = (h + 1) % SC_OPENINGS;
       if (sc->cmdq[h]) break;
     }
    if (i >= SC_OPENINGS) return;
    sc->cmdhand = h;
    xs = sc->cmdq[h];
    sc->cmdq[h] = 0;
    sc->curcmd = xs;
    if (xs->flags & SCSI_RESET) sc_reset(sc);
  }
 if (sc->reqsense)
  { static struct scsi_sense senserq;
    bzero(&senserq,sizeof(senserq));
    senserq.opcode = REQUEST_SENSE;
    senserq.byte2 = xs->sc_link->lun << 5;
    senserq.length = sizeof(xs->sense);
    cmdptr = (void *) &senserq;
    cmdlen = sizeof(senserq);
    sc->dataptr = (void *) &xs->sense;
    sc->datalen = sizeof(xs->sense);
  }
 else
  { cmdptr = (void *) xs->cmd;
    cmdlen = xs->cmdlen;
    sc->dataptr = xs->data;
    sc->datalen = xs->datalen;
  }
 sc_printcmd(xs,cmdptr,cmdlen);
 regs = sc->regs;
 while (1)
  { SCSI_WAIT_WHILE(i,UNBUSY,regs->ctrl&SC_BUSY);
    if (SCSI_WAIT_TIMEOUT(i,UNBUSY))
     { printf("sc: SCSI bus stuck busy, resetting...\n");
       sc_reset(sc);
       continue;
     }
    break;
  }
 /*
  * Comment taken from the Sprite driver:
  * Select the device.  Sun's SCSI Programmer's Manual recommends
  *  resetting the SCSI_WORD_MODE bit so that the byte packing hardware
  *  is reset and the data byte that has the target ID gets transfered
  *  correctly.  After this, the target's ID is put in the data
  *  register, the SELECT bit is set, and we wait until the device
  *  responds by setting the BUSY bit.  The ID bit of the host adaptor
  *  is not put in the data word because of problems with Sun's Host
  *  Adaptor.
  */
 regs->ctrl = 0;
 regs->data = 1 << xs->sc_link->target;
 regs->ctrl = SC_SELECT;
 SCSI_WAIT_UNTIL(i,SELECT,regs->ctrl&SC_BUSY);
 if (SCSI_WAIT_TIMEOUT(i,SELECT))
  { regs->data = 0;
    regs->ctrl = 0;
#ifdef SCSIDEBUG
    if (sc->link.flags & SDEV_DB4)
     { printf("sc: select %d failed, ctrl=%04x data=%02x\n",xs->sc_link->target,regs->ctrl,regs->data);
     }
#endif
    xs->error = XS_DRIVER_STUFFUP;
    sc_cmd_done(sc);
    return;
  }
 if (0)
  {
punt_request:;
    printf("sc punting request:");
    for (i=0;i<cmdlen;i++) printf(" %02x",0xff&(int)cmdptr[i]);
    printf("\n");
    xs->error = XS_DRIVER_STUFFUP;
    sc_cmd_done(sc);
    return;
  }
 /*
  * Comment taken from the Sprite driver:
  * Set up the interface's registers for the transfer.  The DMA address
  *  is relative to the multibus memory so the kernel's base address
  *  for multibus memory is subtracted from 'addr'.  The host adaptor
  *  increments the dmaCount register until it reaches -1, hence the
  *  funny initialization.  See page 4 of Sun's SCSI Prog. Manual.
  * Of course, we compute the DMA address differently.
  * NB be sure to set up DMA if reqsense, even if main transfer doesn't!
  */
 if ( (sc_options & OPT_DMA) &&
      ( sc->reqsense ||
	(xs->flags & (SCSI_DATA_IN|SCSI_DATA_OUT)) ) )
  { if (! (xs->flags & SCSI_DATA_OUT)) sc_scribble(sc);
    sc_dma_alloc(sc);
    if (! sc->dma_seg) panic("sc: can't alloc dma for %x/%x\n",(unsigned int)sc->dataptr,sc->datalen);
    regs->dma_addr = dvma_kvtopa(sc->dma_addr,BUS_VME16);
    regs->dma_count = ~ (unsigned short int) sc->datalen;
#ifdef SCSIDEBUG
    if (sc->link.flags & SDEV_DB4)
     { printf("sc_startcmd: set dma_addr %08x dma_count %04x\n",regs->dma_addr,(int)regs->dma_count);
     }
#endif
    sc->dmaip = 1;
     { unsigned short int c;
       c = SC_WORD_MODE
	   | SC_DMA_ENABLE
	   | (   (xs->flags & SCSI_POLL)
	       ? 0
	       : (sc_options & OPT_INT)
	       ? SC_INTERRUPT_ENABLE
	       : 0 );
       if (sc_do_debug & SCDB_REG) printf("[%04x->",c);
       regs->ctrl = c;
       if (sc_do_debug & SCDB_REG) printf("%04x]\n",regs->ctrl);
     }
  }
 else
  { regs->dma_addr = 0;
    regs->dma_count = (__typeof__(regs->dma_count)) ~0U; /* XXX cast should be unnecessary */
     { unsigned short int c;
       c =   (xs->flags & SCSI_POLL)
	   ? 0
	   : (sc_options & OPT_INT)
	   ? SC_INTERRUPT_ENABLE
	   : 0;
       if (sc_do_debug & SCDB_REG) printf("[%04x->",c);
       regs->ctrl = c;
       if (sc_do_debug & SCDB_REG) printf("%04x]\n",regs->ctrl);
     }
  }
 /*
  * Comment taken from the Sprite driver:
  * Stuff the control block through the commandStatus register.  The
  *  handshake on the SCSI bus is visible here: we have to wait for the
  *  Request line on the SCSI bus to be raised before we can send the
  *  next command byte to the controller.  All commands are of "group
  *  0" which means they are 6 bytes long.
  * For us, the register name is wrong, and the last sentence is false.
  */
 if (cmdlen > sizeof(loopcnt)/sizeof(loopcnt[0])) panic("sc: loopcnt[] too small");
 for (i=0;i<cmdlen;i++)
  { unsigned int loops;
    SCSI_WAIT_UNTIL(loops,COMMAND,regs->ctrl&SC_REQUEST);
    if (SCSI_WAIT_TIMEOUT(loops,COMMAND))
     { sc_reset(sc);
       printf("sc: command transfer timed out\n");
       goto punt_request;
     }
    loopcnt[i] = SCSI_WAIT_LOOPCOUNT(loops,COMMAND);
    /*
     * Comment taken from the Sprite driver:
     * The device keeps the Control/Data line set while it is accepting
     *  control block bytes.
     */
    if ((regs->ctrl & SC_COMMAND) == 0)
     { sc_reset(sc);
       printf("sc: short command transfer (len %d, did %d)\n",cmdlen,i);
       goto punt_request;
     }
    regs->cmdstat = cmdptr[i];
  }
#ifdef SCSIDEBUG
 if (sc->link.flags & SDEV_DB4)
  { printf("sc_startcmd: loopcnts");
    for (i=0;i<cmdlen;i++) printf(" %d",loopcnt[i]);
    printf(", cmd");
    for (i=0;i<cmdlen;i++) printf(" %02x",0xff&(int)cmdptr[i]);
    printf("\n");
  }
#endif
 SCSI_WAIT_WHILE(i,CMDEND,regs->ctrl&SC_COMMAND);
 if (SCSI_WAIT_TIMEOUT(i,CMDEND))
  { sc_reset(sc);
    printf("sc: timed out waiting for COMMAND phase to end");
    goto punt_request;
  }
 if (sc_options & OPT_DMA)
  { if ((xs->flags & SCSI_POLL) || !(sc_options & OPT_INT))
     { SCSI_WAIT_UNTIL(i,POLL,regs->ctrl&SC_INTERRUPT_REQUEST);
       if (SCSI_WAIT_TIMEOUT(i,POLL))
	{ printf("sc: POLL request taking a long time"
#ifdef DDB
		 ", ddb sc_abort_poll nonzero to stop waiting"
#endif
		 "\n");
#ifdef DDB
	  sc_abort_poll = 0;
	  while (!(regs->ctrl&SC_INTERRUPT_REQUEST) && !sc_abort_poll) ;
#else
	  while (!(regs->ctrl&SC_INTERRUPT_REQUEST)) ;
#endif
	}
       sc_interrupted(sc);
     }
  }
 else
  { if (sc->reqsense || (xs->flags & SCSI_DATA_IN))
     { sc_pio_in(sc);
     }
    else if (xs->flags & SCSI_DATA_OUT)
     { sc_pio_out(sc);
     }
    sc_interrupted(sc);
  }
}

static int sc_scsi_cmd(struct scsi_xfer *xs)
{
 int f;
 int s;
 struct sc_softc *sc;
 int i;

 f = xs->flags;
 if (xs->bp) f |= SCSI_NOSLEEP;
 if (f & ITSDONE) panic("sc_scsi_cmd: already done");
 if (! (f & INUSE)) panic("sc_scsi_cmd: not in use");
 if (f & SCSI_DATA_UIO) panic("sc_scsi_cmd: data uio requested");
 s = splbio();
 sc = xs->sc_link->adapter_softc;
 if (sc->curcmd && (xs->flags & SCSI_POLL)) panic("sc_scsi_cmd: poll while busy");
 for (i=0;(i<SC_OPENINGS)&&sc->cmdq[i];i++) ;
 if (i >= SC_OPENINGS)
  { splx(s);
    return(TRY_AGAIN_LATER);
  }
 sc->cmdq[i] = xs;
 if (! sc->curcmd) sc_startcmd(sc);
 splx(s);
 s = splbio();
 if (xs->flags & ITSDONE)
  { splx(s);
    return(COMPLETE);
  }
 else if (xs->flags & SCSI_POLL)
  { panic("sc_scsi_cmd: poll didn't finish");
  }
 else
  { return(SUCCESSFULLY_QUEUED);
  }
}

static int sc_print(void *aux, char *name)
{
 if (name) printf("%s: scsibus ",name);
 return(UNCONF);
}

static int sc_intr(void *arg)
{
 struct sc_softc *sc;
 int resid;

 sc = arg;
 if (! (sc->regs->ctrl & SC_INTERRUPT_REQUEST))
  { printf("sc_intr: spurious\n");
    return(0);
  }
 sc_interrupted(sc);
 return(1);
}

#define SC_MINPHYS 65536 /* XXX */
static void sc_minphys(struct buf *b)
{
 if (b->b_bcount > SC_MINPHYS)
  { printf("sc_minphys setting b->b_count = %x\n",SC_MINPHYS);
    b->b_bcount = SC_MINPHYS;
  }
 minphys(b);
}

static int sc_match(struct device *parent, void *vcf, void *args)
{
 struct cfdata *cf;
 struct confargs *ca;
 int x;

 cf = vcf;
 ca = args;
 if (ca->ca_bustype != BUS_VME16) return(0);
 if (ca->ca_paddr == -1) return(0);
 if (ca->ca_intpri == -1) ca->ca_intpri = 2;
 x = bus_peek(ca->ca_bustype,ca->ca_paddr+1,1);
 if (x == -1) return(0);
 /* don't confuse us with an si! */
 x = bus_peek(ca->ca_bustype,ca->ca_paddr+0x801,1);
 if (x == -1) return(0);
 return(1);
}

static void sc_attach(struct device *parent, struct device *self, void *args)
{
 struct sc_softc *sc;
 volatile struct sc_regs *regs;
 struct confargs *ca;
 int i;

 printf(" (options=%d)\n",sc_options);
 ca = args;
 sc = (struct sc_softc *) self;
 switch (ca->ca_bustype)
  { case BUS_VME16:
       /* XXX bus_mapin() should return void *, cast should be unneded */
       regs = (struct sc_regs *) bus_mapin(ca->ca_bustype,ca->ca_paddr,sizeof(*regs));
       isr_add_vectored(sc_intr,sc,ca->ca_intpri,ca->ca_intvec);
       break;
    default:
       panic("sc on non-VME16");
       break;
  }
 sc->regs = regs;
 sc->link.adapter_softc = sc;
 sc->link.adapter_target = 7;
 sc->link.adapter = &sc_adapter;
 sc->link.device = &sc_dev;
 sc->link.openings = SC_OPENINGS;
#if defined(SCSIDEBUG) && defined(DDB)
 if (sc_do_debug & SCDB_DDB) Debugger();
 if (sc_do_debug & SCDB_DB1) sc->link.flags |= SDEV_DB1;
 if (sc_do_debug & SCDB_DB2) sc->link.flags |= SDEV_DB2;
 if (sc_do_debug & SCDB_DB3) sc->link.flags |= SDEV_DB3;
 if (sc_do_debug & SCDB_DB4) sc->link.flags |= SDEV_DB4;
#endif
 sc_reset(sc);
 regs->vector = ca->ca_intvec;
 for (i=0;i<SC_OPENINGS;i++) sc->cmdq[i] = 0;
 sc->curcmd = 0;
 sc->cmdhand = 0;
 sc->reqsense = 0;
 sc->dmaip = 0;
 config_found_sm(self,&sc->link,sc_print,0);
}

struct cfattach sc_ca = { sizeof(struct sc_softc), sc_match, sc_attach };
struct cfdriver sc_cd = { 0, "sc", DV_DULL };
struct scsi_adapter sc_adapter = { sc_scsi_cmd, sc_minphys, 0, 0 };
struct scsi_device sc_dev = { 0, 0, 0, 0 };

#if 0

/*
 * devSCSI0.c --
 *
 *	Driver routines specific to the original Sun Host Adaptor.
 *	This lives either on the Multibus or the VME.  It does
 *	not support dis-connect/connect.
 * 	This information is derived from Sun's "Manual for Sun SCSI
 *	Programmers", which details the layout of the this implementation
 *	of the Host Adaptor, the device that interfaces to the SCSI Bus.
 *
 * Copyright 1986 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /cdrom/src/kernel/Cvsroot/kernel/dev/sun3.md/devSCSI0.c,v 9.3 91/08/19 13:47:59 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <mach.h>
#include <scsi0.h>
#include <dev.h>
#include <devInt.h>
#include <sys/scsi.h>
#include <scsiHBA.h>
#include <scsiDevice.h>
#include <devMultibus.h>
#include <sync.h>
#include <stdio.h>
#include <stdlib.h>
#include <bstring.h>
#include <string.h>

/*
 * The device registers for the original Sun SCSI Host Adaptor.
 * Through these registers the SCSI bus is controlled.  There are the
 * usual status and control bits, and there are also registers through
 * which command blocks and status blocks are transmitted.  This format
 * is defined on Page 10. of Sun's SCSI Programmers' Manual.
 */
typedef struct CtrlRegs {
    unsigned char data;		/* Data register.  Contains the ID of the
				 * SCSI "target", or controller, for the
				 * SELECT phase. Also, leftover odd bytes
				 * are left here after a read. */
    unsigned char pad1;		/* The other half of the data register which
				 * is never used by us */
    unsigned char commandStatus;/* Command and status blocks are passed
				 * in and out through this */
    unsigned char pad2;		/* The other half of the commandStatus register
				 * which never contains useful information */
    unsigned short control;	/* The SCSI interface control register.
				 * Bits are defined below */
    unsigned short pad3;
    unsigned int dmaAddress;	/* Target address for DMA */
    short 	dmaCount;	/* Number of bytes for DMA.  Initialize this
				 * this to minus the byte count minus 1 more,
				 * and the device increments to -1. If this
				 * is 0 or 1 after a transfer then there was
				 * a DMA overrun. */
    unsigned char pad4;
    unsigned char intrVector;	/* For VME, Index into autovector */
} CtrlRegs;

/*
 * Control bits in the SCSI Host Interface control register.
 *
 *	SCSI_PARITY_ERROR There was a parity error on the SCSI bus.
 *	SCSI_BUS_ERROR	There was a bus error on the SCSI bus.
 *	SCSI_ODD_LENGTH An odd byte is left over in the data register after
 *			a read or write.
 *	SCSI_INTERRUPT_REQUEST bit checked by polling routine.  If a command
 *			block is sent and the SCSI_INTERRUPT_ENABLE bit is
 *			NOT set, then the appropriate thing to do is to
 *			wait around (poll) until this bit is set.
 *	SCSI_REQUEST	Set by controller to start byte passing handshake.
 *	SCSI_MESSAGE	Set by a controller during message phase.
 *	SCSI_COMMAND	Set during the command, status, and messages phase.
 *	SCSI_INPUT	If set means data (or commandStatus) set by device.
 *	SCSI_PARITY	Used to test the parity checking hardware.
 *	SCSI_BUSY	Set by controller after it has been selected.
 *  The following bits can be set by the CPU.
 *	SCSI_SELECT	Set by the host when it want to select a controller.
 *	SCSI_RESET	Set by the host when it want to reset the SCSI bus.
 *	SCSI_PARITY_ENABLE	Enable parity checking on transfer
 *	SCSI_WORD_MODE		Send 2 bytes at a time
 *	SCSI_DMA_ENABLE		Do DMA, always used.
 *	SCSI_INTERRUPT_ENABLE	Interrupt upon completion.
 */
#define SCSI_PARITY_ERROR		0x8000
#define SCSI_BUS_ERROR			0x4000
#define SCSI_ODD_LENGTH			0x2000
#define SCSI_INTERRUPT_REQUEST		0x1000
#define SCSI_REQUEST			0x0800
#define SCSI_MESSAGE			0x0400
#define SCSI_COMMAND			0x0200
#define SCSI_INPUT			0x0100
#define SCSI_PARITY			0x0080
#define SCSI_BUSY			0x0040
#define SCSI_SELECT			0x0020
#define SCSI_RESET			0x0010
#define SCSI_PARITY_ENABLE		0x0008
#define SCSI_WORD_MODE			0x0004
#define SCSI_DMA_ENABLE			0x0002
#define SCSI_INTERRUPT_ENABLE		0x0001

/* Forward declaration. */
typedef struct Controller Controller;

/*
 * Device - The data structure containing information about a device. One of
 * these structure is kept for each attached device. Note that is structure
 * is casted into a ScsiDevice and returned to higher level software.
 * This implies that the ScsiDevice must be the first field in this
 * structure.
 */

typedef struct Device {
    ScsiDevice handle;	/* Scsi Device handle. This is the only part
			 * of this structure visible to higher
			 * level software. MUST BE FIRST FIELD IN STRUCTURE.
			 */
    int	targetID;	/* SCSI Target ID of this device. Note that
			 * the LUN is store in the device handle.
			 */
    Controller *ctrlPtr;	/* Controller to which device is attached. */
		   /*
		    * The following part of this structure is
		    * used to handle SCSI commands that return
		    * CHECK status. To handle the REQUEST SENSE
		    * command we must: 1) Save the state of the current
		    * command into the "struct FrozenCommand". 2) Submit
		    * a request sense command formatted in SenseCmd
		    * to the device.
		    */
    struct FrozenCommand {
	ScsiCmd	*scsiCmdPtr;	  /* The frozen command. */
	unsigned char statusByte; /* It's SCSI status byte, Will always have
				   * the check bit set.  */
	int amountTransferred;    /* Number of bytes transferred by this
				   * command.  */
    } frozen;
    char senseBuffer[DEV_MAX_SENSE_BYTES]; /* Data buffer for request sense */
    ScsiCmd		SenseCmd;  	   /* Request sense command buffer. */
} Device;

/*
 * Controller - The Data structure describing a sun SCSI0 controller. One
 * of these structures exists for each active SCSI0 HBA on the system. Each
 * controller may have from zero to 56 (7 targets each with 8 logical units)
 * devices attached to it.
 */
struct Controller {
    volatile CtrlRegs *regsPtr;	/* Pointer to the registers
                                    of this controller. */
    int	    dmaState;	/* DMA state for this controller, defined below. */
    char    *name;	/* String for error message for this controller.  */
    DevCtrlQueues devQueues;    /* Device queues for devices attached to this
				 * controller.	 */
    Sync_Semaphore mutex; /* Lock protecting controller's data structures. */
    Device     *devPtr;	   /* Current active command. */
    ScsiCmd   *scsiCmdPtr; /* Current active command. */
    Address   dmaBuffer;  /* dma buffer allocated for request. */
    Device  *devicePtr[8][8]; /* Pointers to the device
                               * attached to the
			       * controller index by [targetID][LUN].
			       * NIL if device not attached yet. Zero if
			       * device conflicts with HBA address.  */

};

/*
 * SCSI_WAIT_LENGTH - the number of microseconds that the host waits for
 *	various control lines to be set on the SCSI bus.  The largest wait
 *	time is when a controller is being selected.  This delay is
 *	called the Bus Abort delay and is about 250 milliseconds.
 */
#define SCSI_WAIT_LENGTH		250000

/*
 * Possible values for the dmaState state field of a controller.
 *
 * DMA_RECEIVE  - data is being received from the device, such as on
 *	a read, inquiry, or request sense.
 * DMA_SEND     - data is being send to the device, such as on a write.
 * DMA_INACTIVE - no data needs to be transferred.
 */

#define DMA_RECEIVE 0
#define	DMA_SEND 1
#define	DMA_INACTIVE 2
/*
 * Test, mark, and unmark the controller as busy.
 */
#define	IS_CTRL_BUSY(ctrlPtr)	((ctrlPtr)->scsiCmdPtr != (ScsiCmd *) NIL)
#define	SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr) \
		((ctrlPtr)->scsiCmdPtr = (scsiCmdPtr))
#define	SET_CTRL_FREE(ctrlPtr)	((ctrlPtr)->scsiCmdPtr = (ScsiCmd *) NIL)

/*
 * MAX_SCSI0_CTRLS - Maximum number of SCSI0 controllers attached to the
 *		     system. We set this to the maximum number of VME slots
 *		     in any Sun2 system currently available.
 */
#define	MAX_SCSI0_CTRLS	4
static Controller *Controllers[MAX_SCSI0_CTRLS];
/*
 * Highest number controller we have probed for.
 */
static int numSCSI0Controllers = 0;

int devSCSI0Debug = 0;

static void RequestDone _ARGS_ ((Device *devPtr, ScsiCmd *scsiCmdPtr,
    ReturnStatus status, unsigned int scsiStatusByte, int amountTransferred));
static ReturnStatus Wait _ARGS_ ((Controller *ctrlPtr,
    int condition, Boolean reset));
static int              SpecialSenseProc _ARGS_((ScsiCmd *scsiCmdPtr,
					    ReturnStatus status,
					    int statusByte,
					    int byteCount,
					    int senseLength,
					    Address senseDataPtr));


/*
 *----------------------------------------------------------------------
 *
 * Probe --
 *
 *	Probe memory for the old-style VME SCSI interface.  We rely
 *	on the fact that this occupies 4K of address space.
 *
 * Results:
 *	TRUE if the host adaptor was found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static Boolean
Probe(address)
    int address;			/* Alledged controller address */
{
    ReturnStatus	status;
    register volatile CtrlRegs *regsPtr = (CtrlRegs *)address;
    short value;

    /*
     * Touch the device. If it exists it occupies 4K.
     */
    value = 0x4BCC;
    status = Mach_Probe(sizeof(regsPtr->dmaCount), (char *) &value,
			(char *) &(regsPtr->dmaCount));
    if (status == SUCCESS) {
	value = 0x5BCC;
	status = Mach_Probe(sizeof(regsPtr->dmaCount),(char *) &value,
			  ((char *) &(regsPtr->dmaCount)) + 0x800);
    }
    return(status == SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Reset --
 *
 *	Reset a SCSI bus controlled by the orignial Sun Host Adaptor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reset the controller.
 *
 *----------------------------------------------------------------------
 */
static void
Reset(ctrlPtr)
    Controller *ctrlPtr;
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;

    regsPtr->control = SCSI_RESET;
    MACH_DELAY(100);
    regsPtr->control = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * SendCommand --
 *
 *      Send a command to a controller on the old-style SCSI Host Adaptor
 *      indicated by devPtr.
 *
 *	Note: the ID of the controller is never placed on the bus
 *	(contrary to standard protocol, but necessary for the early Sun
 *	SCSI interface).
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	Those of the command (Read, write etc.)
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SendCommand(devPtr, scsiCmdPtr)
    Device	*devPtr;		/* Device to sent to. */
    ScsiCmd	*scsiCmdPtr;		/* Command to send. */
{
    register ReturnStatus status;
    register volatile CtrlRegs *regsPtr;/* Host Adaptor registers */
    char *charPtr;			/* Used to put the control block
					 * into the commandStatus register */
    int bits = 0;			/* variable bits to OR into control */
    int targetID;			/* Id of the SCSI device to select */
    int size;				/* Number of bytes to transfer */
    Address addr;			/* Kernel address of transfer */
    Controller	*ctrlPtr;		/* HBA of device. */
    int	i;

    /*
     * Set current active device and command for this controller.
     */
    ctrlPtr = devPtr->ctrlPtr;
    SET_CTRL_BUSY(ctrlPtr,scsiCmdPtr);
    ctrlPtr->dmaBuffer = (Address) NIL;
    ctrlPtr->devPtr = devPtr;
    size = scsiCmdPtr->bufferLen;
    addr = scsiCmdPtr->buffer;
    targetID = devPtr->targetID;
    regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    if (size == 0) {
	ctrlPtr->dmaState = DMA_INACTIVE;
    } else {
	ctrlPtr->dmaState = (scsiCmdPtr->dataToDevice) ? DMA_SEND :
							 DMA_RECEIVE;
    }
   /*
     * Check against a continuously busy bus.  This stupid condition would
     * fool the code below that tries to select a device.
     */
    for (i=0 ; i < SCSI_WAIT_LENGTH ; i++) {
	if ((regsPtr->control & SCSI_BUSY) == 0) {
	    break;
	} else {
	    MACH_DELAY(10);
	}
    }
    if (i == SCSI_WAIT_LENGTH) {
	Reset(ctrlPtr);
	printf("Warning: %s SCSI bus stuck busy\n", ctrlPtr->name);
	return(FAILURE);
    }
    /*
     * Select the device.  Sun's SCSI Programmer's Manual recommends
     * resetting the SCSI_WORD_MODE bit so that the byte packing hardware
     * is reset and the data byte that has the target ID gets transfered
     * correctly.  After this, the target's ID is put in the data register,
     * the SELECT bit is set, and we wait until the device responds
     * by setting the BUSY bit.  The ID bit of the host adaptor is not
     * put in the data word because of problems with Sun's Host Adaptor.
     */
    regsPtr->control = 0;
    regsPtr->data = (1 << targetID);
    regsPtr->control = SCSI_SELECT;
    status = Wait(ctrlPtr, SCSI_BUSY, FALSE);
    if (status != SUCCESS) {
	regsPtr->data = 0;
	regsPtr->control = 0;
        printf("Warning: %s: can't select device at %s\n",
				 ctrlPtr->name, devPtr->handle.locationName);
	return(status);
    }
    /*
     * Set up the interface's registers for the transfer.  The DMA address
     * is relative to the multibus memory so the kernel's base address
     * for multibus memory is subtracted from 'addr'. The host adaptor
     * increments the dmaCount register until it reaches -1, hence the
     * funny initialization. See page 4 of Sun's SCSI Prog. Manual.
     */
    if (ctrlPtr->dmaState != DMA_INACTIVE) {
	ctrlPtr->dmaBuffer = addr = VmMach_DMAAlloc(size,scsiCmdPtr->buffer);
    }
    if (addr == (Address) NIL) {
	panic("%s can't allocate DMA buffer of %d bytes\n",
			devPtr->handle.locationName, size);
    }
    regsPtr->dmaAddress = (int)(addr - VMMACH_DMA_START_ADDR);
    regsPtr->dmaCount = -size - 1;
    bits = SCSI_WORD_MODE | SCSI_DMA_ENABLE | SCSI_INTERRUPT_ENABLE;
    regsPtr->control = bits;

    /*
     * Stuff the control block through the commandStatus register.
     * The handshake on the SCSI bus is visible here:  we have to
     * wait for the Request line on the SCSI bus to be raised before
     * we can send the next command byte to the controller.  All commands
     * are of "group 0" which means they are 6 bytes long.
     */
    charPtr = scsiCmdPtr->commandBlock;
    for (i=0 ; i < scsiCmdPtr->commandBlockLen ; i++) {
	status = Wait(ctrlPtr, SCSI_REQUEST, TRUE);
	if (status != SUCCESS) {
	    printf("Warning: %s couldn't send command block byte %d\n",
				 ctrlPtr->name, i);
	    return(status);
	}
	/*
	 * The device keeps the Control/Data line set while it
	 * is accepting control block bytes.
	 */
	if ((regsPtr->control & SCSI_COMMAND) == 0) {
	    Reset(ctrlPtr);
	    printf("Warning: %s: device %s dropped command line\n",
				ctrlPtr->name, devPtr->handle.locationName);
	    return(DEV_HANDSHAKE_ERROR);
	}
        regsPtr->commandStatus = *charPtr;
	charPtr++;
    }
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * GetStatusByte --
 *
 *	Complete an SCSI command by getting the status bytes from
 *	the device and waiting for the ``command complete''
 *	message that follows the status bytes.  If the command has
 *	additional ``sense data'' then this routine issues the
 *	SCSI_REQUEST_SENSE command to get the sense data.
 *
 * Results:
 *	An error code if the status didn't come through or it
 *	indicated an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetStatusByte(ctrlPtr, statusBytePtr)
    Controller *ctrlPtr;
    unsigned char *statusBytePtr;
{
    register ReturnStatus status;
    register volatile CtrlRegs *regsPtr;
    short message;
    char statusByte;
    int numStatusBytes = 0;

    regsPtr = ctrlPtr->regsPtr;
    *statusBytePtr = 0;
    for (;;) {
	/*
	 * Could probably wait either on the INTERUPT_REQUEST bit or the
	 * REQUEST bit.  Reading the byte out of the commandStatus
	 * register acknowledges the REQUEST and clears these bits.  Here
	 * we grab bytes until the MESSAGE bit indicates that all the
	 * status bytes have been received and that the byte in the
	 * commandStatus register is the message byte.
	 */
	status = Wait(ctrlPtr, SCSI_REQUEST, TRUE);
	if (status != SUCCESS) {
	    printf("Warning: %s: wait error after %d status bytes\n",
				 ctrlPtr->name, numStatusBytes);
	    break;
	}
	if (regsPtr->control & SCSI_MESSAGE) {
	    message = regsPtr->commandStatus & 0xff;
	    if (message != SCSI_COMMAND_COMPLETE) {
		printf("Warning %s: Unexpected message 0x%x\n",
				     ctrlPtr->name, message);
	    }
	    break;
	} else {
	    /*
	     * This is another status byte.  Place the first status
	     * bytes into the status block.
	     */
	    statusByte = regsPtr->commandStatus;
	    if (numStatusBytes < 1) {
		*statusBytePtr = statusByte;
	    }
	    numStatusBytes++;
	}
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * Wait --
 *
 *	Wait for a condition in the SCSI controller.
 *
 * Results:
 *	SUCCESS if the condition occurred before a threashold time limit,
 *	DEV_TIMEOUT otherwise.
 *
 * Side effects:
 *	This resets the SCSI bus if the reset parameter is true and
 *	the condition bits are not set by the controller before timeout..
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
Wait(ctrlPtr, condition, reset)
    Controller *ctrlPtr;
    int condition;
    Boolean reset;
{
    volatile CtrlRegs *regsPtr = (volatile CtrlRegs *)ctrlPtr->regsPtr;
    register int i;
    ReturnStatus status = DEV_TIMEOUT;
    register int control = 0;

    for (i=0 ; i < SCSI_WAIT_LENGTH ; i++) {
	control = regsPtr->control;
	if (devSCSI0Debug && i < 5) {
	    printf("%d/%x ", i, control);
	}
	if (control & condition) {
	    return(SUCCESS);
	}
	if (control & SCSI_BUS_ERROR) {
	    printf("Warning: %s : SCSI bus error\n",ctrlPtr->name);
	    status = DEV_DMA_FAULT;
	    break;
	} else if (control & SCSI_PARITY_ERROR) {
	    printf("Warning: %s: parity error\n",ctrlPtr->name);
	    status = DEV_DMA_FAULT;
	    break;
	}
	MACH_DELAY(10);
    }
    if (devSCSI0Debug) {
	printf("DevSCSI0Wait: timed out, control = %x.\n", control);
    }
    if (reset) {
	Reset(ctrlPtr);
    }
    return(status);
}

/*
 *----------------------------------------------------------------------
 *
 * entryAvailProc --
 *
 *	Act upon an entry becomming available in the queue for this
 *	controller. This routine is the Dev_Queue callback function that
 *	is called whenever work becomes available for this controller.
 *	If the controller is not already busy we dequeue and start the
 *	request.
 *	NOTE: This routine is also called from DevSCSI0Intr to start the
 *	next request after the previously one finishes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Request may be dequeue and submitted to the device. Request callback
 *	function may be called.
 *
 *----------------------------------------------------------------------
 */

static Boolean
entryAvailProc(clientData, newRequestPtr)
   ClientData	clientData;	/* Really the Device this request ready. */
   List_Links *newRequestPtr;	/* The new SCSI request. */
{
    register Device *devPtr;
    register Controller *ctrlPtr;
    register ScsiCmd	*scsiCmdPtr;
    ReturnStatus	status;

    devPtr = (Device *) clientData;
    ctrlPtr = devPtr->ctrlPtr;
    /*
     * If we are busy (have an active request) just return. Otherwise
     * start the request.
     */

    if (IS_CTRL_BUSY(ctrlPtr)) {
	return FALSE;
    }
again:
    scsiCmdPtr = (ScsiCmd *) newRequestPtr;
    devPtr = (Device *) clientData;
    status = SendCommand( devPtr, scsiCmdPtr);
    /*
     * If the command couldn't be started do the callback function.
     */
    if (status != SUCCESS) {
	 RequestDone(devPtr,scsiCmdPtr,status,0,0);
    }
    if (!IS_CTRL_BUSY(ctrlPtr)) {
        newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
	if (newRequestPtr != (List_Links *) NIL) {
	    goto again;
	}
    }
    return TRUE;

}


/*
 *----------------------------------------------------------------------
 *
 *  SpecialSenseProc --
 *
 *	Special function used for HBA generated REQUEST SENSE. A SCSI
 *	command request with this function as a call back proc will
 *	be processed by routine RequestDone as a result of a
 *	REQUEST SENSE. This routine is never called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SpecialSenseProc(scsiCmdPtr, status, statusByte, byteCount, senseLength,
		 senseDataPtr)
    ScsiCmd		*scsiCmdPtr;
    ReturnStatus	status;
    unsigned char	statusByte;
    int			byteCount;
    int			senseLength;
    Address		senseDataPtr;
{
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * RequestDone --
 *
 *	Process a request that has finished. Unless a SCSI check condition
 *	bit is present in the status returned, the request call back
 *	function is called.  If check condition is set we fire off a
 *	SCSI REQUEST SENSE to get the error sense bytes from the device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The call back function may be called.
 *
 *----------------------------------------------------------------------
 */

static void
RequestDone(devPtr,scsiCmdPtr,status,scsiStatusByte,amountTransferred)
    Device	*devPtr;	/* Device for request. */
    ScsiCmd	*scsiCmdPtr;	/* Request that finished. */
    ReturnStatus status;	/* Status returned. */
    unsigned int scsiStatusByte;	/* SCSI Status Byte. */
    int		amountTransferred; /* Amount transferred by command. */
{
    ReturnStatus	senseStatus;
    Controller	        *ctrlPtr = devPtr->ctrlPtr;


    if (devSCSI0Debug > 3) {
	printf("RequestDone for %s status 0x%x scsistatus 0x%x count %d\n",
	    devPtr->handle.locationName, status,scsiStatusByte,
	    amountTransferred);
    }
    if (ctrlPtr->dmaState != DMA_INACTIVE) {
	VmMach_DMAFree(scsiCmdPtr->bufferLen,ctrlPtr->dmaBuffer);
	ctrlPtr->dmaState = DMA_INACTIVE;
    }
    /*
     * First check to see if this is the reponse of a HBA generated
     * REQUEST SENSE command.  If this is the case, we can process
     * the callback of the frozen command for this device and
     * allow the flow of command to the device to be resummed.
     */
    if (scsiCmdPtr->doneProc == SpecialSenseProc) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(devPtr->frozen.scsiCmdPtr->doneProc)(devPtr->frozen.scsiCmdPtr,
			SUCCESS,
			devPtr->frozen.statusByte,
			devPtr->frozen.amountTransferred,
			amountTransferred,
			devPtr->senseBuffer);
	 MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_CTRL_FREE(ctrlPtr);
	 return;
    }
    /*
     * This must be a outside request finishing. If the request
     * suffered an error or the HBA or the scsi status byte
     * says there is no error sense present, we can do the
     * callback and free the controller.
     */
    if ((status != SUCCESS) || !SCSI_CHECK_STATUS(scsiStatusByte)) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	 MASTER_LOCK(&(ctrlPtr->mutex));
	 SET_CTRL_FREE(ctrlPtr);
	 return;
   }
   /*
    * If we got here than the SCSI command came back from the device
    * with the CHECK bit set in the status byte.
    * Need to perform a REQUEST SENSE. Move the current request
    * into the frozen state and issue a REQUEST SENSE.
    */
   devPtr->frozen.scsiCmdPtr = scsiCmdPtr;
   devPtr->frozen.statusByte = scsiStatusByte;
   devPtr->frozen.amountTransferred = amountTransferred;
   DevScsiSenseCmd((ScsiDevice *)devPtr, DEV_MAX_SENSE_BYTES,
		   devPtr->senseBuffer, &(devPtr->SenseCmd));
   devPtr->SenseCmd.doneProc = SpecialSenseProc,
   senseStatus = SendCommand(devPtr, &(devPtr->SenseCmd));
   /*
    * If we got an HBA error on the REQUEST SENSE we end the outside
    * command with the SUCCESS status but zero sense bytes returned.
    */
   if (senseStatus != SUCCESS) {
        MASTER_UNLOCK(&(ctrlPtr->mutex));
	(scsiCmdPtr->doneProc)(scsiCmdPtr, status, scsiStatusByte,
				   amountTransferred, 0, (char *) 0);
	MASTER_LOCK(&(ctrlPtr->mutex));
	SET_CTRL_FREE(ctrlPtr);
   }

}

/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Intr --
 *
 *	Handle interrupts from the SCSI controller.  This has to poll
 *	through the possible SCSI controllers to find the one generating
 *	the interrupt.  The usual action is to wake up whoever is waiting
 *	for I/O to complete.  This may also start up another transaction
 *	with the controller if there are things in its queue.
 *
 * Results:
 *	TRUE if the SCSI controller was responsible for the interrupt
 *	and this routine handled it.
 *
 * Side effects:
 *	Usually a process is notified that an I/O has completed.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
Boolean
DevSCSI0Intr(clientDataArg)
    ClientData	clientDataArg;
{
    register Controller *ctrlPtr;
    List_Links *newRequestPtr;
    Device	*devPtr;
    volatile CtrlRegs *regsPtr;
    int		residual;
    ReturnStatus	status;
    unsigned char statusByte;
    ClientData	clientData;

    ctrlPtr = (Controller *) clientDataArg;
    regsPtr = ctrlPtr->regsPtr;
    devPtr = ctrlPtr->devPtr;
    MASTER_LOCK(&(ctrlPtr->mutex));
    if (regsPtr->control & SCSI_INTERRUPT_REQUEST) {
	if (regsPtr->control & SCSI_BUS_ERROR) {
	    if (regsPtr->dmaCount >= 0) {
		/*
		 * A DMA overrun.  Unlikely with a disk but could
		 * happen while reading a large tape block.  Consider
		 * the I/O complete with no residual bytes
		 * un-transferred.
		 */
		residual = 0;
	    } else {
		/*
		 * A real Bus Error.  Complete the I/O but flag an error.
		 * The residual is computed because the Bus Error could
		 * have occurred after a number of sectors.
		 */
		residual = -regsPtr->dmaCount -1;
	    }
	    /*
	     * The board needs to be reset to clear the Bus Error
	     * condition so no status bytes are grabbed.
	     */
	    Reset(ctrlPtr);
	    status = DEV_DMA_FAULT;
	    RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status, 0,
			ctrlPtr->scsiCmdPtr->bufferLen - residual);
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
		if (newRequestPtr != (List_Links *) NIL) {
		    (void) entryAvailProc(clientData,newRequestPtr);
		}
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	} else {
	    /*
	     * Normal command completion.  Compute the residual,
	     * the number of bytes not transferred, check for
	     * odd transfer sizes, and finally get the completion
	     * status from the device.
	     */
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		printf("Warning: Spurious interrupt from SCSI0\n");
		Reset(ctrlPtr);
		MASTER_UNLOCK(&(ctrlPtr->mutex));
		return(TRUE);
	    }
	    residual = -regsPtr->dmaCount -1;
	    if (regsPtr->control & SCSI_ODD_LENGTH) {
		/*
		 * On a read the last odd byte is left in the data
		 * register.  On both reads and writes the number
		 * of bytes transferred as determined from dmaCount
		 * is off by one.  See Page 8 of Sun's SCSI
		 * Programmers' Manual.
		 */
		if (!ctrlPtr->scsiCmdPtr->dataToDevice) {
		  *(volatile char *)(DEV_MULTIBUS_BASE + regsPtr->dmaAddress) =
			regsPtr->data;
		    residual--;
		} else {
		    residual++;
		}
	    }
	    status = GetStatusByte(ctrlPtr,&statusByte);
	    RequestDone(devPtr, ctrlPtr->scsiCmdPtr, status,
			statusByte,
			ctrlPtr->scsiCmdPtr->bufferLen - residual);
	    if (!IS_CTRL_BUSY(ctrlPtr)) {
		newRequestPtr = Dev_QueueGetNextFromSet(ctrlPtr->devQueues,
				DEV_QUEUE_ANY_QUEUE_MASK,&clientData);
		if (newRequestPtr != (List_Links *) NIL) {
		    (void) entryAvailProc(clientData,newRequestPtr);
		}
	    }
	    MASTER_UNLOCK(&(ctrlPtr->mutex));
	    return(TRUE);
	}
    }
    MASTER_UNLOCK(&(ctrlPtr->mutex));
    return (FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * ReleaseProc --
 *
 *	Device release proc for controller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static ReturnStatus
ReleaseProc(scsiDevicePtr)
    ScsiDevice	*scsiDevicePtr;
{
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0Init --
 *
 *	Check for the existant of the Sun SCSI0 HBA controller. If it
 *	exists allocate data stuctures for it.
 *
 * Results:
 *	TRUE if the controller exists, FALSE otherwise.
 *
 * Side effects:
 *	Memory may be allocated.
 *
 *----------------------------------------------------------------------
 */
ClientData
DevSCSI0Init(ctrlLocPtr)
    DevConfigController	*ctrlLocPtr;	/* Controller location. */
{
    int	ctrlNum;
    Boolean	found;
    Controller *ctrlPtr;
    int	i,j;

    /*
     * See if the controller is there.
     */
    ctrlNum = ctrlLocPtr->controllerID;
    found =  Probe(ctrlLocPtr->address);
    if (!found) {
	return DEV_NO_CONTROLLER;
    }
    /*
     * It's there. Allocate and fill in the Controller structure.
     */
    if (ctrlNum+1 > numSCSI0Controllers) {
	numSCSI0Controllers = ctrlNum+1;
    }
    Controllers[ctrlNum] = ctrlPtr = (Controller *) malloc(sizeof(Controller));
    bzero((char *) ctrlPtr, sizeof(Controller));
    ctrlPtr->regsPtr = (volatile CtrlRegs *) (ctrlLocPtr->address);
    ctrlPtr->regsPtr->intrVector = ctrlLocPtr->vectorNumber;
    ctrlPtr->name = ctrlLocPtr->name;
    Sync_SemInitDynamic(&(ctrlPtr->mutex),ctrlPtr->name);
    /*
     * Initialized the name, device queue header, and the master lock.
     * The controller comes up with no devices active and no devices
     * attached.  Reserved the devices associated with the
     * targetID of the controller (7).
     */
    ctrlPtr->devQueues = Dev_CtrlQueuesCreate(&(ctrlPtr->mutex),entryAvailProc);
    for (i = 0; i < 8; i++) {
	for (j = 0; j < 8; j++) {
	    ctrlPtr->devicePtr[i][j] = (i == 7) ? (Device *) 0 : (Device *) NIL;
	}
    }
    ctrlPtr->scsiCmdPtr = (ScsiCmd *) NIL;
    Controllers[ctrlNum] = ctrlPtr;
    Reset(ctrlPtr);
    return (ClientData) ctrlPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * DevSCSI0ttachDevice --
 *
 *	Attach a SCSI device using the Sun SCSI0 HBA.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ScsiDevice   *
DevSCSI0AttachDevice(devicePtr, insertProc)
    Fs_Device	*devicePtr;	 /* Device to attach. */
    void	(*insertProc) _ARGS_ ((List_Links *elementPtr,
                                       List_Links *elementListHdrPtr));
                                 /* Queue insert procedure. */
{
    Device *devPtr;
    Controller	*ctrlPtr;
    char   tmpBuffer[512];
    int	   length;
    int	   ctrlNum;
    int	   targetID, lun;

    /*
     * First find the SCSI0 controller this device is on.
     */
    ctrlNum = SCSI_HBA_NUMBER(devicePtr);
    if ((ctrlNum > MAX_SCSI0_CTRLS) ||
	(Controllers[ctrlNum] == (Controller *) 0)) {
	return (ScsiDevice  *) NIL;
    }
    ctrlPtr = Controllers[ctrlNum];
    targetID = SCSI_TARGET_ID(devicePtr);
    lun = SCSI_LUN(devicePtr);
    /*
     * Allocate a device structure for the device and fill in the
     * handle part. This must be created before we grap the MASTER_LOCK.
     */
    devPtr = (Device *) malloc(sizeof(Device));
    bzero((char *) devPtr, sizeof(Device));
    devPtr->handle.devQueue = Dev_QueueCreate(ctrlPtr->devQueues,
				1, insertProc, (ClientData) devPtr);
    devPtr->handle.locationName = "Unknown";
    devPtr->handle.LUN = lun;
    devPtr->handle.releaseProc = ReleaseProc;
    devPtr->handle.maxTransferSize = 63*1024;
    /*
     * See if the device is already present.
     */
    MASTER_LOCK(&(ctrlPtr->mutex));
    /*
     * A device pointer of zero means that targetID/LUN
     * conflicts with that of the HBA. A NIL means the
     * device hasn't been attached yet.
     */
    if (ctrlPtr->devicePtr[targetID][lun] == (Device *) 0) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(void) Dev_QueueDestroy(devPtr->handle.devQueue);
	free((char *) devPtr);
	return (ScsiDevice *) NIL;
    }
    if (ctrlPtr->devicePtr[targetID][lun] != (Device *) NIL) {
	MASTER_UNLOCK(&(ctrlPtr->mutex));
	(void) Dev_QueueDestroy(devPtr->handle.devQueue);
	free((char *) devPtr);
	return (ScsiDevice *) (ctrlPtr->devicePtr[targetID][lun]);
    }

    ctrlPtr->devicePtr[targetID][lun] = devPtr;
    devPtr->targetID = targetID;
    devPtr->ctrlPtr = ctrlPtr;
    MASTER_UNLOCK(&(ctrlPtr->mutex));

    (void) sprintf(tmpBuffer, "%s#%d Target %d LUN %d", ctrlPtr->name, ctrlNum,
			devPtr->targetID, devPtr->handle.LUN);
    length = strlen(tmpBuffer);
    devPtr->handle.locationName = (char *) strcpy(malloc(length+1),tmpBuffer);

    return (ScsiDevice *) devPtr;
}

#endif
