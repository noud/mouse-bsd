/* $NetBSD: nextkbd.c,v 1.3 1999/03/26 04:17:46 dbj Exp $ */
/*
 * Copyright (c) 1998 Matt DeBergalis
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
 *      This product includes software developed by Matt DeBergalis
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/ttycom.h>
#include <sys/signalvar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <next68k/dev/nextkbdvar.h>
#include <next68k/dev/wskbdmap_next.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <next68k/next68k/isr.h>

struct nextkbd_internal {
	int num_ints; /* interrupt total */
	int polling;
	int isconsole;

	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct nextkbd_softc *t_sc; /* back pointer */
	u_int32_t mods;
};

struct mon_regs {
	u_int32_t mon_csr;
	u_int32_t mon_1;
	u_int32_t mon_data;
};

int nextkbd_match __P((struct device *, struct cfdata *, void *));
void nextkbd_attach __P((struct device *, struct device *, void *));

int nextkbc_cnattach __P((bus_space_tag_t));

struct cfattach nextkbd_ca = {
	sizeof(struct nextkbd_softc), nextkbd_match, nextkbd_attach
};

int	nextkbd_enable __P((void *, int));
void	nextkbd_set_leds __P((void *, int));
int	nextkbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

const struct wskbd_accessops nextkbd_accessops = {
	nextkbd_enable,
	nextkbd_set_leds,
	nextkbd_ioctl,
};

void	nextkbd_cngetc __P((void *, u_int *, int *));
void	nextkbd_cnpollc __P((void *, int));

const struct wskbd_consops nextkbd_consops = {
	nextkbd_cngetc,
	nextkbd_cnpollc,
};

const struct wskbd_mapdata nextkbd_keymapdata = {
	nextkbd_keydesctab,
	KB_US,
};

static int nextkbd_read_data __P((struct nextkbd_internal *));
static int nextkbd_decode __P((struct nextkbd_internal *, int, u_int *, int *));

static struct nextkbd_internal nextkbd_consdata;
static int nextkbd_is_console __P((bus_space_tag_t bst));

#define NK_RINGSIZE 1024
#define NK_RINGMASK 1023
static volatile unsigned int nk_ring[NK_RINGSIZE];
static volatile int nk_ring_h = 0;
static volatile int nk_ring_t = 0;
#define NK_RING_ADV(x) (((x)+1)&NK_RINGMASK)
static volatile int nk_ring_waiting = 0;
static char nk_wchan;
static struct selinfo nk_selr;
static int nk_flags = 0;
#define NK_F_ISOPEN 0x00000001
#define NK_F_ASYNC  0x00000002
#define NK_F_NBLOCK 0x00000004
static int nk_pgrp = 0;

int nextkbdhard __P((void *));

#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/select.h>
cdev_decl(nk_);

int nk_open(dev_t dev, int flag, int mode, struct proc *p)
{
 int s;

 if (minor(dev) != 0) return(ENXIO);
 s = splhigh();
 if (! (nk_flags & NK_F_ISOPEN))
  { nk_flags |= NK_F_ISOPEN;
    nk_flags &= ~NK_F_ASYNC;
    nk_pgrp = 0;
    /* It's arguably wrong to ignore NONBLOCK on non-first opens, but it's
       also arguably wrong to have the latest open's setting override all
       previous opens' settings. */
    if (flag & FNONBLOCK) nk_flags |= NK_F_NBLOCK; else nk_flags &= ~NK_F_NBLOCK;
  }
 splx(s);
 return(0);
}

int nk_close(dev_t dev, int flag, int mode, struct proc *p)
{
 nk_flags = 0;
 nk_pgrp = 0;
 return(0);
}

int nk_read(dev_t dev, struct uio *uio, int ioflag)
{
 int s;
 int err;
 int n;
 unsigned int buf[32];
 int f;
 int h;
 int t;

 n = uio->uio_resid;
 if (n < 1) return(0);
 if (n < sizeof(buf[0])) return(EIO);
 n /= sizeof(buf[0]);
 if (n > 32) n = 32;
 s = splhigh();
 while (1)
  { h = nk_ring_h;
    t = nk_ring_t;
    if (h != t) break;
    if (nk_flags & NK_F_NBLOCK)
     { splx(s);
       return(EWOULDBLOCK);
     }
    nk_ring_waiting = 1;
    err = tsleep(&nk_wchan,PZERO|PCATCH,"nk_read",0);
    if (err)
     { splx(s);
       return(err);
     }
  }
 for (f=0;(f<n)&&(t!=h);f++)
  { buf[f] = nk_ring[t];
    t = NK_RING_ADV(t);
  }
 nk_ring_t = t;
 splx(s);
 return(uiomove(&buf[0],f*sizeof(buf[0]),uio));
}

int nk_write(dev_t dev, struct uio *uio, int ioflag)
{
 return(EIO);
}

int nk_ioctl(dev_t dev, u_long cmd, caddr_t arg, int flag, struct proc *p)
{
 switch (cmd)
  { case FIONBIO:
       if (*(int *)arg) nk_flags |= NK_F_NBLOCK; else nk_flags &= ~NK_F_NBLOCK;
       return(0);
       break;
    case FIOASYNC:
       if (*(int *)arg) nk_flags |= NK_F_ASYNC; else nk_flags &= ~NK_F_ASYNC;
       return(0);
       break;
    case TIOCGPGRP:
       *(int *)arg = nk_pgrp;
       return(0);
       break;
    case TIOCSPGRP:
	{ int v;
	  struct pgrp *pg;
	  v = *(int *)arg;
	  if (v == 0)
	   { nk_pgrp = 0;
	   }
	  else
	   { pg = pgfind(v);
	     if (pg != p->p_pgrp) return(EPERM);
	     nk_pgrp = v;
	   }
	  return(0);
	}
       break;
  }
 return(ENOTTY);
}

int nk_poll(dev_t dev, int events, struct proc *p)
{
 int s;
 int can;
 int revents;

 revents = 0;
 if (events & (POLLIN|POLLRDNORM))
  { s = splhigh();
    can = (nk_ring_h != nk_ring_t);
    splx(s);
    if (can)
     { revents |= events & (POLLIN | POLLRDNORM);
     }
    else
     { selrecord(p,&nk_selr);
     }
  }
 return(revents);
}

int nk_mmap(dev_t dev, int off, int prot)
{
 return(-1);
}

/*
 * There is some risk of gsignal()ing a pgrp unrelated to any that's
 *  got the nk driver open, despite the checks above in nk_ioctl().  If
 *  a process sets the pgrp, then all processes in that pgrp die (with
 *  the open fd preserved either by forking and leaving the pgrp or by
 *  passing it to another process, eg through an AF_LOCAL socket), the
 *  pgrp is later reused, and an event then arrives, we will gsignal()
 *  unrelated processes.  However, AFAICT there is no way to avoid
 *  this, and it's basically what the rest of the places that do SIGIO
 *  generation do.
 */
static int nk_data(unsigned int val)
{
 int s;
 int h;
 int h2;

 if (! (nk_flags & NK_F_ISOPEN)) return(1);
 s = splhigh();
 h = nk_ring_h;
 h2 = NK_RING_ADV(h);
 if (h2 != nk_ring_t)
  { nk_ring[h] = val;
    nk_ring_h = h2;
    if (nk_ring_waiting)
     { nk_ring_waiting = 0;
       wakeup(&nk_wchan);
     }
    selwakeup(&nk_selr);
    if ((nk_flags & NK_F_ASYNC) && (nk_pgrp != 0)) gsignal(nk_pgrp,SIGIO);
  }
 splx(s);
 return(0);
}

static int
nextkbd_is_console(bst)
	bus_space_tag_t bst;
{
	return (nextkbd_consdata.isconsole
			&& (bst == nextkbd_consdata.iot));
}

int
nextkbd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

void
nextkbd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct nextkbd_softc *sc = (struct nextkbd_softc *)self;
	int isconsole;
	struct wskbddev_attach_args a;

	printf("\n");

	isconsole = nextkbd_is_console(NEXT68K_INTIO_BUS_SPACE); /* XXX */

	if (isconsole) {
		sc->id = &nextkbd_consdata;
	} else {
		sc->id = malloc(sizeof(struct nextkbd_internal),
				M_DEVBUF, M_WAITOK);

		memset(sc->id, 0, sizeof(struct nextkbd_internal));
		sc->id->iot = NEXT68K_INTIO_BUS_SPACE;
		if (bus_space_map(sc->id->iot, NEXT_P_MON,
				sizeof(struct mon_regs),
				0, &sc->id->ioh)) {
			printf("%s: can't map mon status control register\n",
					sc->sc_dev.dv_xname);
			return;
		}
	}

	sc->id->t_sc = sc; /* set back pointer */

	isrlink_autovec(nextkbdhard, sc, NEXT_I_IPL(NEXT_I_KYBD_MOUSE), 0);

	INTR_ENABLE(NEXT_I_KYBD_MOUSE);

	a.console = isconsole;
	a.keymap = &nextkbd_keymapdata;
	a.accessops = &nextkbd_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wskbd, saving a handle to it.
	 * XXX XXX XXX
	 */
	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}

int
nextkbd_enable(v, on)
	void *v;
	int on;
{
	/* XXX not sure if this should do anything */
	/* printf("nextkbd_enable %d\n", on); */
	return 0;
}

/* XXX not yet implemented */
void
nextkbd_set_leds(v, leds)
	void *v;
	int leds;
{
	return;
}

int
nextkbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	/* XXX struct nextkbd_softc *nc = v; */

	switch (cmd) {
	case WSKBDIO_GTYPE:
		/* XXX */
		*(int *)data = WSKBD_TYPE_NEXT;
		return (0);
	case WSKBDIO_SETLEDS:
		return (0);
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return (0);
	case WSKBDIO_COMPLEXBELL:
		return (0);
	}
	return -1;
}

int
nextkbdhard(arg)
	void *arg;
{
	register struct nextkbd_softc *sc = arg;
	int type, key;
	unsigned int val;

	if (!INTR_OCCURRED(NEXT_I_KYBD_MOUSE)) return 0;

#define CSR_INT 0x00800000
#define CSR_DATA 0x00400000

#define KD_KEYMASK			0x007f
#define KD_DIRECTION		0x0080 /* pressed or released */
#define KD_CNTL					0x0100
#define KD_LSHIFT				0x0200
#define KD_RSHIFT				0x0400
#define KD_LCOMM				0x0800
#define KD_RCOMM				0x1000
#define KD_LALT					0x2000
#define KD_RALT					0x4000
#define KD_VALID				0x8000 /* only set for scancode keys ? */
#define KD_MODS					0x4f00

	val = nextkbd_read_data(sc->id);
	if ( nk_data(val) &&
	     ((val >> 28) == 1) &&
	     nextkbd_decode(sc->id, val&0xffff, &type, &key) ) {
		wskbd_input(sc->sc_wskbddev, type, key);
	}
	return(1);
}

int
nextkbd_cnattach(bst)
	bus_space_tag_t bst;
{
	bus_space_handle_t bsh;

	if (bus_space_map(bst, NEXT_P_MON, sizeof(struct mon_regs),
			0, &bsh))
		return (ENXIO);

	memset(&nextkbd_consdata, 0, sizeof(nextkbd_consdata));

	nextkbd_consdata.iot = bst;
	nextkbd_consdata.ioh = bsh;
	nextkbd_consdata.isconsole = 1;

	wskbd_cnattach(&nextkbd_consops, &nextkbd_consdata,
			&nextkbd_keymapdata);

	return (0);
}

void
nextkbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	struct nextkbd_internal *t = v;
	unsigned int val;

	for (;;) {
		if (INTR_OCCURRED(NEXT_I_KYBD_MOUSE)) {
			val = nextkbd_read_data(t);
			if (((val >> 28) == 1) && nextkbd_decode(t, val&0xffff, type, data))
				return;
		}
	}
}

void
nextkbd_cnpollc(v, on)
	void *v;
	int on;
{
	struct nextkbd_internal *t = v;

	t->polling = on;
	if (on) {
		INTR_DISABLE(NEXT_I_KYBD_MOUSE);
	} else {
		INTR_ENABLE(NEXT_I_KYBD_MOUSE);
	}

}

static int
nextkbd_read_data(struct nextkbd_internal *id)
{
	struct mon_regs stat;

	bus_space_read_region_4(id->iot, id->ioh, 0, &stat, 3);
	if ((stat.mon_csr & CSR_INT) && (stat.mon_csr & CSR_DATA)) {
		stat.mon_csr &= ~CSR_INT;
		id->num_ints++;
		bus_space_write_4(id->iot, id->ioh, 0, stat.mon_csr);
		return(stat.mon_data);
	}
	return (-1);
}

static int
nextkbd_decode(id, datain, type, dataout)
	struct nextkbd_internal *id;
	int datain;
	u_int *type;
	int *dataout;
{
	/* printf("datain %08x mods %08x\n", datain, id->mods); */

	if ((datain ^ id->mods) & KD_LSHIFT) {
		id->mods ^= KD_LSHIFT;
		*dataout = 90;
		if (datain & KD_LSHIFT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RSHIFT) {
		id->mods ^= KD_RSHIFT;
		*dataout = 91;
		if (datain & KD_RSHIFT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_LALT) {
		id->mods ^= KD_LALT;
		*dataout = 92;
		if (datain & KD_LALT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RALT) {
		id->mods ^= KD_RALT;
		*dataout = 93;
		if (datain & KD_RALT)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_CNTL) {
		id->mods ^= KD_CNTL;
		*dataout = 94;
		if (datain & KD_CNTL)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_LCOMM) {
		id->mods ^= KD_LCOMM;
		*dataout = 95;
		if (datain & KD_LCOMM)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if ((datain ^ id->mods) & KD_RCOMM) {
		id->mods ^= KD_RCOMM;
		*dataout = 96;
		if (datain & KD_RCOMM)
			*type = WSCONS_EVENT_KEY_DOWN;
		else
			*type = WSCONS_EVENT_KEY_UP;
	} else if (datain & KD_KEYMASK) {
		if (datain & KD_DIRECTION)
			*type = WSCONS_EVENT_KEY_UP;
		else
			*type = WSCONS_EVENT_KEY_DOWN;

		*dataout = (datain & KD_KEYMASK);
	} else {
		*dataout = 0;
	}

	return 1;
}
