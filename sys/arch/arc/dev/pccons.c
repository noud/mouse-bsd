/*	$NetBSD: pccons.c,v 1.13 2000/01/23 21:01:54 soda Exp $	*/
/*	$OpenBSD: pccons.c,v 1.15 1997/05/19 16:01:07 pefo Exp $	*/
/*	NetBSD: pccons.c,v 1.89 1995/05/04 19:35:20 cgd Exp	*/
/*	NetBSD: pms.c,v 1.21 1995/04/18 02:25:18 mycroft Exp	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

/*
 * code to work keyboard & display for PC-style console
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/kcore.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/pio.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/display.h>
#include <machine/pccons.h>
#include <arc/arc/arctype.h>
#include <arc/arc/arcbios.h>
#include <arc/pica/pica.h>
#include <arc/dti/desktech.h>

#include <dev/isa/isavar.h>
#include <machine/isa_machdep.h>
#include <machine/kbdreg.h>

#include <dev/cons.h>

extern int cputype;

#define	XFREE86_BUG_COMPAT

#ifndef BEEP_FREQ
#define BEEP_FREQ 1600
#endif
#ifndef BEEP_TIME
#define BEEP_TIME (hz/5)
#endif

#define PCBURST 128

static u_short *Crtat;			/* pointer to backing store */
static u_short *crtat;			/* pointer to current char */
static u_char async, kernel, polling;	/* Really, you don't want to know. */
static u_char lock_state = 0x00,	/* all off */
	      old_lock_state = 0xff,
	      typematic_rate = 0xff,	/* don't update until set by user */
	      old_typematic_rate = 0xff;
static u_short cursor_shape = 0xffff,	/* don't update until set by user */
	       old_cursor_shape = 0xffff;
static pccons_keymap_t scan_codes[KB_NUM_KEYS];/* keyboard translation table */
int pc_xmode = 0;

cdev_decl(pc);

/*
 *  Keyboard output queue.
 */
int	kb_oq_put = 0;
int	kb_oq_get = 0;
u_char	kb_oq[8];

#define	PCUNIT(x)	(minor(x))

static struct video_state {
	int 	cx, cy;		/* escape parameters */
	int 	row, col;	/* current cursor position */
	int 	nrow, ncol, nchr;	/* current screen geometry */
	int	offset;		/* Saved cursor pos */
	u_char	state;		/* parser state */
#define	VSS_ESCAPE	1
#define	VSS_EBRACE	2
#define	VSS_EPARAM	3
	char	so;		/* in standout mode? */
	char	color;		/* color or mono display */
	char	at;		/* normal attributes */
	char	so_at;		/* standout attributes */
} vs;

struct pc_softc {
	struct	device sc_dev;
	struct	tty *sc_tty;
};

struct opms_softc {		/* driver status information */
	struct device sc_dev;

	struct clist sc_q;
	struct selinfo sc_rsel;
	u_char sc_state;	/* mouse driver state */
#define	PMS_OPEN	0x01	/* device is open */
#define	PMS_ASLP	0x02	/* waiting for mouse data */
	u_char sc_status;	/* mouse button status */
	int sc_x, sc_y;		/* accumulated motion in the X,Y axis */
};

int pcprobe __P((struct device *, struct cfdata *, void *));
void pcattach __P((struct device *, struct device *, void *));
int pcintr __P((void *));
void pc_xmode_on __P((void));
void pc_xmode_off __P((void));
static u_char kbc_get8042cmd __P((void));
static int kbc_put8042cmd __P((u_char));
int kbc_8042sysreset __P((void));
int kbd_cmd __P((u_char, u_char));
static __inline int kbd_wait_output __P((void));
static __inline int kbd_wait_input __P((void));
static __inline void kbd_flush_input __P((void));
void set_cursor_shape __P((void));
void get_cursor_shape __P((void));
void async_update __P((void));
void do_async_update __P((u_char));;

void pccnattach __P((void));
void pccnputc __P((dev_t, int c));
int pccngetc __P((dev_t));
void pccnpollc __P((dev_t, int));

extern struct cfdriver pc_cd;

struct cfattach pc_pica_ca = {
	 sizeof(struct pc_softc), pcprobe, pcattach
};

struct cfattach pc_isa_ca = {
	 sizeof(struct pc_softc), pcprobe, pcattach
};
int opmsprobe __P((struct device *, struct cfdata *, void *));
void opmsattach __P((struct device *, struct device *, void *));
int opmsintr __P((void *));

struct cfattach opms_ca = {
	sizeof(struct opms_softc), opmsprobe, opmsattach
};

extern struct cfdriver opms_cd;

#define	PMSUNIT(dev)	(minor(dev))

#define	CHR		2

static unsigned int addr_6845;
static unsigned int mono_base = 0x3b4;
static unsigned int mono_buf = 0xb0000;
static unsigned int cga_base = 0x3d4;
static unsigned int cga_buf = 0xb8000;
static unsigned int kbd_cmdp = 0x64;
static unsigned int kbd_datap = 0x60;

char *sget __P((void));
void sput __P((u_char *, int));

void	pcstart __P((struct tty *));
int	pcparam __P((struct tty *, struct termios *));
static __inline void wcopy __P((void *, void *, u_int));

char	partab[];

extern void fillw __P((int, u_int16_t *, int));

#define	KBD_DELAY \
		DELAY(10);

/*
 * bcopy variant that only moves word-aligned 16-bit entities,
 * for stupid VGA cards.  cnt is required to be an even vale.
 */
static __inline void
wcopy(src, tgt, cnt)
	void *src, *tgt;
	u_int cnt;
{
	u_int16_t *from = src;
	u_int16_t *to = tgt;

	cnt >>= 1;
	if (to < from || to >= from + cnt)
		while(cnt--)
			*to++ = *from++;
	else {
		to += cnt;
		from += cnt;
		while(cnt--)
			*--to = *--from;
	}
}

static __inline int
kbd_wait_output()
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(kbd_cmdp) & KBS_IBF) == 0) {
			KBD_DELAY;
			return 1;
		}
	return 0;
}

static __inline int
kbd_wait_input()
{
	u_int i;

	for (i = 100000; i; i--)
		if ((inb(kbd_cmdp) & KBS_DIB) != 0) {
			KBD_DELAY;
			return 1;
		}
	return 0;
}

static __inline void
kbd_flush_input()
{
	u_char c;

	while ((c = inb(kbd_cmdp)) & 0x03)
		if ((c & KBS_DIB) == KBS_DIB) {
			/* XXX - delay is needed to prevent some keyboards from
			   wedging when the system boots */
			delay(6);
			(void) inb(kbd_datap);
		}
}



/*
 * Pass system reset command  to keyboard controller (8042).
 */
int
kbc_8042sysreset()
{

	if (!kbd_wait_output())
		return 0;
	outb(kbd_cmdp, 0xd1);
	if (!kbd_wait_output())
		return 0;
	outb(kbd_datap, 0);		/* ZAP */
	return 1;
}

#if 1
/*
 * Get the current command byte.
 */
static u_char
kbc_get8042cmd()
{

	if (!kbd_wait_output())
		return -1;
	outb(kbd_cmdp, K_RDCMDBYTE);
	if (!kbd_wait_input())
		return -1;
	return inb(kbd_datap);
}
#endif

/*
 * Pass command byte to keyboard controller (8042).
 */
static int
kbc_put8042cmd(val)
	u_char val;
{

	if (!kbd_wait_output())
		return 0;
	outb(kbd_cmdp, K_LDCMDBYTE);
	if (!kbd_wait_output())
		return 0;
	outb(kbd_datap, val);
	return 1;
}

/*
 * Pass command to keyboard itself
 */
int
kbd_cmd(val, polling)
	u_char val;
	u_char polling;
{
	u_int retries = 3;
	register u_int i;

	if(!polling) {
		i = spltty();
		if(kb_oq_get == kb_oq_put) {
			outb(kbd_datap, val);
		}
		kb_oq[kb_oq_put] = val;
		kb_oq_put = (kb_oq_put + 1) & 7;
		splx(i);
		return(1);
	}
	else do {
		if (!kbd_wait_output())
			return 0;
		outb(kbd_datap, val);
		for (i = 100000; i; i--) {
			if (inb(kbd_cmdp) & KBS_DIB) {
				register u_char c;

				KBD_DELAY;
				c = inb(kbd_datap);
				if (c == KBR_ACK || c == KBR_ECHO) {
					return 1;
				}
				if (c == KBR_RESEND) {
					break;
				}
#ifdef DIAGNOSTIC
				printf("kbd_cmd: input char %x lost\n", c);
#endif
			}
		}
	} while (--retries);
	return 0;
}

void
set_cursor_shape()
{
	register int iobase = addr_6845;

	outb(iobase, 10);
	outb(iobase+1, cursor_shape >> 8);
	outb(iobase, 11);
	outb(iobase+1, cursor_shape);
	old_cursor_shape = cursor_shape;
}

void
get_cursor_shape()
{
	register int iobase = addr_6845;

	outb(iobase, 10);
	cursor_shape = inb(iobase+1) << 8;
	outb(iobase, 11);
	cursor_shape |= inb(iobase+1);

	/*
	 * real 6845's, as found on, MDA, Hercules or CGA cards, do
	 * not support reading the cursor shape registers. the 6845
	 * tri-states it's data bus. This is _normally_ read by the
	 * cpu as either 0x00 or 0xff.. in which case we just use
	 * a line cursor.
	 */
	if (cursor_shape == 0x0000 || cursor_shape == 0xffff)
		cursor_shape = 0x0b10;
	else
		cursor_shape &= 0x1f1f;
}

void
do_async_update(poll)
	u_char poll;
{
	int pos;
	static int old_pos = -1;

	async = 0;

	if (lock_state != old_lock_state) {
		old_lock_state = lock_state;
		if (!kbd_cmd(KBC_MODEIND, poll) ||
		    !kbd_cmd(lock_state, poll)) {
			printf("pc: timeout updating leds\n");
			(void) kbd_cmd(KBC_ENABLE, poll);
		}
	}
	if (typematic_rate != old_typematic_rate) {
		old_typematic_rate = typematic_rate;
		if (!kbd_cmd(KBC_TYPEMATIC, poll) ||
		    !kbd_cmd(typematic_rate, poll)) {
			printf("pc: timeout updating typematic rate\n");
			(void) kbd_cmd(KBC_ENABLE, poll);
		}
	}

	if (pc_xmode > 0)
		return;

	pos = crtat - Crtat;
	if (pos != old_pos) {
		register int iobase = addr_6845;
		outb(iobase, 14);
		outb(iobase+1, pos >> 8);
		outb(iobase, 15);
		outb(iobase+1, pos);
		old_pos = pos;
	}
	if (cursor_shape != old_cursor_shape)
		set_cursor_shape();
}

void
async_update()
{

	if (kernel || polling) {
		if (async)
			untimeout((void(*)(void *))do_async_update, NULL);
		do_async_update(1);
	} else {
		if (async)
			return;
		async = 1;
		timeout((void(*)(void *))do_async_update, NULL, 1);
	}
}

/*
 * these are both bad jokes
 */
int
pcprobe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;
	u_int i;

	/* Make shure we're looking for this type of device */
	if(!strcmp((parent)->dv_cfdata->cf_driver->cd_name, "pica")) {
		if(!BUS_MATCHNAME(ca, "pc"))
			return(0);
	}

	/* Enable interrupts and keyboard, etc. */
	if (!kbc_put8042cmd(CMDBYTE)) {
		printf("pcprobe: command error\n");
		return 0;
	}

#if 1
	/* Flush any garbage. */
	kbd_flush_input();
	/* Reset the keyboard. */
	if (!kbd_cmd(KBC_RESET, 1)) {
		printf("pcprobe: reset error %d\n", 1);
		goto lose;
	}
	for (i = 600000; i; i--)
		if ((inb(kbd_cmdp) & KBS_DIB) != 0) {
			KBD_DELAY;
			break;
		}
	if (i == 0 || inb(kbd_datap) != KBR_RSTDONE) {
		printf("pcprobe: reset error %d\n", 2);
		goto lose;
	}
	/*
	 * Some keyboards seem to leave a second ack byte after the reset.
	 * This is kind of stupid, but we account for them anyway by just
	 * flushing the buffer.
	 */
	kbd_flush_input();
	/* Just to be sure. */
	if (!kbd_cmd(KBC_ENABLE, 1)) {
		printf("pcprobe: reset error %d\n", 3);
		goto lose;
	}

	/*
	 * Some keyboard/8042 combinations do not seem to work if the keyboard
	 * is set to table 1; in fact, it would appear that some keyboards just
	 * ignore the command altogether.  So by default, we use the AT scan
	 * codes and have the 8042 translate them.  Unfortunately, this is
	 * known to not work on some PS/2 machines.  We try desparately to deal
	 * with this by checking the (lack of a) translate bit in the 8042 and
	 * attempting to set the keyboard to XT mode.  If this all fails, well,
	 * tough luck.
	 *
	 * XXX It would perhaps be a better choice to just use AT scan codes
	 * and not bother with this.
	 */
	if (kbc_get8042cmd() & KC8_TRANS) {
		/* The 8042 is translating for us; use AT codes. */
		if (!kbd_cmd(KBC_SETTABLE, 1) || !kbd_cmd(2, 1)) {
			printf("pcprobe: reset error %d\n", 4);
			goto lose;
		}
	} else {
		/* Stupid 8042; set keyboard to XT codes. */
		if (!kbd_cmd(KBC_SETTABLE, 1) || !kbd_cmd(1, 1)) {
			printf("pcprobe: reset error %d\n", 5);
			goto lose;
		}
	}

lose:
	/*
	 * Technically, we should probably fail the probe.  But we'll be nice
	 * and allow keyboard-less machines to boot with the console.
	 */
#endif

	return 1;
}

void
pcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct confargs *ca = aux;
	struct isa_attach_args *ia = aux;
	struct pc_softc *sc = (void *)self;

	printf(": %s\n", vs.color ? "color" : "mono");
	do_async_update(1);

	switch(cputype) {
	case ACER_PICA_61:
		BUS_INTR_ESTABLISH(ca, pcintr, (void *)(long)sc);
		break;
	case DESKSTATION_RPC44:                     /* XXX ick */
	case DESKSTATION_TYNE:
		isa_intr_establish(ia->ia_ic, ia->ia_irq, 1,
			2, pcintr, sc);			/*XXX ick */
		break;
	}
}

int
pcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct pc_softc *sc;
	int unit = PCUNIT(dev);
	struct tty *tp;

	if (unit >= pc_cd.cd_ndevs)
		return ENXIO;
	sc = pc_cd.cd_devs[unit];
	if (sc == 0)
		return ENXIO;

	if (!sc->sc_tty) {
		tp = sc->sc_tty = ttymalloc();
	}
	else {
		tp = sc->sc_tty;
	}

	tp->t_oproc = pcstart;
	tp->t_param = pcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		pcparam(tp, &tp->t_termios);
		ttsetwater(tp);
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return EBUSY;
	tp->t_state |= TS_CARR_ON;

	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

int
pcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXX */
	ttyfree(tp);
#endif
	return(0);
}

int
pcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
pcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
pctty(dev)
	dev_t dev;
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;

	return (tp);
}

/*
 * Got a console receive interrupt -
 * the console processor wants to give us a character.
 * Catch the character, and see who it goes to.
 */
int
pcintr(arg)
	void *arg;
{
	struct pc_softc *sc = arg;
	register struct tty *tp = sc->sc_tty;
	u_char *cp;

	if ((inb(kbd_cmdp) & KBS_DIB) == 0)
		return 0;
	if (polling)
		return 1;
	do {
		cp = sget();
		if (!tp || (tp->t_state & TS_ISOPEN) == 0)
			return 1;
		if (cp)
			do
				(*linesw[tp->t_line].l_rint)(*cp++, tp);
			while (*cp);
	} while (inb(kbd_cmdp) & KBS_DIB);
	return 1;
}

int
pcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return error;

	switch (cmd) {
	case CONSOLE_X_MODE_ON:
		pc_xmode_on();
		return 0;
	case CONSOLE_X_MODE_OFF:
		pc_xmode_off();
		return 0;
	case CONSOLE_X_BELL:
		/*
		 * If set, data is a pointer to a length 2 array of
		 * integers.  data[0] is the pitch in Hz and data[1]
		 * is the duration in msec.
		 */
		if (data)
			sysbeep(((int*)data)[0],
				(((int*)data)[1] * hz) / 1000);
		else
			sysbeep(BEEP_FREQ, BEEP_TIME);
		return 0;
	case CONSOLE_SET_TYPEMATIC_RATE: {
 		u_char	rate;

 		if (!data)
			return EINVAL;
		rate = *((u_char *)data);
		/*
		 * Check that it isn't too big (which would cause it to be
		 * confused with a command).
		 */
		if (rate & 0x80)
			return EINVAL;
		typematic_rate = rate;
		async_update();
		return 0;
 	}
	case CONSOLE_SET_KEYMAP: {
		pccons_keymap_t *map = (pccons_keymap_t *) data;
		int i;

		if (!data)
			return EINVAL;
		for (i = 0; i < KB_NUM_KEYS; i++)
			if (map[i].unshift[KB_CODE_SIZE-1] ||
			    map[i].shift[KB_CODE_SIZE-1] ||
			    map[i].ctl[KB_CODE_SIZE-1] ||
			    map[i].altgr[KB_CODE_SIZE-1] ||
			    map[i].shift_altgr[KB_CODE_SIZE-1])
				return EINVAL;

		bcopy(data, scan_codes, sizeof(pccons_keymap_t[KB_NUM_KEYS]));
		return 0;
	}
	case CONSOLE_GET_KEYMAP:
		if (!data)
			return EINVAL;
		bcopy(scan_codes, data, sizeof(pccons_keymap_t[KB_NUM_KEYS]));
		return 0;

	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("pcioctl: impossible");
#endif
}

void
pcstart(tp)
	struct tty *tp;
{
	struct clist *cl;
	int s, len;
	u_char buf[PCBURST];

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	splx(s);
	/*
	 * We need to do this outside spl since it could be fairly
	 * expensive and we don't want our serial ports to overflow.
	 */
	cl = &tp->t_outq;
	len = q_to_b(cl, buf, PCBURST);
	sput(buf, len);
	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (cl->c_cc) {
		tp->t_state |= TS_TIMEOUT;
		timeout(ttrstrt, tp, 1);
	}
	if (cl->c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup(cl);
		}
		selwakeup(&tp->t_wsel);
	}
out:
	splx(s);
}

void
pcstop(tp, flag)
	struct tty *tp;
	int flag;
{
}

/* ARGSUSED */
void
pccnattach()
{
	int maj;
	static struct consdev pccons = {
		NULL, NULL, pccngetc, pccnputc, pccnpollc, NODEV, CN_NORMAL
	};

	/*
	 * For now, don't screw with it.
	 */
	/* crtat = 0; */
	switch(cputype) {

	case ACER_PICA_61:
		mono_base += PICA_V_LOCAL_VIDEO_CTRL;
		mono_buf += PICA_V_LOCAL_VIDEO;
		cga_base += PICA_V_LOCAL_VIDEO_CTRL;
		cga_buf += PICA_V_LOCAL_VIDEO;
		kbd_cmdp = PICA_SYS_KBD + 0x61;
		kbd_datap = PICA_SYS_KBD + 0x60;
		break;

	case DESKSTATION_TYNE:
		mono_base += TYNE_V_ISA_IO;
		mono_buf += TYNE_V_ISA_MEM;
		cga_base += TYNE_V_ISA_IO;
		cga_buf += TYNE_V_ISA_MEM;
		kbd_cmdp = TYNE_V_ISA_IO + 0x64;
		kbd_datap = TYNE_V_ISA_IO + 0x60;
		outb(TYNE_V_ISA_IO + 0x3ce, 6);		/* Correct video mode */
		outb(TYNE_V_ISA_IO + 0x3cf, inb(TYNE_V_ISA_IO + 0x3cf) | 0xc);
		kbc_put8042cmd(CMDBYTE);		/* Want XT codes.. */
		break;

	case DESKSTATION_RPC44:
		mono_base += arc_bus_io.bus_base;
		mono_buf += arc_bus_mem.bus_base;
		cga_base += arc_bus_io.bus_base;
		cga_buf = arc_bus_mem.bus_base + 0xa0000;
		kbd_cmdp = arc_bus_io.bus_base + 0x64;
		kbd_datap = arc_bus_io.bus_base + 0x60;
		kbc_put8042cmd(CMDBYTE);		/* Want XT codes.. */
		break;
	}

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == pcopen)
			break;
	pccons.cn_dev = makedev(maj, 0);

	cn_tab = &pccons;
}

/* ARGSUSED */
void
pccnputc(dev, c)
	dev_t dev;
	int c;
{
	u_char oldkernel = kernel;

	kernel = 1;
	if (c == '\n')
		sput("\r\n", 2);
	else
		sput(&c, 1);
	kernel = oldkernel;
}

/* ARGSUSED */
int
pccngetc(dev)
	dev_t dev;
{
	register char *cp;

	if (pc_xmode > 0)
		return 0;

	do {
		/* wait for byte */
		while ((inb(kbd_cmdp) & KBS_DIB) == 0);
		/* see if it's worthwhile */
		cp = sget();
	} while (!cp);
	if (*cp == '\r')
		return '\n';
	return *cp;
}

void
pccnpollc(dev, on)
	dev_t dev;
	int on;
{

	polling = on;
	if (!on) {
		int unit;
		struct pc_softc *sc;
		int s;

		/*
		 * If disabling polling on a device that's been configured,
		 * make sure there are no bytes left in the FIFO, holding up
		 * the interrupt line.  Otherwise we won't get any further
		 * interrupts.
		 */
		unit = PCUNIT(dev);
		if (pc_cd.cd_ndevs > unit) {
			sc = pc_cd.cd_devs[unit];
			if (sc != 0) {
				s = spltty();
				pcintr(sc);
				splx(s);
			}
		}
	}
}

/*
 * Set line parameters.
 */
int
pcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

#define	wrtchar(c, at) do {\
	char *cp = (char *)crtat; *cp++ = (c); *cp = (at); crtat++; vs.col++; \
} while (0)

/* translate ANSI color codes to standard pc ones */
static char fgansitopc[] = {
	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
};

static char bgansitopc[] = {
	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
};

static u_char iso2ibm437[] =
{
            0,     0,     0,     0,     0,     0,     0,     0,
            0,     0,     0,     0,     0,     0,     0,     0,
            0,     0,     0,     0,     0,     0,     0,     0,
            0,     0,     0,     0,     0,     0,     0,     0,
         0xff,  0xad,  0x9b,  0x9c,     0,  0x9d,     0,  0x40,
         0x6f,  0x63,  0x61,  0xae,     0,     0,     0,     0,
         0xf8,  0xf1,  0xfd,  0x33,     0,  0xe6,     0,  0xfa,
            0,  0x31,  0x6f,  0xaf,  0xac,  0xab,     0,  0xa8,
         0x41,  0x41,  0x41,  0x41,  0x8e,  0x8f,  0x92,  0x80,
         0x45,  0x90,  0x45,  0x45,  0x49,  0x49,  0x49,  0x49,
         0x81,  0xa5,  0x4f,  0x4f,  0x4f,  0x4f,  0x99,  0x4f,
         0x4f,  0x55,  0x55,  0x55,  0x9a,  0x59,     0,  0xe1,
         0x85,  0xa0,  0x83,  0x61,  0x84,  0x86,  0x91,  0x87,
         0x8a,  0x82,  0x88,  0x89,  0x8d,  0xa1,  0x8c,  0x8b,
            0,  0xa4,  0x95,  0xa2,  0x93,  0x6f,  0x94,  0x6f,
         0x6f,  0x97,  0xa3,  0x96,  0x81,  0x98,     0,     0
};

/*
 * `pc3' termcap emulation.
 */
void
sput(cp, n)
	u_char *cp;
	int n;
{
	u_char c, scroll = 0;

	if (pc_xmode > 0)
		return;

	if (crtat == 0) {
		volatile u_short *cp;
		u_short was;
		unsigned cursorat;

		cp = (volatile u_short *)cga_buf;
		was = *cp;
		*cp = (volatile u_short) 0xA55A;
		if (*cp != 0xA55A) {
			cp = (volatile u_short *)mono_buf;
			addr_6845 = mono_base;
			vs.color = 0;
		} else {
			*cp = was;
			addr_6845 = cga_base;
			vs.color = 1;
		}

#ifdef FAT_CURSOR
		cursor_shape = 0x0012;
#else
		get_cursor_shape();
#endif

		bios_display_info(&vs.col, &vs.row, &vs.ncol, &vs.nrow);
		vs.nchr = vs.ncol * vs.nrow;
		vs.col--;
		vs.row--;
		cursorat = vs.ncol * vs.row + vs.col;
		vs.at = FG_LIGHTGREY | BG_BLACK;

		Crtat = (u_short *)cp;
		crtat = Crtat + cursorat;

		if (vs.color == 0)
			vs.so_at = FG_BLACK | BG_LIGHTGREY;
		else
			vs.so_at = FG_YELLOW | BG_BLACK;

		fillw((vs.at << 8) | ' ', crtat, vs.nchr - cursorat);
	}

	while (n--) {
		if (!(c = *cp++))
			continue;

		switch (c) {
		case 0x1B:
			if (vs.state >= VSS_ESCAPE) {
				wrtchar(c, vs.so_at);
				vs.state = 0;
				goto maybe_scroll;
			} else
				vs.state = VSS_ESCAPE;
			break;

		case 0x9B:	/* CSI */
			vs.cx = vs.cy = 0;
			vs.state = VSS_EBRACE;
			break;

		case '\t': {
			int inccol = 8 - (vs.col & 7);
			crtat += inccol;
			vs.col += inccol;
		}
		maybe_scroll:
			if (vs.col >= vs.ncol) {
				vs.col -= vs.ncol;
				scroll = 1;
			}
			break;

		case '\b':
			if (crtat <= Crtat)
				break;
			--crtat;
			if (--vs.col < 0)
				vs.col += vs.ncol;	/* non-destructive backspace */
			break;

		case '\r':
			crtat -= vs.col;
			vs.col = 0;
			break;

		case '\n':
			crtat += vs.ncol;
			scroll = 1;
			break;

		default:
			switch (vs.state) {
			case 0:
				if (c == '\a')
					sysbeep(BEEP_FREQ, BEEP_TIME);
				else {
					/*
					 * If we're outputting multiple printed
					 * characters, just blast them to the
					 * screen until we reach the end of the
					 * buffer or a control character.  This
					 * saves time by short-circuiting the
					 * switch.
					 * If we reach the end of the line, we
					 * break to do a scroll check.
					 */
					for (;;) {
						if (c & 0x80)
							c = iso2ibm437[c&0x7f];

						if (vs.so)
							wrtchar(c, vs.so_at);
						else
							wrtchar(c, vs.at);
						if (vs.col >= vs.ncol) {
							vs.col = 0;
							scroll = 1;
							break;
						}
						if (!n || (c = *cp) < ' ')
							break;
						n--, cp++;
					}
				}
				break;
			case VSS_ESCAPE:
				switch (c) {
					case '[': /* Start ESC [ sequence */
						vs.cx = vs.cy = 0;
						vs.state = VSS_EBRACE;
						break;
					case 'c': /* Create screen & home */
						fillw((vs.at << 8) | ' ',
						    Crtat, vs.nchr);
						crtat = Crtat;
						vs.col = 0;
						vs.state = 0;
						break;
					case '7': /* save cursor pos */
						vs.offset = crtat - Crtat;
						vs.state = 0;
						break;
					case '8': /* restore cursor pos */
						crtat = Crtat + vs.offset;
						vs.row = vs.offset / vs.ncol;
						vs.col = vs.offset % vs.ncol;
						vs.state = 0;
						break;
					default: /* Invalid, clear state */
						wrtchar(c, vs.so_at);
						vs.state = 0;
						goto maybe_scroll;
				}
				break;

			default: /* VSS_EBRACE or VSS_EPARAM */
				switch (c) {
					int pos;
				case 'm':
					if (!vs.cx)
						vs.so = 0;
					else
						vs.so = 1;
					vs.state = 0;
					break;
				case 'A': { /* back cx rows */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.nrow;
					pos = crtat - Crtat;
					pos -= vs.ncol * cx;
					if (pos < 0)
						pos += vs.nchr;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'B': { /* down cx rows */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.nrow;
					pos = crtat - Crtat;
					pos += vs.ncol * cx;
					if (pos >= vs.nchr)
						pos -= vs.nchr;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'C': { /* right cursor */
					int cx = vs.cx,
					    col = vs.col;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.ncol;
					pos = crtat - Crtat;
					pos += cx;
					col += cx;
					if (col >= vs.ncol) {
						pos -= vs.ncol;
						col -= vs.ncol;
					}
					vs.col = col;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'D': { /* left cursor */
					int cx = vs.cx,
					    col = vs.col;
					if (cx <= 0)
						cx = 1;
					else
						cx %= vs.ncol;
					pos = crtat - Crtat;
					pos -= cx;
					col -= cx;
					if (col < 0) {
						pos += vs.ncol;
						col += vs.ncol;
					}
					vs.col = col;
					crtat = Crtat + pos;
					vs.state = 0;
					break;
				}
				case 'J': /* Clear ... */
					switch (vs.cx) {
					case 0:
						/* ... to end of display */
						fillw((vs.at << 8) | ' ',
						    crtat,
						    Crtat + vs.nchr - crtat);
						break;
					case 1:
						/* ... to next location */
						fillw((vs.at << 8) | ' ',
						    Crtat,
						    crtat - Crtat + 1);
						break;
					case 2:
						/* ... whole display */
						fillw((vs.at << 8) | ' ',
						    Crtat,
						    vs.nchr);
						break;
					}
					vs.state = 0;
					break;
				case 'K': /* Clear line ... */
					switch (vs.cx) {
					case 0:
						/* ... current to EOL */
						fillw((vs.at << 8) | ' ',
						    crtat,
						    vs.ncol - vs.col);
						break;
					case 1:
						/* ... beginning to next */
						fillw((vs.at << 8) | ' ',
						    crtat - vs.col,
						    vs.col + 1);
						break;
					case 2:
						/* ... entire line */
						fillw((vs.at << 8) | ' ',
						    crtat - vs.col, vs.ncol);
						break;
					}
					vs.state = 0;
					break;
				case 'f': /* in system V consoles */
				case 'H': { /* Cursor move */
					int cx = vs.cx,
					    cy = vs.cy;
					if (!cx || !cy) {
						crtat = Crtat;
						vs.col = 0;
					} else {
						if (cx > vs.nrow)
							cx = vs.nrow;
						if (cy > vs.ncol)
							cy = vs.ncol;
						crtat = Crtat +
						    (cx - 1) * vs.ncol + cy - 1;
						vs.col = cy - 1;
					}
					vs.state = 0;
					break;
				}
				case 'M': { /* delete cx rows */
					u_short *crtAt = crtat - vs.col;
					int cx = vs.cx,
					    row = (crtAt - Crtat) / vs.ncol,
					    nrow = vs.nrow - row;
					if (cx <= 0)
						cx = 1;
					else if (cx > nrow)
						cx = nrow;
					if (cx < nrow)
#ifdef PCCONS_FORCE_WORD
						wcopy(crtAt + vs.ncol * cx,
						    crtAt, vs.ncol * (nrow -
						    cx) * CHR);
#else
						bcopy(crtAt + vs.ncol * cx,
						    crtAt, vs.ncol * (nrow -
						    cx) * CHR);
#endif
					fillw((vs.at << 8) | ' ',
					    crtAt + vs.ncol * (nrow - cx),
					    vs.ncol * cx);
					vs.state = 0;
					break;
				}
				case 'S': { /* scroll up cx lines */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else if (cx > vs.nrow)
						cx = vs.nrow;
					if (cx < vs.nrow)
#ifdef PCCONS_FORCE_WORD
						wcopy(Crtat + vs.ncol * cx,
						    Crtat, vs.ncol * (vs.nrow -
						    cx) * CHR);
#else
						bcopy(Crtat + vs.ncol * cx,
						    Crtat, vs.ncol * (vs.nrow -
						    cx) * CHR);
#endif
					fillw((vs.at << 8) | ' ',
					    Crtat + vs.ncol * (vs.nrow - cx),
					    vs.ncol * cx);
					/* crtat -= vs.ncol * cx; XXX */
					vs.state = 0;
					break;
				}
				case 'L': { /* insert cx rows */
					u_short *crtAt = crtat - vs.col;
					int cx = vs.cx,
					    row = (crtAt - Crtat) / vs.ncol,
					    nrow = vs.nrow - row;
					if (cx <= 0)
						cx = 1;
					else if (cx > nrow)
						cx = nrow;
					if (cx < nrow)
#ifdef PCCONS_FORCE_WORD
						wcopy(crtAt,
						    crtAt + vs.ncol * cx,
						    vs.ncol * (nrow - cx) *
						    CHR);
#else
						bcopy(crtAt,
						    crtAt + vs.ncol * cx,
						    vs.ncol * (nrow - cx) *
						    CHR);
#endif
					fillw((vs.at << 8) | ' ', crtAt,
					    vs.ncol * cx);
					vs.state = 0;
					break;
				}
				case 'T': { /* scroll down cx lines */
					int cx = vs.cx;
					if (cx <= 0)
						cx = 1;
					else if (cx > vs.nrow)
						cx = vs.nrow;
					if (cx < vs.nrow)
#ifdef PCCONS_FORCE_WORD
						wcopy(Crtat,
						    Crtat + vs.ncol * cx,
						    vs.ncol * (vs.nrow - cx) *
						    CHR);
#else
						bcopy(Crtat,
						    Crtat + vs.ncol * cx,
						    vs.ncol * (vs.nrow - cx) *
						    CHR);
#endif
					fillw((vs.at << 8) | ' ', Crtat,
					    vs.ncol * cx);
					/* crtat += vs.ncol * cx; XXX */
					vs.state = 0;
					break;
				}
				case ';': /* Switch params in cursor def */
					vs.state = VSS_EPARAM;
					break;
				case 'r':
					vs.so_at = (vs.cx & FG_MASK) |
					    ((vs.cy << 4) & BG_MASK);
					vs.state = 0;
					break;
				case 's': /* save cursor pos */
					vs.offset = crtat - Crtat;
					vs.state = 0;
					break;
				case 'u': /* restore cursor pos */
					crtat = Crtat + vs.offset;
					vs.row = vs.offset / vs.ncol;
					vs.col = vs.offset % vs.ncol;
					vs.state = 0;
					break;
				case 'x': /* set attributes */
					switch (vs.cx) {
					case 0:
						vs.at = FG_LIGHTGREY | BG_BLACK;
						break;
					case 1:
						/* ansi background */
						if (!vs.color)
							break;
						vs.at &= FG_MASK;
						vs.at |= bgansitopc[vs.cy & 7];
						break;
					case 2:
						/* ansi foreground */
						if (!vs.color)
							break;
						vs.at &= BG_MASK;
						vs.at |= fgansitopc[vs.cy & 7];
						break;
					case 3:
						/* pc text attribute */
						if (vs.state >= VSS_EPARAM)
							vs.at = vs.cy;
						break;
					}
					vs.state = 0;
					break;

				default: /* Only numbers valid here */
					if ((c >= '0') && (c <= '9')) {
						if (vs.state >= VSS_EPARAM) {
							vs.cy *= 10;
							vs.cy += c - '0';
						} else {
							vs.cx *= 10;
							vs.cx += c - '0';
						}
					} else
						vs.state = 0;
					break;
				}
				break;
			}
		}
		if (scroll) {
			scroll = 0;
			/* scroll check */
			if (crtat >= Crtat + vs.nchr) {
				if (!kernel) {
					int s = spltty();
					if (lock_state & KB_SCROLL)
						tsleep(&lock_state,
						    PUSER, "pcputc", 0);
					splx(s);
				}
#if PCCONS_FORCE_WORD
				wcopy(Crtat + vs.ncol, Crtat,
				    (vs.nchr - vs.ncol) * CHR);
#else
				bcopy(Crtat + vs.ncol, Crtat,
				    (vs.nchr - vs.ncol) * CHR);
#endif
				fillw((vs.at << 8) | ' ',
				    Crtat + vs.nchr - vs.ncol,
				    vs.ncol);
				crtat -= vs.ncol;
			}
		}
	}
	async_update();
}

/* the unshifted code for KB_SHIFT keys is used by X to distinguish between
   left and right shift when reading the keyboard map */
static pccons_keymap_t	scan_codes[KB_NUM_KEYS] = {
/*  type       unshift   shift     control   altgr     shift_altgr scancode */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 0 unused */
  { KB_ASCII,  "\033",   "\033",   "\033",   "",       ""}, /* 1 ESCape */
  { KB_ASCII,  "1",      "!",      "!",      "",       ""}, /* 2 1 */
  { KB_ASCII,  "2",      "@",      "\000",   "",       ""}, /* 3 2 */
  { KB_ASCII,  "3",      "#",      "#",      "",       ""}, /* 4 3 */
  { KB_ASCII,  "4",      "$",      "$",      "",       ""}, /* 5 4 */
  { KB_ASCII,  "5",      "%",      "%",      "",       ""}, /* 6 5 */
  { KB_ASCII,  "6",      "^",      "\036",   "",       ""}, /* 7 6 */
  { KB_ASCII,  "7",      "&",      "&",      "",       ""}, /* 8 7 */
  { KB_ASCII,  "8",      "*",      "\010",   "",       ""}, /* 9 8 */
  { KB_ASCII,  "9",      "(",      "(",      "",       ""}, /* 10 9 */
  { KB_ASCII,  "0",      ")",      ")",      "",       ""}, /* 11 0 */
  { KB_ASCII,  "-",      "_",      "\037",   "",       ""}, /* 12 - */
  { KB_ASCII,  "=",      "+",      "+",      "",       ""}, /* 13 = */
  { KB_ASCII,  "\177",   "\177",   "\010",   "",       ""}, /* 14 backspace */
  { KB_ASCII,  "\t",     "\t",     "\t",     "",       ""}, /* 15 tab */
  { KB_ASCII,  "q",      "Q",      "\021",   "",       ""}, /* 16 q */
  { KB_ASCII,  "w",      "W",      "\027",   "",       ""}, /* 17 w */
  { KB_ASCII,  "e",      "E",      "\005",   "",       ""}, /* 18 e */
  { KB_ASCII,  "r",      "R",      "\022",   "",       ""}, /* 19 r */
  { KB_ASCII,  "t",      "T",      "\024",   "",       ""}, /* 20 t */
  { KB_ASCII,  "y",      "Y",      "\031",   "",       ""}, /* 21 y */
  { KB_ASCII,  "u",      "U",      "\025",   "",       ""}, /* 22 u */
  { KB_ASCII,  "i",      "I",      "\011",   "",       ""}, /* 23 i */
  { KB_ASCII,  "o",      "O",      "\017",   "",       ""}, /* 24 o */
  { KB_ASCII,  "p",      "P",      "\020",   "",       ""}, /* 25 p */
  { KB_ASCII,  "[",      "{",      "\033",   "",       ""}, /* 26 [ */
  { KB_ASCII,  "]",      "}",      "\035",   "",       ""}, /* 27 ] */
  { KB_ASCII,  "\r",     "\r",     "\n",     "",       ""}, /* 28 return */
  { KB_CTL,    "",       "",       "",       "",       ""}, /* 29 control */
  { KB_ASCII,  "a",      "A",      "\001",   "",       ""}, /* 30 a */
  { KB_ASCII,  "s",      "S",      "\023",   "",       ""}, /* 31 s */
  { KB_ASCII,  "d",      "D",      "\004",   "",       ""}, /* 32 d */
  { KB_ASCII,  "f",      "F",      "\006",   "",       ""}, /* 33 f */
  { KB_ASCII,  "g",      "G",      "\007",   "",       ""}, /* 34 g */
  { KB_ASCII,  "h",      "H",      "\010",   "",       ""}, /* 35 h */
  { KB_ASCII,  "j",      "J",      "\n",     "",       ""}, /* 36 j */
  { KB_ASCII,  "k",      "K",      "\013",   "",       ""}, /* 37 k */
  { KB_ASCII,  "l",      "L",      "\014",   "",       ""}, /* 38 l */
  { KB_ASCII,  ";",      ":",      ";",      "",       ""}, /* 39 ; */
  { KB_ASCII,  "'",      "\"",     "'",      "",       ""}, /* 40 ' */
  { KB_ASCII,  "`",      "~",      "`",      "",       ""}, /* 41 ` */
  { KB_SHIFT,  "\001",   "",       "",       "",       ""}, /* 42 shift */
  { KB_ASCII,  "\\",     "|",      "\034",   "",       ""}, /* 43 \ */
  { KB_ASCII,  "z",      "Z",      "\032",   "",       ""}, /* 44 z */
  { KB_ASCII,  "x",      "X",      "\030",   "",       ""}, /* 45 x */
  { KB_ASCII,  "c",      "C",      "\003",   "",       ""}, /* 46 c */
  { KB_ASCII,  "v",      "V",      "\026",   "",       ""}, /* 47 v */
  { KB_ASCII,  "b",      "B",      "\002",   "",       ""}, /* 48 b */
  { KB_ASCII,  "n",      "N",      "\016",   "",       ""}, /* 49 n */
  { KB_ASCII,  "m",      "M",      "\r",     "",       ""}, /* 50 m */
  { KB_ASCII,  ",",      "<",      "<",      "",       ""}, /* 51 , */
  { KB_ASCII,  ".",      ">",      ">",      "",       ""}, /* 52 . */
  { KB_ASCII,  "/",      "?",      "\037",   "",       ""}, /* 53 / */
  { KB_SHIFT,  "\002",   "",       "",       "",       ""}, /* 54 shift */
  { KB_KP,     "*",      "*",      "*",      "",       ""}, /* 55 kp * */
  { KB_ALT,    "",       "",       "",       "",       ""}, /* 56 alt */
  { KB_ASCII,  " ",      " ",      "\000",   "",       ""}, /* 57 space */
  { KB_CAPS,   "",       "",       "",       "",       ""}, /* 58 caps */
  { KB_FUNC,   "\033[M", "\033[Y", "\033[k", "",       ""}, /* 59 f1 */
  { KB_FUNC,   "\033[N", "\033[Z", "\033[l", "",       ""}, /* 60 f2 */
  { KB_FUNC,   "\033[O", "\033[a", "\033[m", "",       ""}, /* 61 f3 */
  { KB_FUNC,   "\033[P", "\033[b", "\033[n", "",       ""}, /* 62 f4 */
  { KB_FUNC,   "\033[Q", "\033[c", "\033[o", "",       ""}, /* 63 f5 */
  { KB_FUNC,   "\033[R", "\033[d", "\033[p", "",       ""}, /* 64 f6 */
  { KB_FUNC,   "\033[S", "\033[e", "\033[q", "",       ""}, /* 65 f7 */
  { KB_FUNC,   "\033[T", "\033[f", "\033[r", "",       ""}, /* 66 f8 */
  { KB_FUNC,   "\033[U", "\033[g", "\033[s", "",       ""}, /* 67 f9 */
  { KB_FUNC,   "\033[V", "\033[h", "\033[t", "",       ""}, /* 68 f10 */
  { KB_NUM,    "",       "",       "",       "",       ""}, /* 69 num lock */
  { KB_SCROLL, "",       "",       "",       "",       ""}, /* 70 scroll lock */
  { KB_KP,     "7",      "\033[H", "7",      "",       ""}, /* 71 kp 7 */
  { KB_KP,     "8",      "\033[A", "8",      "",       ""}, /* 72 kp 8 */
  { KB_KP,     "9",      "\033[I", "9",      "",       ""}, /* 73 kp 9 */
  { KB_KP,     "-",      "-",      "-",      "",       ""}, /* 74 kp - */
  { KB_KP,     "4",      "\033[D", "4",      "",       ""}, /* 75 kp 4 */
  { KB_KP,     "5",      "\033[E", "5",      "",       ""}, /* 76 kp 5 */
  { KB_KP,     "6",      "\033[C", "6",      "",       ""}, /* 77 kp 6 */
  { KB_KP,     "+",      "+",      "+",      "",       ""}, /* 78 kp + */
  { KB_KP,     "1",      "\033[F", "1",      "",       ""}, /* 79 kp 1 */
  { KB_KP,     "2",      "\033[B", "2",      "",       ""}, /* 80 kp 2 */
  { KB_KP,     "3",      "\033[G", "3",      "",       ""}, /* 81 kp 3 */
  { KB_KP,     "0",      "\033[L", "0",      "",       ""}, /* 82 kp 0 */
  { KB_KP,     ",",      "\177",   ",",      "",       ""}, /* 83 kp , */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 84 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 85 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 86 0 */
  { KB_FUNC,   "\033[W", "\033[i", "\033[u", "",       ""}, /* 87 f11 */
  { KB_FUNC,   "\033[X", "\033[j", "\033[v", "",       ""}, /* 88 f12 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 89 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 90 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 91 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 92 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 93 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 94 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 95 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 96 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 97 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 98 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 99 0 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 100 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 101 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 102 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 103 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 104 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 105 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 106 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 107 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 108 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 109 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 110 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 111 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 112 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 113 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 114 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 115 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 116 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 117 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 118 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 119 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 120 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 121 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 122 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 123 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 124 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 125 */
  { KB_NONE,   "",       "",       "",       "",       ""}, /* 126 */
  { KB_NONE,   "",       "",       "",       "",       ""}  /* 127 */
};

/*
 * Get characters from the keyboard.  If none are present, return NULL.
 */
char *
sget()
{
	u_char dt;
	static u_char extended = 0, shift_state = 0;
	static u_char capchar[2];

top:
	KBD_DELAY;
	dt = inb(kbd_datap);

	switch (dt) {
	case KBR_ACK: case KBR_ECHO:
		kb_oq_get = (kb_oq_get + 1) & 7;
		if(kb_oq_get != kb_oq_put) {
			outb(kbd_datap, kb_oq[kb_oq_get]);
		}
		goto loop;
	case KBR_RESEND:
		outb(kbd_datap, kb_oq[kb_oq_get]);
		goto loop;
	}

	if (pc_xmode > 0) {
#if defined(DDB) && defined(XSERVER_DDB)
		/* F12 enters the debugger while in X mode */
		if (dt == 88)
			Debugger();
#endif
		capchar[0] = dt;
		capchar[1] = 0;
		/*
		 * Check for locking keys.
		 *
		 * XXX Setting the LEDs this way is a bit bogus.  What if the
		 * keyboard has been remapped in X?
		 */
		switch (scan_codes[dt & 0x7f].type) {
		case KB_NUM:
			if (dt & 0x80) {
				shift_state &= ~KB_NUM;
				break;
			}
			if (shift_state & KB_NUM)
				break;
			shift_state |= KB_NUM;
			lock_state ^= KB_NUM;
			async_update();
			break;
		case KB_CAPS:
			if (dt & 0x80) {
				shift_state &= ~KB_CAPS;
				break;
			}
			if (shift_state & KB_CAPS)
				break;
			shift_state |= KB_CAPS;
			lock_state ^= KB_CAPS;
			async_update();
			break;
		case KB_SCROLL:
			if (dt & 0x80) {
				shift_state &= ~KB_SCROLL;
				break;
			}
			if (shift_state & KB_SCROLL)
				break;
			shift_state |= KB_SCROLL;
			lock_state ^= KB_SCROLL;
			if ((lock_state & KB_SCROLL) == 0)
				wakeup((caddr_t)&lock_state);
			async_update();
			break;
		}
		return capchar;
	}

	switch (dt) {
	case KBR_EXTENDED:
		extended = 1;
		goto loop;
	}

#ifdef DDB
	/*
	 * Check for cntl-alt-esc.
	 */
	if ((dt == 1) && (shift_state & (KB_CTL | KB_ALT)) == (KB_CTL | KB_ALT)) {
		/* XXX - check pccons_is_console */
		Debugger();
		dt |= 0x80;	/* discard esc (ddb discarded ctl-alt) */
	}
#endif

	/*
	 * Check for make/break.
	 */
	if (dt & 0x80) {
		/*
		 * break
		 */
		dt &= 0x7f;
		switch (scan_codes[dt].type) {
		case KB_NUM:
			shift_state &= ~KB_NUM;
			break;
		case KB_CAPS:
			shift_state &= ~KB_CAPS;
			break;
		case KB_SCROLL:
			shift_state &= ~KB_SCROLL;
			break;
		case KB_SHIFT:
			shift_state &= ~KB_SHIFT;
			break;
		case KB_ALT:
			if (extended)
				shift_state &= ~KB_ALTGR;
			else
				shift_state &= ~KB_ALT;
			break;
		case KB_CTL:
			shift_state &= ~KB_CTL;
			break;
		}
	} else {
		/*
		 * make
		 */
		switch (scan_codes[dt].type) {
		/*
		 * locking keys
		 */
		case KB_NUM:
			if (shift_state & KB_NUM)
				break;
			shift_state |= KB_NUM;
			lock_state ^= KB_NUM;
			async_update();
			break;
		case KB_CAPS:
			if (shift_state & KB_CAPS)
				break;
			shift_state |= KB_CAPS;
			lock_state ^= KB_CAPS;
			async_update();
			break;
		case KB_SCROLL:
			if (shift_state & KB_SCROLL)
				break;
			shift_state |= KB_SCROLL;
			lock_state ^= KB_SCROLL;
			if ((lock_state & KB_SCROLL) == 0)
				wakeup((caddr_t)&lock_state);
			async_update();
			break;
		/*
		 * non-locking keys
		 */
		case KB_SHIFT:
			shift_state |= KB_SHIFT;
			break;
		case KB_ALT:
			if (extended)
				shift_state |= KB_ALTGR;
			else
				shift_state |= KB_ALT;
			break;
		case KB_CTL:
			shift_state |= KB_CTL;
			break;
		case KB_ASCII:
			/* control has highest priority */
			if (shift_state & KB_CTL)
				capchar[0] = scan_codes[dt].ctl[0];
			else if (shift_state & KB_ALTGR) {
				if (shift_state & KB_SHIFT)
					capchar[0] = scan_codes[dt].shift_altgr[0];
				else
					capchar[0] = scan_codes[dt].altgr[0];
			}
			else {
				if (shift_state & KB_SHIFT)
					capchar[0] = scan_codes[dt].shift[0];
				else
					capchar[0] = scan_codes[dt].unshift[0];
			}
			if ((lock_state & KB_CAPS) && capchar[0] >= 'a' &&
			    capchar[0] <= 'z') {
				capchar[0] -= ('a' - 'A');
			}
			capchar[0] |= (shift_state & KB_ALT);
			extended = 0;
			return capchar;
		case KB_NONE:
printf("keycode %d\n",dt);
			break;
		case KB_FUNC: {
			char *more_chars;
			if (shift_state & KB_SHIFT)
				more_chars = scan_codes[dt].shift;
			else if (shift_state & KB_CTL)
				more_chars = scan_codes[dt].ctl;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return more_chars;
		}
		case KB_KP: {
			char *more_chars;
			if (shift_state & (KB_SHIFT | KB_CTL) ||
			    (lock_state & KB_NUM) == 0 || extended)
				more_chars = scan_codes[dt].shift;
			else
				more_chars = scan_codes[dt].unshift;
			extended = 0;
			return more_chars;
		}
		}
	}

	extended = 0;
loop:
	if ((inb(kbd_cmdp) & KBS_DIB) == 0)
		return 0;
	goto top;
}

int
pcmmap(dev, offset, nprot)
	dev_t dev;
	int offset;
	int nprot;
{

	switch(cputype) {

	case ACER_PICA_61:
		if (offset >= 0xa0000 && offset < 0xc0000)
			return mips_btop(PICA_P_LOCAL_VIDEO + offset);
		if (offset >= 0x0000 && offset < 0x10000)
			return mips_btop(PICA_P_LOCAL_VIDEO_CTRL + offset);
		if (offset >= 0x40000000 && offset < 0x40800000)
			return mips_btop(PICA_P_LOCAL_VIDEO + offset - 0x40000000);
		return -1;

	case DESKSTATION_TYNE:
		/* Addresses returned are a fake to be able to handle >32 bit
		 * physical addresses used by the tyne. The real physical adr
		 * processing is done in pmap.c. Until we are a real 64 bit
		 * port this is how it will be done.
		 */
		if (offset >= 0xa0000 && offset < 0xc0000)
			return mips_btop(TYNE_V_ISA_MEM + offset);
		if (offset >= 0x0000 && offset < 0x10000)
			return mips_btop(TYNE_V_ISA_IO + offset);
		if (offset >= 0x40000000 && offset < 0x40800000)
			return mips_btop(TYNE_V_ISA_MEM + offset - 0x40000000);
		return -1;
	}
	return -1;
}

void
pc_xmode_on()
{
	if (pc_xmode)
		return;
	pc_xmode = 1;

#ifdef XFREE86_BUG_COMPAT
	/* If still unchanged, get current shape. */
	if (cursor_shape == 0xffff)
		get_cursor_shape();
#endif
}

void
pc_xmode_off()
{
	if (pc_xmode == 0)
		return;
	pc_xmode = 0;

#ifdef XFREE86_BUG_COMPAT
	/* XXX It would be hard to justify why the X server doesn't do this. */
	set_cursor_shape();
#endif
	async_update();
}

#include <machine/mouse.h>

/* status bits */
#define	PMS_OBUF_FULL	0x01
#define	PMS_IBUF_FULL	0x02

/* controller commands */
#define	PMS_INT_ENABLE	0x47	/* enable controller interrupts */
#define	PMS_INT_DISABLE	0x65	/* disable controller interrupts */
#define	PMS_AUX_ENABLE	0xa7	/* enable auxiliary port */
#define	PMS_AUX_DISABLE	0xa8	/* disable auxiliary port */
#define	PMS_MAGIC_1	0xa9	/* XXX */

#define	PMS_8042_CMD	0x65

/* mouse commands */
#define	PMS_SET_SCALE11	0xe6	/* set scaling 1:1 */
#define	PMS_SET_SCALE21 0xe7	/* set scaling 2:1 */
#define	PMS_SET_RES	0xe8	/* set resolution */
#define	PMS_GET_SCALE	0xe9	/* get scaling factor */
#define	PMS_SET_STREAM	0xea	/* set streaming mode */
#define	PMS_SET_SAMPLE	0xf3	/* set sampling rate */
#define	PMS_DEV_ENABLE	0xf4	/* mouse on */
#define	PMS_DEV_DISABLE	0xf5	/* mouse off */
#define	PMS_RESET	0xff	/* reset */

#define	PMS_CHUNK	128	/* chunk size for read */
#define	PMS_BSIZE	1020	/* buffer size */

#define	FLUSHQ(q) { if((q)->c_cc) ndflush(q, (q)->c_cc); }

int opmsopen __P((dev_t, int));
int opmsclose __P((dev_t, int));
int opmsread __P((dev_t, struct uio *, int));
int opmsioctl __P((dev_t, u_long, caddr_t, int));
int opmsselect __P((dev_t, int, struct proc *));
static __inline void pms_dev_cmd __P((u_char));
static __inline void pms_aux_cmd __P((u_char));
static __inline void pms_pit_cmd __P((u_char));

static __inline void
pms_dev_cmd(value)
	u_char value;
{
	kbd_flush_input();
	outb(kbd_cmdp, 0xd4);
	kbd_flush_input();
	outb(kbd_datap, value);
}

static __inline void
pms_aux_cmd(value)
	u_char value;
{
	kbd_flush_input();
	outb(kbd_cmdp, value);
}

static __inline void
pms_pit_cmd(value)
	u_char value;
{
	kbd_flush_input();
	outb(kbd_cmdp, 0x60);
	kbd_flush_input();
	outb(kbd_datap, value);
}

int
opmsprobe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;
	u_char x;

	/* Make shure we're looking for this type of device */
	if(!BUS_MATCHNAME(ca, "pms"))
		return(0);

	pms_dev_cmd(KBC_RESET);
	pms_aux_cmd(PMS_MAGIC_1);
	delay(10000);
	x = inb(kbd_datap);
	pms_pit_cmd(PMS_INT_DISABLE);
	if (x & 0x04)
		return 0;

	return 1;
}

void
opmsattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct opms_softc *sc = (void *)self;
	struct confargs *ca = aux;

	printf("\n");

	/* Other initialization was done by opmsprobe. */
	sc->sc_state = 0;

	BUS_INTR_ESTABLISH(ca, opmsintr, (void *)(long)sc);
}

int
opmsopen(dev, flag)
	dev_t dev;
	int flag;
{
	int unit = PMSUNIT(dev);
	struct opms_softc *sc;

	if (unit >= opms_cd.cd_ndevs)
		return ENXIO;
	sc = opms_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (sc->sc_state & PMS_OPEN)
		return EBUSY;

	if (clalloc(&sc->sc_q, PMS_BSIZE, 0) == -1)
		return ENOMEM;

	sc->sc_state |= PMS_OPEN;
	sc->sc_status = 0;
	sc->sc_x = sc->sc_y = 0;

	/* Enable interrupts. */
	pms_dev_cmd(PMS_DEV_ENABLE);
	pms_aux_cmd(PMS_AUX_ENABLE);
	pms_dev_cmd(PMS_SET_RES);
	pms_dev_cmd(3);		/* 8 counts/mm */
	pms_dev_cmd(PMS_SET_SCALE21);
#if 0
	pms_dev_cmd(PMS_SET_SAMPLE);
	pms_dev_cmd(100);	/* 100 samples/sec */
	pms_dev_cmd(PMS_SET_STREAM);
#endif
	pms_pit_cmd(PMS_INT_ENABLE);

	return 0;
}

int
opmsclose(dev, flag)
	dev_t dev;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];

	/* Disable interrupts. */
	pms_dev_cmd(PMS_DEV_DISABLE);
	pms_pit_cmd(PMS_INT_DISABLE);
	pms_aux_cmd(PMS_AUX_DISABLE);

	sc->sc_state &= ~PMS_OPEN;

	clfree(&sc->sc_q);

	return 0;
}

int
opmsread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int s;
	int error = 0;
	size_t length;
	u_char buffer[PMS_CHUNK];

	/* Block until mouse activity occured. */

	s = spltty();
	while (sc->sc_q.c_cc == 0) {
		if (flag & IO_NDELAY) {
			splx(s);
			return EWOULDBLOCK;
		}
		sc->sc_state |= PMS_ASLP;
		error = tsleep((caddr_t)sc, PZERO | PCATCH, "pmsrea", 0);
		if (error) {
			sc->sc_state &= ~PMS_ASLP;
			splx(s);
			return error;
		}
	}
	splx(s);

	/* Transfer as many chunks as possible. */

	while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) {
		length = min(sc->sc_q.c_cc, uio->uio_resid);
		if (length > sizeof(buffer))
			length = sizeof(buffer);

		/* Remove a small chunk from the input queue. */
		(void) q_to_b(&sc->sc_q, buffer, length);

		/* Copy the data to the user process. */
		error = uiomove(buffer, length, uio);
		if (error)
			break;
	}

	return error;
}

int
opmsioctl(dev, cmd, addr, flag)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	struct mouseinfo info;
	int s;
	int error;

	switch (cmd) {
	case MOUSEIOCREAD:
		s = spltty();

		info.status = sc->sc_status;
		if (sc->sc_x || sc->sc_y)
			info.status |= MOVEMENT;

		if (sc->sc_x > 127)
			info.xmotion = 127;
		else if (sc->sc_x < -127)
			/* Bounding at -127 avoids a bug in XFree86. */
			info.xmotion = -127;
		else
			info.xmotion = sc->sc_x;

		if (sc->sc_y > 127)
			info.ymotion = 127;
		else if (sc->sc_y < -127)
			info.ymotion = -127;
		else
			info.ymotion = sc->sc_y;

		/* Reset historical information. */
		sc->sc_x = sc->sc_y = 0;
		sc->sc_status &= ~BUTCHNGMASK;
		ndflush(&sc->sc_q, sc->sc_q.c_cc);

		splx(s);
		error = copyout(&info, addr, sizeof(struct mouseinfo));
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}

/* Masks for the first byte of a packet */
#define PS2LBUTMASK 0x01
#define PS2RBUTMASK 0x02
#define PS2MBUTMASK 0x04

int
opmsintr(arg)
	void *arg;
{
	struct opms_softc *sc = arg;
	static int state = 0;
	static u_char buttons;
	u_char changed;
	static char dx, dy;
	u_char buffer[5];

	if ((sc->sc_state & PMS_OPEN) == 0) {
		/* Interrupts are not expected.  Discard the byte. */
		kbd_flush_input();
		return 0;
	}

	switch (state) {

	case 0:
		buttons = inb(kbd_datap);
		if ((buttons & 0xc0) == 0)
			++state;
		break;

	case 1:
		dx = inb(kbd_datap);
		/* Bounding at -127 avoids a bug in XFree86. */
		dx = (dx == -128) ? -127 : dx;
		++state;
		break;

	case 2:
		dy = inb(kbd_datap);
		dy = (dy == -128) ? -127 : dy;
		state = 0;

		buttons = ((buttons & PS2LBUTMASK) << 2) |
			  ((buttons & (PS2RBUTMASK | PS2MBUTMASK)) >> 1);
		changed = ((buttons ^ sc->sc_status) & BUTSTATMASK) << 3;
		sc->sc_status = buttons | (sc->sc_status & ~BUTSTATMASK) | changed;

		if (dx || dy || changed) {
			/* Update accumulated movements. */
			sc->sc_x += dx;
			sc->sc_y += dy;

			/* Add this event to the queue. */
			buffer[0] = 0x80 | (buttons & BUTSTATMASK);
			if(dx < 0)
				buffer[0] |= 0x10;
			buffer[1] = dx & 0x7f;
			if(dy < 0)
				buffer[0] |= 0x20;
			buffer[2] = dy & 0x7f;
			buffer[3] = buffer[4] = 0;
			(void) b_to_q(buffer, sizeof buffer, &sc->sc_q);

			if (sc->sc_state & PMS_ASLP) {
				sc->sc_state &= ~PMS_ASLP;
				wakeup((caddr_t)sc);
			}
			selwakeup(&sc->sc_rsel);
		}

		break;
	}
	return -1;
}

int
opmspoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct opms_softc *sc = opms_cd.cd_devs[PMSUNIT(dev)];
	int revents = 0;
	int s = spltty();

	if (events & (POLLIN | POLLRDNORM))
		if (sc->sc_q.c_cc > 0)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);

	splx(s);
	return (revents);
}
