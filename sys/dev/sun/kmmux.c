/*
 * Keyboard/mouse multiplexer device.  This sits between kbd/ms pairs
 *  and zsc chip interfaces:
 *
 *	kmmux0 at zs1 channel 0
 *	kmmux1 at zs1 channel 1
 *	kbd0 at kmmux0 bond 0
 *	ms0 at kmmux1 bond 0
 *	kbd1 at kmmux0 bond 1
 *	ms3 at kmmux1 bond 1
 *
 * kmmux units are paired: even units are keyboards, odd units are
 *  mice.  A kbd or ms will not attach to the wrong kmmux unit, and
 *  each even/odd pair functions as a single mux.  When switching, the
 *  bond values identify which kbd and ms devices go together; in the
 *  above, for example, kbd0 and ms0 go together, and kbd1 and ms3 go
 *  together.  (That is, when kmmux switches the real keyboard to kbd1,
 *  it will switch the real mouse to ms3.)  Bond values are specific to
 *  a kmmux pair; the above would not conflict with "bond 0" used on
 *  kbd and ms units attaching to kmmux2/kmmux3.  If the bond values
 *  are omitted, the kbd/ms unit number is used instead.  Multiple kbd
 *  or ms children with matching bond values is an error and will cause
 *  all but one of the conflicting children to fail to attach.
 *
 * Configuring an even kmmux without the matching odd kmmux will simply
 *  dummy out the mouse side of things.  Configuring an odd kmmux
 *  without the matching even kmmux will "work" but is useless, as
 *  there will be no way to switch.
 *
 * If there is no ms bonded with a kbd, mouse input will be discarded
 *  when that kbd is in use; if there is no kbd bonded with an ms,
 *  keyboard input will be ignored (except for switch sequences) when
 *  that ms is in use.
 *
 * The above is actually a slight lie.  It is possible to split
 *  keyboard and mouse focus.  If a k or m is typed before the digit
 *  selecting a sub-mux, only keyboard or mouse focus will be switched.
 *
 * There is a problem here.  We really ought to switch the keyboard for
 *  all purposes, including output from BELs and such.  However,
 *  kbd_docmd, in kbd.c, does not take any argument saying which kbd
 *  instance to send the command to - and this would be difficult to
 *  fix, because the framebuffer driver needs to produce beeps in
 *  response to BELs without having a keyboard instance handy.  (In
 *  principle, it should perhaps use the keyboard currently connected
 *  to /dev/console.  But that is difficult, since all keyboards not in
 *  use otherwise are connected to the console.  Rather than redesign
 *  the whole keyboard interface, we punt, and make a mux's bell be the
 *  OR of the mux's own bell with all of that mux's keyboards bells.
 *  (This is the way i386 console beeps work, and it's useful there.
 *  Less useful here, with no virtual consoles, but still.)  We take
 *  care to track each virtual keyboard's bell state correctly.
 */

#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <machine/psl.h>
#include <machine/kbd.h>
#include <dev/sun/msvar.h>
#include <dev/sun/kbdvar.h>
#include <dev/ic/z8530sc.h>
#include <dev/ic/z8530reg.h>
#include <machine/z8530var.h>
#include <dev/sun/kmmuxvar.h>

#include "locators.h"

#include "kmmux.h"

/* Test whether a unit or softc is for keyboard or mouse */
#define MOUSE_UNIT(u) ((u) & 1)
#define KBD_UNIT(u) (!MOUSE_UNIT(u))
#define MOUSE_HALF(km) MOUSE_UNIT((km)->dev.dv_unit)
#define KBD_HALF(km) (!MOUSE_HALF(km))

/* Defines for the keyboard keys we care about */
#define KEY_CTRL  0x4c
#define KEY_F1    0x05
#define KEY_ESC   0x1d
#define KEY_1     0x1e
#define KEY_2     0x1f
#define KEY_3     0x20
#define KEY_4     0x21
#define KEY_5     0x22
#define KEY_6     0x23
#define KEY_7     0x24
#define KEY_8     0x25
#define KEY_9     0x26
#define KEY_0     0x27
#define KEY_k     0x54
#define KEY_m     0x6a
#define KEY_SPACE 0x79
#define KEY_IDLE  0x7f
#define KEY__UP 0x80 /* indicates a release */

#define NMUX ((NKMMUX+1)/2)

typedef struct kmmux_softc SOFTC;
typedef struct kmmux_mux MUX;
typedef struct pair PAIR;

struct pair {
  struct kbd_softc *kbd;
  struct ms_softc *ms;
  int bond;
  int click;
  int leds;
  int bell;
  MUX *mux;
  int kid;
  unsigned int flags;
#define KMPF_SETLEDS    0x00000001
#define KMPF_WANTRESET  0x00000002
#define KMPF_WANTLAYOUT 0x00000004
  } ;

struct kmmux_mux {
  SOFTC *kbd;
  SOFTC *ms;
  unsigned int mcur;
  unsigned int kcur;
  unsigned int nkids;
  PAIR **kids;
  unsigned int flags;
#define KMMF_HAVETYPE   0x00000001 /* type is valid */
#define KMMF_HAVELAYOUT 0x00000002 /* layout is valid */
#define KMMF_GETID      0x00000004 /* saw KBD_RESET, expecting ID */
#define KMMF_GETLAYOUT  0x00000008 /* saw KBD_LAYOUT, expecting layout */
#define KMMF_CTRLDOWN   0x00000010 /* control key is down */
#define KMMF_SWITCHING  0x00000020 /* ctrl-f1 pressed, switching */
#define KMMF_SWITCHED   0x00000040 /* switched, waiting for idle */
#define KMMF_WANTRESET  0x00000080 /* attached keyboard wants reset */
#define KMMF_WANTLAYOUT 0x00000100 /* attached keyboard wants layout */
#define KMMF_MULTI      0x00000200 /* multi-digit switch mode */
#define KMMF_SWITCH_K   0x00000400 /* switch just keyboard */
#define KMMF_SWITCH_M   0x00000800 /* switch just mouse */
  unsigned char type;
  unsigned char layout;
  int bell;
  int mswitchn;
  } ;

static MUX mux[NMUX];

extern struct cfdriver kmmux_cd;

extern struct cfattach kmmux_zs_ca;
extern struct cfattach kbd_kmmux_ca;

static void *km_alloc(int nb, const char *pstr)
{
 void *mem;

 mem = malloc(nb,M_DEVBUF,M_NOWAIT);
 if (mem == 0) panic(pstr);
 return(mem);
}

static void km_free(void *buf)
{
 free(buf,M_DEVBUF);
}

static void queue_output(SOFTC *km, unsigned char c)
{
 int put;

 put = km->tput;
 km->tring[put] = c;
 KMMUX_TRINGINC(put);
 if (put == km->tget)
  { log(LOG_WARNING,"%s: output overrun\n",km->dev.dv_xname);
  }
 else
  { km->tput = put;
  }
}

static void start_output(SOFTC *km)
{
 int get;
 unsigned char c;
 int s;

 if (km->swflags & KMSF_TX_BUSY) return;
 get = km->tget;
 if (get == km->tput) return;
 c = km->tring[get];
 KMMUX_TRINGINC(get);
 km->tget = get;
 km->swflags |= KMSF_TX_BUSY;
 s = splzs();
 zs_write_data(km->zcs,c);
 splx(s);
}

static void setbell(MUX *m)
{
 int s;
 PAIR *p;

 if (!m || !m->kbd) return;
 if (m->bell < 0)
  { printf("kmmux: %p->bell=%d\n",(void *)m,m->bell);
    m->bell = 0;
  }
 p = (m->kcur < m->nkids) ? m->kids[m->kcur] : 0;
 s = splzs();
 queue_output(m->kbd,m->bell?KBD_CMD_BELL:KBD_CMD_NOBELL);
 start_output(m->kbd);
 splx(s);
}

static void setclick(MUX *m)
{
 int s;
 PAIR *p;

 if (!m || !m->kbd) return;
 p = (m->kcur < m->nkids) ? m->kids[m->kcur] : 0;
 s = splzs();
 queue_output(m->kbd,(p&&p->click)?KBD_CMD_CLICK:KBD_CMD_NOCLICK);
 start_output(m->kbd);
 splx(s);
}

static void setleds(MUX *m)
{
 int s;
 PAIR *p;

 if (!m || !m->kbd) return;
 p = (m->kcur < m->nkids) ? m->kids[m->kcur] : 0;
 s = splzs();
 if (p)
  { queue_output(m->kbd,KBD_CMD_SETLED);
    queue_output(m->kbd,p->leds);
    start_output(m->kbd);
  }
 splx(s);
}

static void maybe_setleds(MUX *m)
{
 if ((m->flags & KMMF_HAVETYPE) && (m->type > 3)) setleds(m);
}

static void stopbell(void *mv)
{
 MUX *m;

 m = mv;
 m->bell --;
 setbell(m);
}

static void switchto(MUX *m, int unit)
{
 PAIR *cur;
 int s;

 if (! (m->flags & KMMF_SWITCH_M))
  { cur = m->kids[m->kcur];
    if (m->flags & KMMF_CTRLDOWN)
     { if (cur->kbd) kbd_input_raw(cur->kbd,KEY_CTRL|KEY__UP);
       m->flags &= ~KMMF_CTRLDOWN;
     }
  }
 m->flags = (m->flags & ~(KMMF_SWITCHING|KMMF_MULTI)) | KMMF_SWITCHED;
 if ((unit < 0) || (unit >= m->nkids))
  { m->bell ++;
    setbell(m);
    timeout(stopbell,m,hz/5);
    return;
  }
 if ( ((m->flags&KMMF_SWITCH_M) || (unit == m->kcur)) &&
      ((m->flags&KMMF_SWITCH_K) || (unit == m->mcur)) ) return;
 s = spltty();
 if (!(m->flags&KMMF_SWITCH_M) && (unit != m->kcur))
  { m->kcur = unit;
    maybe_setleds(m);
    setclick(m);
    setbell(m);
  }
 if (!(m->flags&KMMF_SWITCH_K) && (unit != m->mcur))
  { m->mcur = unit;
  }
 splx(s);
}

static void for_flagged_pairs(MUX *m, unsigned int bit, void (*fn)(PAIR *, unsigned int), unsigned char arg)
{
 int i;
 PAIR *p;

 for (i=0;i<m->nkids;i++)
  { p = m->kids[i];
    if (p->flags & bit) (*fn)(p,arg);
  }
}

static void send_chr(PAIR *p, unsigned int c)
{
 kbd_input_raw(p->kbd,c);
}

static void clear_bit(PAIR *p, unsigned int bit)
{
 p->flags &= ~bit;
}

static void kmmux_switch_speed(SOFTC *km)
{
 int s;

 switch (km->baud)
  { default:
       km->baud = 9600;
       break;
    case 2400: case 4800: case 9600:
       km->baud >>= 1;
       break;
  }
 s = splzs();
 zs_set_speed(km->zcs,km->baud);
 zs_loadchannelregs(km->zcs);
 splx(s);
 /* normally would spltty and do km->rget=km->rput here, but
    we're called only from kmmux_zs_softint, already at spltty and
    with the equivalent of that assignment being done soon. */
 log(LOG_WARNING,"%s: trying %d baud\n",km->dev.dv_xname,km->baud);
}

static void kmmux_input_raw(SOFTC *km, unsigned char c)
{
 MUX *m;
 PAIR *p;

 m = km->mux;
 if (m == 0) return;
 if (MOUSE_HALF(km))
  { if (m->mcur >= m->nkids) return;
    p = m->kids[m->mcur];
    if (p->ms) ms_input(p->ms,c);
  }
 else
  { if (m->kcur >= m->nkids) return;
    p = m->kids[m->kcur];
    if (m->flags & KMMF_GETID)
     { m->type = c;
       m->flags = (m->flags & ~KMMF_GETID) | KMMF_HAVETYPE;
       if (m->flags & KMMF_WANTRESET)
	{ for_flagged_pairs(m,KMPF_WANTRESET,send_chr,KBD_RESET);
	  for_flagged_pairs(m,KMPF_WANTRESET,send_chr,c);
	  for_flagged_pairs(m,KMPF_WANTRESET,clear_bit,KMPF_WANTRESET);
	  m->flags &= ~KMMF_WANTRESET;
	}
       maybe_setleds(m);
       setbell(m);
       setclick(m);
       return;
     }
    if (m->flags & KMMF_GETLAYOUT)
     { m->layout = c;
       m->flags = (m->flags & ~KMMF_GETLAYOUT) | KMMF_HAVELAYOUT;
       if (m->flags & KMMF_WANTLAYOUT)
	{ for_flagged_pairs(m,KMPF_WANTLAYOUT,send_chr,KBD_LAYOUT);
	  for_flagged_pairs(m,KMPF_WANTLAYOUT,send_chr,c);
	  for_flagged_pairs(m,KMPF_WANTLAYOUT,clear_bit,KMPF_WANTLAYOUT);
	  m->flags &= ~KMMF_WANTLAYOUT;
	}
       return;
     }
    switch (c)
     { case KBD_RESET:
	  m->flags |= KMMF_GETID;
	  return;
	  break;
       case KBD_LAYOUT:
	  m->flags |= KMMF_GETLAYOUT;
	  return;
	  break;
       case KBD_ERROR:
	  printf("%s: keyboard reports error\n",km->dev.dv_xname);
	  return;
	  break;
     }
    if (m->flags & KMMF_SWITCHING)
     { if ((m->flags & KMMF_CTRLDOWN) && (c == (KEY_CTRL|KEY__UP)))
	{ if (p->kbd) kbd_input_raw(p->kbd,KEY_CTRL|KEY__UP);
	  m->flags &= ~KMMF_CTRLDOWN;
	}
       if ((c & KEY__UP) || (c == KBD_IDLE)) return;
       if (m->flags & KMMF_MULTI)
	{ switch (c)
	   { case KEY_1: m->mswitchn = (10 * m->mswitchn) + 1; break;
	     case KEY_2: m->mswitchn = (10 * m->mswitchn) + 2; break;
	     case KEY_3: m->mswitchn = (10 * m->mswitchn) + 3; break;
	     case KEY_4: m->mswitchn = (10 * m->mswitchn) + 4; break;
	     case KEY_5: m->mswitchn = (10 * m->mswitchn) + 5; break;
	     case KEY_6: m->mswitchn = (10 * m->mswitchn) + 6; break;
	     case KEY_7: m->mswitchn = (10 * m->mswitchn) + 7; break;
	     case KEY_8: m->mswitchn = (10 * m->mswitchn) + 8; break;
	     case KEY_9: m->mswitchn = (10 * m->mswitchn) + 9; break;
	     case KEY_0: m->mswitchn =  10 * m->mswitchn     ; break;
	     case KEY_k: m->flags = (m->flags & ~KMMF_SWITCH_M) | KMMF_SWITCH_K; break;
	     case KEY_m: m->flags = (m->flags & ~KMMF_SWITCH_K) | KMMF_SWITCH_M; break;
	     default: switchto(m,-1); break;
	     case KEY_SPACE: switchto(m,m->mswitchn-1); break;
	   }
	}
       else
	{ switch (c)
	   { case KEY_F1:
		if (m->flags & KMMF_CTRLDOWN)
		 { if (p->kbd) kbd_input_raw(p->kbd,KEY_F1);
		   m->flags &= ~KMMF_SWITCHING;
		 }
		break;
	     case KEY_1: switchto(m,0); break;
	     case KEY_2: switchto(m,1); break;
	     case KEY_3: switchto(m,2); break;
	     case KEY_4: switchto(m,3); break;
	     case KEY_5: switchto(m,4); break;
	     case KEY_6: switchto(m,5); break;
	     case KEY_7: switchto(m,6); break;
	     case KEY_8: switchto(m,7); break;
	     case KEY_9: switchto(m,8); break;
	     case KEY_0: switchto(m,9); break;
	     case KEY_k: m->flags = (m->flags & ~KMMF_SWITCH_M) | KMMF_SWITCH_K; break;
	     case KEY_m: m->flags = (m->flags & ~KMMF_SWITCH_K) | KMMF_SWITCH_M; break;
	     default: switchto(m,-1); break;
	     case KEY_SPACE:
		m->flags |= KMMF_MULTI;
		m->mswitchn = 0;
		break;
	     case KEY_ESC:
		m->flags = (m->flags & ~KMMF_SWITCHING) | KMMF_SWITCHED;
		m->bell = 0;
		setbell(m);
		break;
	   }
	}
       return;
     }
    if (m->flags & KMMF_SWITCHED)
     { if (c == KBD_IDLE) m->flags &= ~KMMF_SWITCHED;
       return;
     }
    switch (c)
     { case KBD_RESET:
	  m->flags |= KMMF_GETID;
	  break;
       case KBD_LAYOUT:
	  m->flags |= KMMF_GETLAYOUT;
	  break;
       case KBD_ERROR:
	  printf("%s: keyboard reports error\n",km->dev.dv_xname);
	  break;
       default:
	  switch (c)
	   { case KEY_CTRL:
		m->flags |= KMMF_CTRLDOWN;
		break;
	     case KEY_CTRL|KEY__UP:
	     case KEY_IDLE:
		m->flags &= ~KMMF_CTRLDOWN;
		break;
	     case KEY_F1:
		if (m->flags & KMMF_CTRLDOWN)
		 { m->flags = (m->flags & ~(KMMF_SWITCH_M|KMMF_SWITCH_K)) | KMMF_SWITCHING;
		   return;
		 }
		break;
	   }
	  if (p->kbd) kbd_input_raw(p->kbd,c);
	  break;
     }
  }
}

static void reply_reset(void *kv)
{
 struct kbd_softc *k;

 k = kv;
 kbd_input_raw(k,KBD_RESET);
 kbd_input_raw(k,((PAIR *)k->k_private)->mux->type);
 kbd_input_raw(k,KBD_IDLE);
}

static void reply_layout(void *kv)
{
 struct kbd_softc *k;

 k = kv;
 kbd_input_raw(k,KBD_LAYOUT);
 kbd_input_raw(k,((PAIR *)k->k_private)->mux->layout);
}

/* The reason we use timeout() rather than inlining the functionality of
   reply_reset and reply_layout is that kbd driver calls us at spltty(),
   and assumes that kbd_input_raw cannot be called at this point because
   of that.  (It does start_tx(), drain_tx(), tsleep(), assuming the wakeup()
   won't happen until after tsleep() drops IPL.)  So we schedule them for
   "later", off the softclock. */
static void kmmux_kbd_write_data_(struct kbd_softc *k, int c)
{
 SOFTC *km;
 MUX *m;
 PAIR *kp;
 PAIR *cp;

 kp = k->k_private;
 m = kp->mux;
 km = m->kbd;
 if (m->kcur >= m->nkids) return;
 cp = m->kids[m->kcur];
 if (kp->flags & KMPF_SETLEDS)
  { kp->leds = c;
    setleds(m);
    kp->flags &= ~KMPF_SETLEDS;
    return;
  }
 switch (c)
  { case KBD_CMD_RESET:
       if ((cp == kp) || !(m->flags & KMMF_HAVETYPE))
	{ queue_output(km,KBD_CMD_RESET);
	  start_output(km);
	  m->flags |= KMMF_WANTRESET;
	  kp->flags |= KMPF_WANTRESET;
	}
       else
	{ timeout(reply_reset,k,0);
	}
       break;
    case KBD_CMD_BELL:
       if (! kp->bell)
	{ kp->bell = 1;
	  m->bell ++;
	}
       setbell(m);
       break;
    case KBD_CMD_NOBELL:
       if (kp->bell)
	{ kp->bell = 0;
	  m->bell --;
	}
       setbell(m);
       break;
    case KBD_CMD_CLICK:
       kp->click = 1;
       setclick(m);
       break;
    case KBD_CMD_NOCLICK:
       kp->click = 0;
       setclick(m);
       break;
    case KBD_CMD_SETLED:
       kp->flags |= KMPF_SETLEDS;
       break;
    case KBD_CMD_GETLAYOUT:
       if ((cp == kp) || !(m->flags & KMMF_HAVELAYOUT))
	{ queue_output(km,KBD_CMD_GETLAYOUT);
	  start_output(km);
	  m->flags |= KMMF_WANTLAYOUT;
	  kp->flags |= KMPF_WANTLAYOUT;
	}
       else
	{ timeout(reply_layout,k,0);
	}
       break;
  }
}

static void kmmux_kbd_write_data(struct kbd_softc *k, int c)
{
 kmmux_kbd_write_data_(k,c);
 k->k_txflags &= ~K_TXBUSY;
 kbd_start_tx(k);
}

static int findkid(MUX *m, int bond, int create)
{
 int i;

 for (i=m->nkids-1;i>=0;i--) if (m->kids[i]->bond == bond) return(i);
 if (create)
  { PAIR **newk;
    PAIR *p;
    p = km_alloc(sizeof(PAIR),"kmmux alloc bonded pair");
    i = m->nkids++;
    newk = km_alloc((i+1)*sizeof(PAIR *),"kmmux alloc bonded pair vector");
    if (m->kids)
     { bcopy(m->kids,newk,i*sizeof(PAIR *));
       km_free(m->kids);
     }
    m->kids = newk;
    newk[i] = p;
    p->kbd = 0;
    p->ms = 0;
    p->bond = bond;
    p->click = 0;
    p->leds = 0;
    p->bell = 0;
    p->mux = m;
    p->kid = i;
  }
 return(i);
}

static void kmmux_setup_kbd(struct kbd_softc *k)
{
 MUX *m;
 int i;
 int bond;

 m = ((SOFTC *)k->k_dev.dv_parent)->mux;
 if (m == 0) panic("attaching to unconfigured kmmux");
 bond = k->k_dev.dv_cfdata->cf_loc[KMMUXCF_BOND];
 if (bond < 0) bond = k->k_dev.dv_unit;
 i = findkid(m,bond,1);
 if (m->kids[i]->kbd) panic("kmmux duplicate keyboard attach");
 k->k_private = m->kids[i];
 m->kids[i]->kbd = k;
 if (k->k_isconsole)
  { m->kcur = i;
    m->mcur = i;
  }
}

static void kmmux_setup_ms(struct ms_softc *ms)
{
 MUX *m;
 int i;
 int bond;

 m = ((SOFTC *)ms->ms_dev.dv_parent)->mux;
 if (m == 0) panic("attaching to unconfigured kmmux");
 bond = ms->ms_dev.dv_cfdata->cf_loc[KMMUXCF_BOND];
 if (bond < 0) bond = ms->ms_dev.dv_unit;
 i = findkid(m,bond,1);
 if (m->kids[i]->ms) panic("kmmux duplicate mouse attach");
 ms->ms_private = m->kids[i];
 m->kids[i]->ms = ms;
}

static int already_bonded(SOFTC *km, int bond)
{
 int i;
 MUX *m;

 m = km->mux;
 if (m == 0) panic("matching child of unconfigured kmmux");
 for (i=0;i<m->nkids;i++)
  { if (m->kids[i]->bond != bond) continue;
    if (  MOUSE_HALF(km)
	? (m->kids[i]->ms != 0)
	: (m->kids[i]->kbd != 0) ) return(1);
  }
 return(0);
}

static void kmmux_reset_real(SOFTC *km)
{
 MUX *m;

 m = km->mux;
 queue_output(km,KBD_CMD_RESET);
 start_output(km);
}

static void end_holdown(void *kmv)
{
 SOFTC *km;
 int s;

 km = kmv;
 s = spltty();
 km->swflags &= ~KMSF_HOLDOWN;
 splx(s);
}

static void kmmux_zs_softint(struct zs_chanstate *zcs)
{
 SOFTC *km;
 unsigned int intf;
 int s;
 int get;
 unsigned short int val;

 km = zcs->cs_private;
 s = splzs();
 intf = km->intflags;
 km->intflags = 0;
 splx(s);
 s = spltty();
 get = km->rget;
 while (get != km->rput)
  { val = km->rring[get];
    KMMUX_RRINGINC(get);
    if (val & ZSRR1_DO) intf |= KMIF_OVERRUN;
    if (val & (ZSRR1_FE|ZSRR1_PE))
     { log(LOG_ERR,"%s: input error (0x%x)\n",km->dev.dv_xname,val);
       get = km->rput;
       goto haderr;
     }
    kmmux_input_raw(km,val>>8);
  }
 if (intf & KMIF_OVERRUN)
  { log(LOG_ERR,"%s: input overrun\n",km->dev.dv_xname);
haderr:;
    if (MOUSE_HALF(km))
     { if (! (km->swflags & KMSF_HOLDOWN))
	{ km->swflags |= KMSF_HOLDOWN;
	  kmmux_switch_speed(km);
	  timeout(end_holdown,km,2);
	}
     }
    else
     { kmmux_reset_real(km);
     }
  }
 km->rget = get;
 if (intf & KMIF_TXEMPTY)
  { km->swflags &= ~KMSF_TX_BUSY;
    start_output(km);
  }
 if (intf & KMIF_STATCHG)
  { log(LOG_ERR,"%s: status interrupt\n",km->dev.dv_xname);
    zcs->cs_rr0_delta = 0;
  }
 splx(s);
}

static void kmmux_zs_rxint(struct zs_chanstate *zcs)
{
 SOFTC *km;
 unsigned char rr1;
 unsigned char c;
 unsigned int put;

 km = zcs->cs_private;
 rr1 = zs_read_reg(zcs,1);
 c = zs_read_data(zcs);
 if (rr1 & (ZSRR1_FE|ZSRR1_DO|ZSRR1_PE))
  { zs_write_csr(zcs,ZSWR0_RESET_ERRORS);
  }
 if (KBD_HALF(km) && (km->swflags & KMSF_CONSOLE))
  { if (km->swflags & KMSF_ABORT_1)
     { if (c == 77) /* A down */
	{ zs_abort(zcs);
	  c = 129; /* fake L1 up */
	}
       km->swflags &= ~KMSF_ABORT_1;
     }
    else
     { if (c == 1) km->swflags |= KMSF_ABORT_1; /* L1 down */
     }
  }
 put = km->rput;
 km->rring[put] = (c << 8) | rr1;
 KMMUX_RRINGINC(put);
 if (put == km->rget)
  { km->intflags |= KMIF_OVERRUN;
  }
 else
  { km->rput = put;
  }
 zcs->cs_softreq = 1;
}

static void kmmux_zs_txint(struct zs_chanstate *zcs)
{
 zs_write_csr(zcs,ZSWR0_RESET_TXINT);
 ((SOFTC *)zcs->cs_private)->intflags |= KMIF_TXEMPTY;
 zcs->cs_softreq = 1;
}

static void kmmux_zs_stint(struct zs_chanstate *zcs, int force)
{
 unsigned char rr0;
 SOFTC *km;

 km = zcs->cs_private;
 rr0 = zs_read_csr(zcs);
 zs_write_csr(zcs,ZSWR0_RESET_STATUS);
 zcs->cs_rr0_delta |= zcs->cs_rr0 ^ rr0;
 zcs->cs_rr0 = rr0;
 km->intflags |= KMIF_STATCHG;
 zcs->cs_softreq = 1;
}

static struct zsops zsops_kmmux = { kmmux_zs_rxint,
				    kmmux_zs_stint,
				    kmmux_zs_txint,
				    kmmux_zs_softint };

static int kmmux_match_zs(struct device *parent, struct cfdata *cf, void *aux)
{
 struct zsc_attach_args *aa;

 aa = aux;
 if ((cf->cf_unit < 0) || (cf->cf_unit >= NMUX*2)) return(0);
 if (cf->cf_loc[ZSCCF_CHANNEL] == aa->channel) return(2);
 return(0);
}

static int kmmux_print(void *aux, const char *name)
{
 return(QUIET);
}

static void kmmux_attach_zs(struct device *parent, struct device *self, void *aux)
{
 struct zsc_softc *zsc;
 SOFTC *km;
 struct zsc_attach_args *aa;
 struct zs_chanstate *zcs;
 struct kmmux_attach_args subaa;
 int baud;
 MUX *m;
 int s;
 const char *pref;

 zsc = (void *) parent;
 km = (void *) self;
 aa = aux;
 if ((self->dv_unit < 0) || (self->dv_unit >= NKMMUX)) panic("kmmux_attach_zs bad unit");
 m = &mux[self->dv_unit>>1];
 if (MOUSE_UNIT(self->dv_unit)) m->ms = km; else m->kbd = km;
 zcs = zsc->zsc_cs[aa->channel];
 zcs->cs_private = km;
 zcs->cs_ops = &zsops_kmmux;
 km->swflags = 0;
 km->intflags = 0;
 km->zcs = zcs;
 km->mux = m;
 km->rput = 0;
 km->rget = 0;
 pref = " (";
 if (aa->hwflags & ZS_HWFLAG_CONSOLE)
  { if (MOUSE_UNIT(self->dv_unit))
     { printf(" (console, but mouse unit");
     }
    else
     { km->swflags |= KMSF_CONSOLE;
       printf(" (console");
     }
    pref = ", ";
  }
 baud = self->dv_cfdata->cf_flags;
 if (baud == 0) baud = 1200;
 printf("%s%d baud)\n",pref,baud);
 km->baud = baud;
 s = splzs();
 if (! (km->swflags & KMSF_CONSOLE))
  { zs_write_reg(zcs,9,aa->channel?ZSWR9_B_RESET:ZSWR9_A_RESET);
  }
 zcs->cs_preg[1] = ZSWR1_RIE | ZSWR1_TIE;
 zs_set_speed(zcs,baud);
 zs_loadchannelregs(zcs);
 splx(s);
 subaa.console = (km->swflags & KMSF_CONSOLE) ? 1 : 0;
 while (config_found(self,&subaa,kmmux_print)) ;
}

static int kbd_match_kmmux(struct device *parent, struct cfdata *cf, void *aux)
{
 int bond;

 if (MOUSE_UNIT(parent->dv_unit)) return(0);
 bond = cf->cf_loc[KMMUXCF_BOND];
 if (bond < 0) bond = cf->cf_unit;
 if (already_bonded((void *)parent,bond)) return(0);
 return(1);
}

static void kbd_attach_kmmux(struct device *parent, struct device *self, void *aux)
{
 struct kmmux_softc *km;
 struct kbd_softc *k;
 struct kmmux_attach_args *aa;

 km = (void *) parent;
 k = (void *) self;
 aa = aux;
 k->k_write_data = kmmux_kbd_write_data;
 if (aa->console)
  { k->k_isconsole = 1;
    aa->console = 0;
    printf(" (console)");
  }
 printf("\n");
 kbd_xlate_init(&k->k_state);
 k->k_repeat_start = hz/2;
 k->k_repeat_step = hz/20;
 kd_init(k->k_dev.dv_unit);
 kmmux_setup_kbd(k);
}

static int ms_match_kmmux(struct device *parent, struct cfdata *cf, void *aux)
{
 int bond;

 if (KBD_UNIT(parent->dv_unit)) return(0);
 bond = cf->cf_loc[KMMUXCF_BOND];
 if (bond < 0) bond = cf->cf_unit;
 if (already_bonded((void *)parent,bond)) return(0);
 return(1);
}

static void ms_attach_kmmux(struct device *parent, struct device *self, void *aux)
{
 struct kmmux_softc *km;
 struct ms_softc *m;
 struct kmmux_attach_args *aa;

 km = (void *) parent;
 m = (void *) self;
 aa = aux;
 printf("\n");
 m->ms_byteno = -1;
 kmmux_setup_ms(m);
}

struct cfattach kmmux_zs_ca = { sizeof(SOFTC),
				kmmux_match_zs,
				kmmux_attach_zs };

struct cfattach kbd_kmmux_ca = { sizeof(struct kbd_softc),
				 kbd_match_kmmux,
				 kbd_attach_kmmux };

struct cfattach ms_kmmux_ca = { sizeof(struct ms_softc),
				ms_match_kmmux,
				ms_attach_kmmux };
