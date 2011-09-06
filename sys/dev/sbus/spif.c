/*
 * Driver for the SUNW,spif multi-port card.
 *
 * For tty units, the low bit of the minor number is the dialout bit,
 *  with the actual unit number shifted over one bit.
 *
 * The RTS bit is negated in hardware - but only on write(!).
 */

#define SPTTY_RX_FIFO_THRESHOLD	4
#define SPTTY_RX_DTR_THRESHOLD	7

#include "spif.h"
#if NSPIF > 0

#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/promlib.h>
#include <dev/sbus/sbusvar.h>
#include <dev/sbus/spifreg.h>

#include "locators.h"

/* There really ought to be a better way to get these.  <machine/conf.h> is
   the place on sparc, but this is dev/sbus, not arch/sparc/dev. */
extern int spttyopen(dev_t, int, int, struct proc *);
extern int spttyclose(dev_t, int, int, struct proc *);
extern int spttyread(dev_t, struct uio *, int);
extern int spttywrite(dev_t, struct uio *, int);
extern int spttyioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern void spttystop(struct tty *, int);
extern struct tty *spttytty(dev_t);
extern int spbppopen(dev_t, int, int, struct proc *);
extern int spbppclose(dev_t, int, int, struct proc *);
extern int spbppread(dev_t, struct uio *, int);
extern int spbppwrite(dev_t, struct uio *, int);
extern int spbppioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int spbpppoll(dev_t, int, struct proc *);

/*
 * Select tty soft interrupt bit based on TTY ipl.
 * This is stolen from magma.c, which says of it "(stole from zs.c)".
 */
#if PIL_TTY == 1
# define SOFT_INTR_LEVEL IE_L1
#elif PIL_TTY == 4
# define SOFT_INTR_LEVEL IE_L4
#elif PIL_TTY == 6
# define SOFT_INTR_LEVEL IE_L6
#else
# error "no suitable software interrupt bit"
#endif

#define NTTY 8
#define NBPP 1

typedef struct spif_softc SPIF;
typedef struct sptty_softc SPTTY;
typedef struct spbpp_softc SPBPP;
typedef struct spif_aa SPAA;

struct spif_aa {
  SPIF *sc;
  int unit;
  int type;
#define SPIF_AA_TYPE_TTY 1
#define SPIF_AA_TYPE_BPP 2
  } ;

struct sptty_softc {
  struct device dev;		/* XXX interface botch */
  SPIF *spif;
  int port;
  struct tty *tty;
  unsigned short int *rb_base;
  int rb_rx;
  int rb_wx;
#define RB_INC(n) (((n) ? : SPTTY_RBUF_SIZE) - 1)
  int txcount;
  unsigned char *txptr;
  unsigned int openflags;
  unsigned int carrier;
  unsigned int flags;
  int softdtr;
  } ;

struct spbpp_softc {
  struct device dev;		/* XXX interface botch */
  SPIF *spif;
  int port;
  } ;

struct spif_softc {
  struct device dev;		/* XXX interface botch */
  struct sbusdev sbdev;		/* XXX interface botch */
  int rev;
  int osc;
  int freq;
  int ofnode;
  int cd180rev;
  bus_space_tag_t tag;
  bus_space_handle_t regs;	/* whole register map */
  bus_space_handle_t stc;	/* STC registers */
  bus_space_handle_t iack;	/* IACK registers (part of STC) */
  bus_space_handle_t dtr;	/* DTR registers */
  bus_space_handle_t ppc;	/* PPC registers */
  void *stc_intr;
  void *ppc_intr;
  void *soft_intr;
  SPTTY *ttys[NTTY];
  SPBPP *bpps[NBPP];
  } ;

extern struct cfdriver sptty_cd;
extern struct cfdriver spbpp_cd;

/* These are really forwards, not externs. */
extern struct cfattach sptty_ca;
extern struct cfattach spbpp_ca;

/* Interrupt-handler flags */
#define SPIF_WANTED   0x01 /* wanted this interrupt */
#define SPIF_WANTSOFT 0x02 /* want to request softint */

static unsigned char istc_read(SPIF *sc, int off)
{
 return(bus_space_read_1(sc->tag,sc->iack,off));
}

static unsigned char stc_read(SPIF *sc, int off)
{
 return(bus_space_read_1(sc->tag,sc->stc,off));
}

static void stc_write(SPIF *sc, int off, unsigned char val)
{
 bus_space_write_1(sc->tag,sc->stc,off,val);
}

static void ccr_write(SPIF *sc, unsigned char val)
{
 int tries;

 do <"ready">
  { for (tries=100000;tries>0;tries--)
     { if (!stc_read(sc,STC_CCR)) break <"ready">;
     }
    printf("%s: ccr timeout\n",&sc->dev.dv_xname[0]);
  } while (0);
 stc_write(sc,STC_CCR,val);
}

static void dtr_write(SPTTY *sc, int set)
{
 sc->softdtr = set ? 1 : 0;
 bus_space_write_1(sc->spif->tag,sc->spif->dtr,sc->port,sc->softdtr);
}

static int dtr_read(SPTTY *sc)
{
 return(sc->softdtr);
}

static void enable_tx(SPTTY *sc)
{
 stc_write(sc->spif,STC_CAR,sc->port);
 stc_write(sc->spif,STC_SRER,stc_read(sc->spif,STC_SRER)|CD180_SRER_TXD);
}

static void disable_tx(SPIF *sc, int port)
{
 stc_write(sc,STC_CAR,port);
 stc_write(sc,STC_SRER,stc_read(sc,STC_SRER)&~CD180_SRER_TXD);
}

static void enable_rx(SPTTY *sc)
{
 stc_write(sc->spif,STC_CAR,sc->port);
 stc_write(sc->spif,STC_SRER,stc_read(sc->spif,STC_SRER)|CD180_SRER_RXD);
}

static void disable_rx(SPTTY *sc)
{
 stc_write(sc->spif,STC_CAR,sc->port);
 stc_write(sc->spif,STC_SRER,stc_read(sc->spif,STC_SRER)&~CD180_SRER_RXD);
}

static int spif_stcint_rx(SPIF *sc)
{
 SPTTY *t;
 unsigned char chan;
 int rv;
 int d;
 int x;
 int p;
 int n;

 rv = SPIF_WANTED;
 chan = CD180_GSCR_CHANNEL(stc_read(sc,STC_GSCR1));
 n = stc_read(sc,STC_RDCR);
 t = (chan < NTTY) ? sc->ttys[chan] : 0;
 if (t)
  { x = t->rb_wx;
    if (n) rv |= SPIF_WANTSOFT;
    for (;n>0;n--)
     { stc_read(sc,STC_RCSR);
       d = stc_read(sc,STC_RDR);
       p = x;
       x = RB_INC(x);
       if (x == t->rb_rx)
	{ t->flags |= SPTTYF_RING_OVERFLOW;
	  break;
	}
       else
	{ t->rb_base[p] = d;
	}
     }
    t->rb_wx = x;
  }
 else
  { disable_rx(t);
  }
 stc_write(sc,STC_EOSRR,0);
 return(rv);
}

static int spif_stcint_rxexc(SPIF *sc)
{
 SPTTY *t;
 unsigned char chan;
 int d;
 int x;

 chan = CD180_GSCR_CHANNEL(stc_read(sc,STC_GSCR1));
 /* the sequence point between these two stc_read()s is important! */
 d = stc_read(sc,STC_RCSR) * 0x100;
 d |= stc_read(sc,STC_RDR);
 t = (chan < NTTY) ? sc->ttys[chan] : 0;
 if (t)
  { x = RB_INC(t->rb_wx);
    if (x == t->rb_rx)
     { t->flags |= SPTTYF_RING_OVERFLOW;
     }
    else
     { t->rb_base[t->rb_wx] = d;
       t->rb_wx = x;
     }
  }
 else
  { disable_rx(t);
  }
 stc_write(sc,STC_EOSRR,0);
 return(SPIF_WANTED|SPIF_WANTSOFT);
}

static int spif_stcint_tx(SPIF *sc)
{
 SPTTY *t;
 unsigned char chan;
 int rv;
 int n;

 rv = SPIF_WANTED;
 chan = CD180_GSCR_CHANNEL(stc_read(sc,STC_GSCR1));
 t = (chan < NTTY) ? sc->ttys[chan] : 0;
 if (! t)
  { disable_tx(sc,chan);
    stc_write(sc,STC_EOSRR,0);
    return(SPIF_WANTED);
  }
 if (! (t->flags & SPTTYF_STOP))
  { n = 0;
    if (t->flags & SPTTYF_SET_BREAK)
     { stc_write(sc,STC_TDR,CD180_TDR_MAGIC);
       stc_write(sc,STC_TDR,CD180_TDR_SBRK);
       t->flags &= ~SPTTYF_SET_BREAK;
       n += 2;
     }
    if (t->flags & SPTTYF_CLR_BREAK)
     { stc_write(sc,STC_TDR,CD180_TDR_MAGIC);
       stc_write(sc,STC_TDR,CD180_TDR_CBRK);
       t->flags &= ~SPTTYF_CLR_BREAK;
       n += 2;
     }
    while (t->txcount > 0)
     { unsigned char ch;
       ch = *t->txptr;
       if (ch == CD180_TDR_MAGIC)
	{ if (n+2 > CD180_TX_FIFO_SIZE) break;
	  stc_write(sc,STC_TDR,CD180_TDR_MAGIC);
	  stc_write(sc,STC_TDR,CD180_TDR_ESCMAGIC);
	  n += 2;
	}
       else
	{ if (n+1 > CD180_TX_FIFO_SIZE) break;
	  stc_write(sc,STC_TDR,ch);
	  n ++;
	}
       t->txptr ++;
       t->txcount --;
     }
  }
 if ((t->txcount == 0) || (t->flags & SPTTYF_STOP))
  { disable_tx(sc,chan);
    t->flags &= ~SPTTYF_STOP;
    t->flags |= SPTTYF_DONE;
    rv |= SPIF_WANTSOFT;
  }
 stc_write(sc,STC_EOSRR,0);
 return(rv);
}

static int spif_stcint_mx(SPIF *sc)
{
 SPTTY *t;
 unsigned char chan;
 unsigned char mcr;
 int rv;

 rv = SPIF_WANTED;
 chan = CD180_GSCR_CHANNEL(stc_read(sc,STC_GSCR1));
 t = (chan < NTTY) ? sc->ttys[chan] : 0;
 mcr = stc_read(sc,STC_MCR);
 if (t && (mcr & CD180_MCR_CD))
  { t->flags |= SPTTYF_CDCHG;
    rv |= SPIF_WANTSOFT;
  }
 stc_write(sc,STC_MCR,0);
 stc_write(sc,STC_EOSRR,0);
 return(rv);
}

static int spif_stc_intr(void *scv)
{
 SPIF *sc;
 int i;
 int want;
 unsigned char v;

 sc = scv;
 want = 0;
 for (i=NTTY;i>0;i--)
  { v = istc_read(sc,STC_RRAR) & CD180_GSVR_IMASK;
    if (v == CD180_GSVR_RXGOOD) want |= spif_stcint_rx(sc);
    else if (v == CD180_GSVR_RXEXCEPTION) want |= spif_stcint_rxexc(sc);
  }
 for (i=NTTY;i>0;i--)
  { v = istc_read(sc,STC_TRAR) & CD180_GSVR_IMASK;
    if (v == CD180_GSVR_TXDATA) want |= spif_stcint_tx(sc);
  }
 for (i=NTTY;i>0;i--)
  { v = istc_read(sc,STC_MRAR) & CD180_GSVR_IMASK;
    if (v == CD180_GSVR_STATCHG) want |= spif_stcint_mx(sc);
  }
 if (want & SPIF_WANTSOFT)
  {
#if defined(SUN4M)
    if (CPU_ISSUN4M)
     { raise(0,PIL_TTY);
     }
    else
#endif
     { ienab_bis(SOFT_INTR_LEVEL);
     }
  }
 return((want&SPIF_WANTED)?1:0);
}

/* parallel port not yet implemented */
static int spif_ppc_intr(void *scv)
{
 SPIF *sc;

 sc = scv;
 return(0);
}

static int spif_soft_intr(void *scv)
{
 SPIF *sc;
 int l;
 SPTTY *t;
 struct tty *tty;
 int d;
 int s;
 int wanted;
 int flags;

 sc = scv;
 wanted = 0;
 for (l=NTTY-1;l>=0;l--)
  { t = sc->ttys[l];
    if (! t) continue;
    tty = t->tty;
    if (! (tty->t_state & TS_ISOPEN)) continue;
    while (t->rb_rx != t->rb_wx)
     { d = t->rb_base[t->rb_rx];
       t->rb_rx = RB_INC(t->rb_rx);
       (*linesw[tty->t_line].l_rint)(
		((d & ((CD180_RCSR_BE|CD180_RCSR_FE)<<8)) ? TTY_FE : 0) |
			((d & (CD180_RCSR_PE<<8)) ? TTY_PE : 0) |
			(d & 0xff),
		tty );
       if (d & (CD180_RCSR_OE<<8))
	{ log(LOG_WARNING,"%s: fifo overrun\n",&t->dev.dv_xname[0]);
	}
       wanted = 1;
     }
    s = splhigh(); /* block hard interrupt */
    flags = t->flags;
    t->flags &= ~(SPTTYF_DONE|SPTTYF_CDCHG|SPTTYF_RING_OVERFLOW);
    splx(s);
    if (flags & SPTTYF_CDCHG)
     { s = spltty();
       stc_write(sc,STC_CAR,t->port);
       d = stc_read(sc,STC_MSVR);
       splx(s);
       t->carrier = (d & CD180_MSVR_CD) ? 1 : 0;
       (*linesw[tty->t_line].l_modem)(tty,t->carrier);
       wanted = 1;
     }
    if (flags & SPTTYF_RING_OVERFLOW)
     { log(LOG_WARNING,"%s: ring buffer overflow\n",&t->dev.dv_xname[0]);
       wanted = 1;
     }
    if (flags & SPTTYF_DONE)
     { ndflush(&tty->t_outq,t->txptr-tty->t_outq.c_cf);
       tty->t_state &= ~TS_BUSY;
       (*linesw[tty->t_line].l_start)(tty);
       wanted = 1;
     }
  }
 return(wanted);
}

static int sptty_modem(SPTTY *sc, int bits, int how)
{
 int s;
 int msvr;

 s = spltty();
 stc_write(sc->spif,STC_CAR,sc->port);
 switch (how)
  { case DMGET:
       bits = TIOCM_LE;
       if (dtr_read(sc)) bits |= TIOCM_DTR;
       msvr = stc_read(sc->spif,STC_MSVR);
       if (msvr & CD180_MSVR_RTS) bits |= TIOCM_RTS;
       if (msvr & CD180_MSVR_CTS) bits |= TIOCM_CTS;
       if (msvr & CD180_MSVR_CD) bits |= TIOCM_CD;
       if (msvr & CD180_MSVR_DSR) bits |= TIOCM_DSR;
       break;
    case DMSET:
       dtr_write(sc,bits&TIOCM_DTR);
       if (bits & TIOCM_RTS)
	{ stc_write(sc->spif,STC_MSVR,
			stc_read(sc->spif,STC_MSVR)&~CD180_MSVR_RTS);
	}
       else
	{ stc_write(sc->spif,STC_MSVR,
			stc_read(sc->spif,STC_MSVR)|CD180_MSVR_RTS);
	}
       break;
    case DMBIS:
       if (bits & TIOCM_DTR) dtr_write(sc,1);
       if ((bits & TIOCM_RTS) && !(sc->tty->t_cflag & CRTSCTS))
	{ stc_write(sc->spif,STC_MSVR,
			stc_read(sc->spif,STC_MSVR)&~CD180_MSVR_RTS);
	}
       break;
    case DMBIC:
       if (bits & TIOCM_DTR) dtr_write(sc,0);
       if ((bits & TIOCM_RTS) && !(sc->tty->t_cflag & CRTSCTS))
	{ stc_write(sc->spif,STC_MSVR,
			stc_read(sc->spif,STC_MSVR)|CD180_MSVR_RTS);
	}
       break;
  }
 return(bits);
}

static int baud_to_brg(speed_t baud, int clock, unsigned short int *brgp)
{
 unsigned int rate;
 speed_t actbaud;

 rate = ((clock / (8 * baud)) + 1) >> 1;
 if ((rate == 0) || (rate > 0xffff)) return(1);
 actbaud = clock / (8 * ((rate * 2) - 1));
 if ((actbaud < (baud-(baud>>4))) || (actbaud > (baud+(baud>>4)))) return(1);
 *brgp = rate;
 return(0);
}

static speed_t brg_to_baud(int clock, unsigned short int brg)
{
 return( clock / (8 * ((brg * 2) - 1)) );
}

static int sptty_param(struct tty *t, struct termios *tio)
{
 int unit;
 SPTTY *sc;
 SPIF *spif;
 int s;
 unsigned short int txb;
 unsigned short int rxb;
 unsigned int bits;

 unit = minor(t->t_dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("sptty_param bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("sptty_param not there");
 if (sc->tty != t) panic("sptty_param tty wrong");
 spif = sc->spif;
 if ( (tio->c_ospeed && baud_to_brg(tio->c_ospeed,spif->freq,&txb)) ||
      (tio->c_ispeed && baud_to_brg(tio->c_ispeed,spif->freq,&rxb)) )
  { return(EINVAL);
  }
 if (tio->c_ospeed) tio->c_ospeed = brg_to_baud(spif->freq,txb);
 if (tio->c_ispeed) tio->c_ispeed = brg_to_baud(spif->freq,rxb);
 s = spltty();
 sptty_modem(sc,TIOCM_DTR,(tio->c_ospeed==0)?DMBIC:DMBIS);
 stc_write(spif,STC_CAR,sc->port);
 bits = 0;
 if (tio->c_cflag & PARENB)
  { bits |= CD180_COR1_PARMODE_NORMAL |
	    ((tio->c_cflag & PARODD) ? CD180_COR1_ODDPAR : CD180_COR1_EVENPAR);
  }
 else
  { bits |= CD180_COR1_PARMODE_NO;
  }
 if (! (tio->c_iflag & INPCK)) bits |= CD180_COR1_IGNPAR;
 if (tio->c_cflag & CSTOPB) bits |= CD180_COR1_STOP2;
 switch (tio->c_cflag & CSIZE)
  { case CS5: bits |= CD180_COR1_CS5; break;
    case CS6: bits |= CD180_COR1_CS6; break;
    case CS7: bits |= CD180_COR1_CS7; break;
    default:
    case CS8: bits |= CD180_COR1_CS8; break;
  }
 stc_write(spif,STC_COR1,bits);
 ccr_write(spif,CD180_CCR_CMD_COR|CD180_CCR_CORCHG1);
 bits = CD180_COR2_ETC;
 if (tio->c_cflag & CRTSCTS) bits |= CD180_COR2_CTSAE;
 stc_write(spif,STC_COR2,bits);
 ccr_write(spif,CD180_CCR_CMD_COR|CD180_CCR_CORCHG2);
 stc_write(spif,STC_COR3,SPTTY_RX_FIFO_THRESHOLD);
 ccr_write(spif,CD180_CCR_CMD_COR|CD180_CCR_CORCHG3);
 /* XXX should these be c_cc[VSTOP] and c_cc[VSTART]? */
 stc_write(spif,STC_SCHR1,0x11); /* ^Q */
 stc_write(spif,STC_SCHR2,0x13); /* ^S */
 stc_write(spif,STC_SCHR3,0x11); /* ^Q */
 stc_write(spif,STC_SCHR4,0x13); /* ^S */
 /* XXX what is the 0x12 magic value? */
 stc_write(spif,STC_RTPR,0x12);
 stc_write(spif,STC_MCOR1,CD180_MCOR1_CDZD|SPTTY_RX_DTR_THRESHOLD);
 stc_write(spif,STC_MCOR2,CD180_MCOR2_CDOD);
 stc_write(spif,STC_MCR,0);
 if (tio->c_ospeed)
  { stc_write(spif,STC_TBPRH,txb>>8);
    stc_write(spif,STC_TBPRL,txb&0xff);
  }
 if (tio->c_ispeed)
  { stc_write(spif,STC_RBPRH,rxb>>8);
    stc_write(spif,STC_RBPRL,rxb&0xff);
  }
 ccr_write(spif,CD180_CCR_CMD_CHAN|CD180_CCR_CHAN_TXEN|CD180_CCR_CHAN_RXEN);
 sc->carrier = (stc_read(spif,STC_MSVR) & CD180_MSVR_CD) ? 1 : 0;
 splx(s);
 return(0);
}

int spttyopen(dev_t dev, int flags, int mode, struct proc *p)
{
 int unit;
 int dialout;
 SPTTY *sc;
 struct tty *t;
 int s;
 int err;

 dialout = minor(dev) & 1;
 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) return(ENXIO);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) return(ENXIO);
 t = sc->tty;
 t->t_dev = dev;
 if ( ((t->t_state & (TS_ISOPEN|TS_XCLUDE)) == (TS_ISOPEN|TS_XCLUDE)) &&
      suser(p->p_ucred,&p->p_acflag) )
  { return(EBUSY);
  }
 s = spltty();
 if (!(t->t_state & TS_ISOPEN) && !t->t_wopen)
  { ttychars(t);
    t->t_iflag = TTYDEF_IFLAG;
    t->t_oflag = TTYDEF_OFLAG;
    t->t_cflag = TTYDEF_CFLAG;
    t->t_lflag = TTYDEF_LFLAG;
    t->t_ispeed = TTYDEF_SPEED;
    t->t_ospeed = TTYDEF_SPEED;
    if (sc->openflags & TIOCFLAG_CLOCAL) t->t_cflag |= CLOCAL;
    if (sc->openflags & TIOCFLAG_CRTSCTS) t->t_cflag |= CRTSCTS;
    if (sc->openflags & TIOCFLAG_MDMBUF) t->t_cflag |= MDMBUF;
    if (sc->openflags & TIOCFLAG_CDTRCTS) t->t_cflag |= CDTRCTS;
    sc->rb_rx = 0;
    sc->rb_wx = 0;
    sc->txcount = 0;
    sc->txptr = 0;
    stc_write(sc->spif,STC_CAR,sc->port);
    ccr_write(sc->spif,CD180_CCR_CMD_RESET|CD180_CCR_RESETCHAN);
    stc_write(sc->spif,STC_CAR,sc->port);
    sptty_param(t,&t->t_termios);
    ttsetwater(t);
    stc_write(sc->spif,STC_SRER,CD180_SRER_CD);
    enable_rx(sc);
    if ((sc->openflags & TIOCFLAG_SOFTCAR) || sc->carrier)
     { t->t_state |= TS_CARR_ON;
     }
    else
     { t->t_state &= ~TS_CARR_ON;
     }
  }
 splx(s);
 err = ttyopen(t,dialout,flags&O_NONBLOCK);
 if (err) return(err);
 err = (*linesw[t->t_line].l_open)(dev,t);
 if (err) return(err);
 return(0);
}

int spttyclose(dev_t dev, int flags, int mode, struct proc *p)
{
 int unit;
 SPTTY *sc;
 struct tty *t;
 int s;

 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttyclose bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttyclose not there");
 t = sc->tty;
 (*linesw[t->t_line].l_close)(t,flags);
 ttyclose(t);
 s = spltty();
 if ((t->t_cflag & HUPCL) && !(t->t_state & TS_ISOPEN))
  { sptty_modem(sc,0,DMSET);
    stc_write(sc->spif,STC_CAR,sc->port);
    stc_write(sc->spif,STC_CCR,CD180_CCR_CMD_RESET|CD180_CCR_RESETCHAN);
  }
 splx(s);
 return(0);
}

int spttyread(dev_t dev, struct uio *uio, int flags)
{
 int unit;
 SPTTY *sc;
 struct tty *t;

 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttyread bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttyread not there");
 t = sc->tty;
 return((*linesw[t->t_line].l_read)(t,uio,flags));
}

int spttywrite(dev_t dev, struct uio *uio, int flags)
{
 int unit;
 SPTTY *sc;
 struct tty *t;

 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttywrite bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttywrite not there");
 t = sc->tty;
 return((*linesw[t->t_line].l_write)(t,uio,flags));
}

int spttyioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
 int unit;
 SPTTY *sc;
 struct tty *t;
 int err;

 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttywrite bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttywrite not there");
 t = sc->tty;
 err = (*linesw[t->t_line].l_ioctl)(t,cmd,data,flags,p);
 if (err >= 0) return(err);
 err = ttioctl(t,cmd,data,flags,p);
 if (err >= 0) return(err);
 switch (cmd)
  { case TIOCSBRK:
       sc->flags |= SPTTYF_SET_BREAK;
       sc->flags &= ~SPTTYF_CLR_BREAK;
       enable_tx(sc);
       break;
    case TIOCCBRK:
       sc->flags |= SPTTYF_CLR_BREAK;
       sc->flags &= ~SPTTYF_SET_BREAK;
       enable_tx(sc);
       break;
    case TIOCSDTR:
       sptty_modem(sc,TIOCM_DTR,DMBIS);
       break;
    case TIOCCDTR:
       sptty_modem(sc,TIOCM_DTR,DMBIC);
       break;
    case TIOCMSET:
       sptty_modem(sc,*(int *)data,DMSET);
       break;
    case TIOCMBIS:
       sptty_modem(sc,*(int *)data,DMBIS);
       break;
    case TIOCMBIC:
       sptty_modem(sc,*(int *)data,DMBIC);
       break;
    case TIOCMGET:
       *(int *)data = sptty_modem(sc,0,DMGET);
       break;
    case TIOCGFLAGS:
       *(int *)data = sc->openflags;
       break;
    case TIOCSFLAGS:
       if (suser(p->p_ucred,&p->p_acflag))
	{ err = EPERM;
	}
       else
	{ sc->openflags = (*(int *)data) &
		(TIOCFLAG_SOFTCAR | TIOCFLAG_CLOCAL |
		 TIOCFLAG_CRTSCTS | TIOCFLAG_MDMBUF |
		 TIOCFLAG_CDTRCTS);
	}
       break;
    default:
       err = ENOTTY;
       break;
  }
 return(err);
}

void spttystop(struct tty *t, int flags)
{
 int unit;
 SPTTY *sc;
 int s;

 unit = minor(t->t_dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttystop bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttystop not there");
 if (sc->tty != t) panic("spttystop tty wrong");
 s = spltty();
 if (t->t_state & TS_BUSY)
  { if (! (t->t_state & TS_TTSTOP)) t->t_state |= TS_FLUSH;
    sc->flags |= SPTTYF_STOP;
  }
 splx(s);
}

struct tty *spttytty(dev_t dev)
{
 int unit;
 SPTTY *sc;

 unit = minor(dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("spttytty bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("spttytty not there");
 return(sc->tty);
}

static void sptty_start(struct tty *t)
{
 int unit;
 SPTTY *sc;
 SPIF *spif;
 int s;

 unit = minor(t->t_dev) >> 1;
 if (unit >= sptty_cd.cd_ndevs) panic("sptty_start bad unit %d",unit);
 sc = (void *) sptty_cd.cd_devs[unit]; /* XXX interface botch */
 if (! sc) panic("sptty_start not there");
 if (sc->tty != t) panic("sptty_start tty wrong");
 spif = sc->spif;
 s = spltty();
 if (! (t->t_state & (TS_TTSTOP|TS_TIMEOUT|TS_BUSY)))
  { if (t->t_outq.c_cc < t->t_lowat)
     { if (t->t_state & TS_ASLEEP)
	{ t->t_state &= ~TS_ASLEEP;
	  wakeup(&t->t_outq);
	}
       selwakeup(&t->t_wsel);
     }
    if (t->t_outq.c_cc)
     { sc->txcount = ndqb(&t->t_outq,0);
       sc->txptr = t->t_outq.c_cf;
       t->t_state |= TS_BUSY;
       enable_tx(sc);
     }
  }
 splx(s);
}

int spbppopen(dev_t dev, int flags, int mode, struct proc *p)
{
 dev=dev; flags=flags; mode=mode; p=p;
 return(ENXIO);
}

int spbppclose(dev_t dev, int flags, int mode, struct proc *p)
{
 dev=dev; flags=flags; mode=mode; p=p;
 panic("spbppclose");
}

int spbppread(dev_t dev, struct uio *uio, int flags)
{
 dev=dev; uio=uio; flags=flags;
 panic("spbppread");
}

int spbppwrite(dev_t dev, struct uio *uio, int flags)
{
 dev=dev; uio=uio; flags=flags;
 panic("spbppwrite");
}

int spbppioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
 dev=dev; cmd=cmd; data=data; flags=flags; p=p;
 panic("spbppioctl");
}

int spbpppoll(dev_t dev, int rw, struct proc *p)
{
 dev=dev; rw=rw; p=p;
 panic("spbpppoll");
}

static int sptty_match(struct device *parent, struct cfdata *cf, void *aux)
{
 SPAA *aa;
 SPIF *sc;

 aa = aux;
 sc = (void *) parent; /* XXX interface botch */
 if (aa->sc != sc) return(0);
 if (aa->type != SPIF_AA_TYPE_TTY) return(0);
 if ((aa->unit < 0) || (aa->unit >= NTTY)) return(0);
 if (sc->ttys[aa->unit]) return(0);
 return(1);
}

static void sptty_attach(struct device *parent, struct device *self, void *aux)
{
 SPAA *aa;
 SPTTY *sc;
 SPIF *psc;

 sc = (void *) self; /* XXX interface botch */
 psc = (void *) parent; /* XXX interface botch */
 aa = aux;
 printf(": ");
 if (aa->type != SPIF_AA_TYPE_TTY) panic("sptty isn't");
 if ((aa->unit < 0) || (aa->unit > NTTY)) panic("sptty bad unit %d",aa->unit);
 if (psc->ttys[aa->unit]) panic("sptty unit %d already present",aa->unit);
 sc->spif = psc;
 sc->port = aa->unit;
 psc->ttys[aa->unit] = sc;
 printf("tty unit %d",aa->unit);
 sc->rb_base = malloc(SPTTY_RBUF_SIZE*sizeof(*sc->rb_base),M_DEVBUF,M_NOWAIT);
 if (! sc->rb_base)
  { printf(" (no ring buffer)\n");
    sc->tty = 0;
    return;
  }
 sc->tty = ttymalloc();
 if (! sc->tty)
  { printf(" (no struct tty)\n");
    free(sc->rb_base,M_DEVBUF);
    return;
  }
 tty_attach(sc->tty);
 sc->tty->t_oproc = &sptty_start;
 sc->tty->t_param = &sptty_param;
 sc->rb_rx = 0;
 sc->rb_wx = 0;
 sc->txcount = 0;
 sc->txptr = 0;
 sc->openflags = 0;
 sc->carrier = 0;
 sc->flags = 0;
 sc->softdtr = 0;
 dtr_write(sc,0);
 printf("\n");
}

static int spbpp_match(struct device *parent, struct cfdata *cf, void *aux)
{
 SPAA *aa;
 SPIF *sc;

 aa = aux;
 sc = (void *) parent; /* XXX interface botch */
 if (aa->sc != sc) return(0);
 if (aa->type != SPIF_AA_TYPE_BPP) return(0);
 if ((aa->unit < 0) || (aa->unit >= NBPP)) return(0);
 if (sc->bpps[aa->unit]) return(0);
 return(1);
}

static void spbpp_attach(struct device *parent, struct device *self, void *aux)
{
 SPAA *aa;
 SPBPP *sc;
 SPIF *psc;

 sc = (void *) self; /* XXX interface botch */
 psc = (void *) parent; /* XXX interface botch */
 aa = aux;
 printf(": ");
 if (aa->type != SPIF_AA_TYPE_BPP) panic("spbpp isn't");
 if ((aa->unit < 0) || (aa->unit > NBPP)) panic("spbpp bad unit %d",aa->unit);
 if (psc->bpps[aa->unit]) panic("spbpp unit %d already present",aa->unit);
 sc->spif = psc;
 sc->port = aa->unit;
 psc->bpps[aa->unit] = sc;
 printf("bpp unit %d (not implemented)\n",aa->unit);
}

static int spif_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
 SPAA *aa;

 aa = aux;
 if ( (cf->cf_loc[SPIFCF_UNIT] != SPIFCF_UNIT_DEFAULT) &&
      (cf->cf_loc[SPIFCF_UNIT] != aa->unit) ) return(0);
 if ((void *)parent != (void *)aa->sc) return(0);
 switch (aa->type)
  { case SPIF_AA_TYPE_TTY:
       if (cf->cf_attach != &sptty_ca) return(0);
       break;
    case SPIF_AA_TYPE_BPP:
       if (cf->cf_attach != &spbpp_ca) return(0);
       break;
    default:
       return(0);
       break;
  }
 return((*cf->cf_attach->ca_match)(parent,cf,aux));
}

static int spif_match(struct device *parent, struct cfdata *cf, void *aux)
{
 return(!strcmp(((struct sbus_attach_args *)aux)->sa_name,"SUNW,spif"));
}

static void spif_attach(struct device *parent, struct device *self, void *aux)
{
 SPIF *sc;
 struct sbus_attach_args *sa;
 int i;
 SPAA aa;

 sc = (void *) self; /* XXX interface botch */
 sa = aux;
 printf(": ");
 if (sa->sa_nintr != 2)
  { printf("wrong interrupt count (wanted 2 got %d)\n",sa->sa_nintr);
    return;
  }
 if (sa->sa_nreg != 1)
  { printf("wrong register bank count (wanted 1 got %d)\n",sa->sa_nreg);
    return;
  }
 sc->ofnode = sa->sa_node;
 sc->rev = getpropint(sc->ofnode,"revlev",0);
 sc->osc = getpropint(sc->ofnode,"verosc",0);
 sc->tag = sa->sa_bustag;
 if (sbus_bus_map( sc->tag, sa->sa_slot, sa->sa_offset, sa->sa_size,
		   BUS_SPACE_MAP_LINEAR, 0, &sc->regs ) != 0)
  { printf("cannot map registers\n");
    return;
  }
 do <"unmap">
  { if ( (bus_space_subregion( sc->tag, sc->regs,
			       STC_REG_OFFSET, STC_REG_LEN,
			       &sc->stc ) != 0) ||
	 (bus_space_subregion( sc->tag, sc->regs,
			       ISTC_REG_OFFSET, ISTC_REG_LEN,
			       &sc->iack ) != 0) ||
	 (bus_space_subregion( sc->tag, sc->regs,
			       DTR_REG_OFFSET, DTR_REG_LEN,
			       &sc->dtr ) != 0) ||
	 (bus_space_subregion( sc->tag, sc->regs,
			       PPC_REG_OFFSET, PPC_REG_LEN,
			       &sc->ppc ) != 0) )
     { printf("cannot subregion registers\n");
       break <"unmap">;
     }
    sc->stc_intr = bus_intr_establish(
	sc->tag, sa->sa_intr[SERIAL_INTR].sbi_pri, 0, &spif_stc_intr, sc);
    if (! sc->stc_intr)
     { printf("cannot establish STC interrupt\n");
       break <"unmap">;
     }
    sc->ppc_intr = bus_intr_establish(
	sc->tag, sa->sa_intr[PARALLEL_INTR].sbi_pri, 0, &spif_ppc_intr, sc);
    if (! sc->ppc_intr)
     { printf("cannot establish PPC interrupt\n");
       /* XXX need a bus_intr_disestablish on stc_intr here! */
       break <"unmap">;
     }
    sc->soft_intr = bus_intr_establish(
	sc->tag, PIL_TTY, BUS_INTR_ESTABLISH_SOFTINTR, &spif_soft_intr, sc);
    if (! sc->soft_intr)
     { printf("cannot establish soft interrupt\n");
       /* XXX need a bus_intr_disestablish on stc_intr and ppc_intr here! */
       break <"unmap">;
     }
    sc->cd180rev = stc_read(sc,STC_GFRCR);
    stc_write(sc,STC_GSVR,0);
    ccr_write(sc,CD180_CCR_CMD_RESET|CD180_CCR_RESETALL);
    while (stc_read(sc,STC_GSVR) != 0xff) ;
    while (stc_read(sc,STC_GFRCR) != sc->cd180rev) ;
    stc_write(sc,STC_PPRH,CD180_PPRH);
    stc_write(sc,STC_PPRL,CD180_PPRL);
    stc_write(sc,STC_MSMR,SPIF_MSMR);
    stc_write(sc,STC_TSMR,SPIF_TSMR);
    stc_write(sc,STC_RSMR,SPIF_RSMR);
    stc_write(sc,STC_GSVR,0);
    stc_write(sc,STC_GSCR1,0);
    stc_write(sc,STC_GSCR2,0);
    stc_write(sc,STC_GSCR3,0);
    printf("rev %#x/%#x osc %d",sc->rev,sc->cd180rev,sc->osc);
    switch (sc->osc)
     { case SPIF_OSC9:
	  sc->freq = 9830400;
	  printf(" (freq %d)",sc->freq);
	  break;
       case SPIF_OSC10:
	  sc->freq = 10000000;
	  printf(" (freq %d)",sc->freq);
	  break;
       default:
	  sc->freq = 10000000;
	  printf(" (assuming freq %d)",sc->freq);
	  break;
     }
    printf("\n");
    for (i=NTTY-1;i>=0;i--) sc->ttys[i] = 0;
    for (i=NBPP-1;i>=0;i--) sc->bpps[i] = 0;
    aa.sc = sc;
    aa.type = SPIF_AA_TYPE_TTY;
    for (i=0;i<NTTY;i++)
     { aa.unit = i;
       config_found_sm(self,&aa,0,&spif_submatch);
     }
    aa.type = SPIF_AA_TYPE_BPP;
    for (i=0;i<NBPP;i++)
     { aa.unit = i;
       config_found_sm(self,&aa,0,&spif_submatch);
     }
    return;
  } while (0);
 bus_space_unmap(sc->tag,sc->regs,sa->sa_size);
}

/*
 * Autoconfig glue
 */
struct cfattach spif_ca
 = { sizeof(SPIF), &spif_match, &spif_attach };

struct cfattach sptty_ca
 = { sizeof(SPTTY), &sptty_match, &sptty_attach };

struct cfattach spbpp_ca
 = { sizeof(SPBPP), &spbpp_match, &spbpp_attach };

#endif /* NSPIF > 0 */
