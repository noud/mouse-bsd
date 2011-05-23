/*	$NetBSD: cons.c,v 1.2 1999/08/05 18:08:13 thorpej Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)cons.c	8.3 (Berkeley) 12/14/93
 */

/*
 * Console (indirect) driver.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <machine/openfirm.h>
#include <machine/bsd_openprom.h>
#include <machine/eeprom.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/kbd.h>
#include <machine/sparc64.h>
#include <machine/autoconf.h>
#include <machine/conf.h>

#include "zs.h"

struct	tty *constty = 0;	/* virtual console output device */
struct	tty *fbconstty = 0;	/* tty structure for frame buffer console */
int	rom_console_input;	/* when set, hardclock calls cnrom() */

/* PROM console descriptors */
#define printf	prom_printf	/* Make sure we can see what's printed in here */
#define panic	prom_printf

int	cons_ocount;		/* output byte count */

/*
 * The output driver may munge the minor number in cons.t_dev.
 */
struct tty cons;		/* rom console tty device */
static void (*fcnstop) __P((struct tty *, int));

static void cnstart __P((struct tty *));
void cnstop __P((struct tty *, int));

static void cnfbstart __P((struct tty *));
static void cnfbstop __P((struct tty *, int));
static void cnfbdma __P((void *));
static struct tty  *xxcntty __P((dev_t));

extern char char_type[];

/*XXX*/
static struct tty *
xxcntty(dev_t dev)
{
	return &cons;
}

void
consinit()
{
	register struct tty *tp = &cons;
	register int in, out;
	register int node,fd;
	char buffer[128];
	register char *cp;
	extern int fbnode;

	/* Done already? */
	if (tp->t_param) return;

/*XXX*/	cdevsw[0].d_tty = xxcntty;
	tp->t_dev = makedev(0, 0);	/* /dev/console */
	tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
	tp->t_param = (int (*)(struct tty *, struct termios *))nullop;
	
	/* We need to probe the PROM device tree */
	
	in = out = -1;
	
	prom_printf("setting up stdin\r\n");
	node = OF_instance_to_package(OF_stdin());
	prom_printf("stdin package = %x\r\n", node);
	if (OF_getproplen(node,"keyboard") >= 0) {
		in = PROMDEV_KBD;
		goto setup_output;
	}
	if (strcmp(getpropstring(node,"device_type"),"serial") != 0) {
		/* not a serial, not keyboard. what is it?!? */
		in = -1;
		goto setup_output;
	}
	/*
	 * At this point we assume the device path is in the form
	 *   ....device@x,y:a for ttya and ...device@x,y:b for ttyb.
	 * If it isn't, we defer to the ROM
	 */
	if(OF_instance_to_path(OF_stdin(), buffer, sizeof(buffer)) <= 0) {
		printf("consinit: bogus stdin path.\n");
		goto setup_output;
	}
	cp = buffer;
	while (*cp)
		cp++;
	cp -= 2;
#ifdef DEBUG
	if (cp < buffer)
		panic("consinit: bad stdin path %s",buffer);
#endif
	/* XXX: only allows tty's a->z, assumes PROMDEV_TTYx contig */
	if (cp[0]==':' && cp[1] >= 'a' && cp[1] <= 'z')
		in = PROMDEV_TTYA + (cp[1] - 'a');
	/* else use rom */
setup_output:
	prom_printf("setting up stdout\r\n");
	node = OF_instance_to_package(OF_stdout());
	prom_printf("stdout package = %x\r\n", node);
	if (strcmp(getpropstring(node,"device_type"),"display") == 0) {
		/* frame buffer output */
		out = PROMDEV_SCREEN;
		fbnode = node;
	} else if (strcmp(getpropstring(node,"device_type"), "serial")
		   != 0) {
		/* not screen, not serial. Whatzit? */
		out = -1;
	} else { /* serial console. which? */
		/*
		 * At this point we assume the device path is in the
		 * form:
		 * ....device@x,y:a for ttya, etc.
		 * If it isn't, we defer to the ROM
		 */
		if(OF_instance_to_path(OF_stdout(), buffer, sizeof(buffer)) <= 0) {
			printf("consinit: bogus stdout path.\n");
			goto setup_output;
		}
		cp = buffer;
		while (*cp)
			cp++;
		cp -= 2;
#ifdef DEBUG
		if (cp < buffer)
			panic("consinit: bad stdout path %s",buffer);
#endif
		/* XXX: only allows tty's a->z, assumes PROMDEV_TTYx contig */
		if (cp[0]==':' && cp[1] >= 'a' && cp[1] <= 'z')
			out = PROMDEV_TTYA + (cp[1] - 'a');
		else out = -1;
	}
setup_console:
	switch (in) {
#if NZS > 0
	case PROMDEV_TTYA:
		zsconsole(tp, 0, 0, NULL);
		break;

	case PROMDEV_TTYB:
		zsconsole(tp, 1, 0, NULL);
		break;
#endif

	case PROMDEV_KBD:
		/*
		 * Tell the keyboard driver to direct ASCII input here.
		 */
		kbd_ascii(tp);
		break;

	default:
		rom_console_input = 1;
		printf("unknown console input source %d; using rom\n", in);
		break;
	}
	switch (out) {

#if NZS > 0
	case PROMDEV_TTYA:
		zsconsole(tp, 0, 1, &fcnstop);
		break;

	case PROMDEV_TTYB:
		zsconsole(tp, 1, 1, &fcnstop);
		break;
#endif

	case PROMDEV_SCREEN:
		fbconstty = tp;
		tp->t_oproc = cnfbstart;
		fcnstop = cnfbstop;
		break;

	default:
		printf("unknown console output sink %d; using rom\n", out);
		tp->t_oproc = cnstart;
		fcnstop = (void (*)(struct tty *, int))nullop;
		break;
	}
}

/* ARGSUSED */
int
cnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &cons;
 	static int firstopen = 1;
	static int rows = 0, cols = 0;

	if (firstopen) {
		int i;
		char *prop;

		clalloc(&tp->t_rawq, 1024, 1);
		clalloc(&tp->t_canq, 1024, 1);
		/* output queue doesn't need quoting */
		clalloc(&tp->t_outq, 1024, 0);
		tty_attach(tp);
		/*
		 * get the console struct winsize.
		 */
		if ((prop = getpropstring(optionsnode, "screen-#rows"))) {
			i = 0;
			while (*prop != '\0')
				i = i * 10 + *prop++ - '0';
			rows = (unsigned short)i;
		}
		if ((prop = getpropstring(optionsnode, "screen-#columns"))) {
			i = 0;
			while (*prop != '\0')
				i = i * 10 + *prop++ - '0';
			cols = (unsigned short)i;
		}
		firstopen = 0;
	}

	if ((tp->t_state & TS_ISOPEN) == 0) {
		/*
		 * Leave baud rate alone!
		 */
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_state = TS_ISOPEN | TS_CARR_ON;
		(void)(*tp->t_param)(tp, &tp->t_termios);
		ttsetwater(tp);
		tp->t_winsize.ws_row = rows;
		tp->t_winsize.ws_col = cols;
	} else if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/* ARGSUSED */
int
cnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &cons;

	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}

/* ARGSUSED */
int
cnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &cons;

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/* ARGSUSED */
int
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp;

	if ((tp = constty) == NULL ||
	    (tp->t_state & (TS_CARR_ON|TS_ISOPEN)) != (TS_CARR_ON|TS_ISOPEN))
		tp = &cons;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int
cnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp;
	int error;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty) {
		error = suser(p->p_ucred, (u_short *)NULL);
		if (error)
			return (error);
		constty = NULL;
		return (0);
	}
	tp = &cons;
	if ((error = linesw[tp->t_line].l_ioctl(tp, cmd, data, flag, p)) >= 0)
		return (error);
	if ((error = ttioctl(tp, cmd, data, flag, p)) >= 0)
		return (error);
	return (ENOTTY);
}

int
cnpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (ttpoll(makedev(major(dev), 0), events, p));
}

/*
 * The rest of this code is run only when we are using the ROM vectors.
 */

/*
 * Generic output.  We just call putchar.  (Very bad for performance.)
 */
static void
cnstart(tp)
	register struct tty *tp;
{
	register int c, s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_TTSTOP)) {
		splx(s);
		return;
	}
	while (tp->t_outq.c_cc) {
		unsigned char c0;
		c = getc(&tp->t_outq);
		/*
		 * *%&!*& ROM monitor console putchar is not reentrant!
		 * splhigh/tty around it so as not to run so long with
		 * clock interrupts blocked.
		 */
		c0 = c & 0177;
		(void) splhigh();
		OF_write(OF_stdout(), &c0, 1);
		(void) spltty();
	}
	if (tp->t_state & TS_ASLEEP) {		/* can't happen? */
		tp->t_state &= ~TS_ASLEEP;
		wakeup((caddr_t)&tp->t_outq);
	}
	selwakeup(&tp->t_wsel);
	splx(s);
}

void
cnstop(tp, flag)
	register struct tty *tp;
	int flag;
{

	(*fcnstop)(tp, flag);
}

/*
 * Frame buffer output.
 * We use pseudo-DMA, via the ROM `write string' function, called from
 * software clock interrupts.
 */
static void
cnfbstart(tp)
	register struct tty *tp;
{
	register int s;

#ifdef NOTDEF_DEBUG
	prom_printf("cnfbstart:start\r\n");
#endif
	s = spltty();		/* paranoid: splsoftclock should suffice */
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
#ifdef NOTDEF_DEBUG
		prom_printf("cnfbstart:busy state:%x\r\n", tp->t_state);
#endif
		return;
	}
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, awaken.
	 */
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
#ifdef NOTDEF_DEBUG
			prom_printf("cnfbstart: wakeup\r\n");
#endif
			wakeup((caddr_t)&tp->t_outq);
		}
#ifdef NOTDEF_DEBUG
		prom_printf("cnfbstart: selwakeup\r\n");
#endif
		selwakeup(&tp->t_wsel);
	}
	if (tp->t_outq.c_cc) {
		tp->t_state |= TS_BUSY;
		if (s == 0) {
			(void) spllowersoftclock();
#ifdef NOTDEF_DEBUG
			prom_printf("cnfbstart: cnfbdma\r\n");
#endif
			cnfbdma((void *)tp);
		} else {
#ifdef NOTDEF_DEBUG
/*		        printspl = 1;*/
			prom_printf("cnfbstart: timeout\r\n");
#endif
			timeout(cnfbdma, tp, 1);
		}
	}
	splx(s);
}

/*
 * Stop frame buffer output: just assert TS_FLUSH if necessary.
 */
static void
cnfbstop(tp, flag)
	register struct tty *tp;
	int flag;
{
	register int s = spltty();	/* paranoid */

	if ((tp->t_state & (TS_BUSY | TS_TTSTOP)) == TS_BUSY)
		tp->t_state |= TS_FLUSH;
#ifdef NOTDEF_DEBUG
	prom_printf("cnfbstop\r\n");
#endif
	splx(s);
}

/*
 * Do pseudo-dma (called from software interrupt).
 */
static void
cnfbdma(tpaddr)
	void *tpaddr;
{
	register struct tty *tp = tpaddr;
	register unsigned char *p, *q;
	register int n, c, s;

	s = spltty();			/* paranoid */
	if (tp->t_state & TS_FLUSH) {
		tp->t_state &= ~(TS_BUSY | TS_FLUSH);
		splx(s);
#ifdef NOTDEF_DEBUG
		prom_printf("cnfbdma: flush\r\n");
#endif
	} else {
		tp->t_state &= ~TS_BUSY;
		splx(s);
		p = tp->t_outq.c_cf;
		n = ndqb(&tp->t_outq, 0);
		for (q = p, c = n; --c >= 0; q++)
			if (*q & 0200)	/* high bits seem to be bad */
				*q &= ~0200;
#ifdef NOTDEF_DEBUG
		prom_printf("cnfbdma: write\r\n");
#endif
		OF_write(OF_stdout(), p, n);
		ndflush(&tp->t_outq, n);
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		cnfbstart(tp);
}

/*
 * The following is for rom console input.  The rom will not call
 * an `interrupt' routine on console input ready, so we must poll.
 * This is all rather sad.
 */
volatile int	cn_rxc = -1;		/* XXX receive `silo' */

/* called from hardclock, which is above spltty, so no tty calls! */
int
cnrom()
{
	unsigned char c0;

	if (cn_rxc >= 0)
		return (1);
	if(OF_read(OF_stdin(), &c0, 1) <= 0)
		return (0);
	cn_rxc = c0;
	return (1);
}

/* pseudo console software interrupt scheduled when cnrom() returns 1 */
void
cnrint()
{
	register struct tty *tp;
	register int c, s;

	s = splclock();
	c = cn_rxc;
	cn_rxc = -1;
	splx(s);
	if (c < 0)
		return;
	tp = &cons;
	if ((tp->t_cflag & CSIZE) == CS7) {
		/* XXX this should be done elsewhere, if at all */
		if (tp->t_cflag & PARENB)
			if (tp->t_cflag & PARODD ?
			    (char_type[c & 0177] & 0200) == (c & 0200) :
			    (char_type[c & 0177] & 0200) != (c & 0200))
				c |= TTY_PE;
		c &= ~0200;
	}
	(*linesw[tp->t_line].l_rint)(c, tp);
}

int
cngetc()
{
	register int s, c;
	register int n = 0;
	unsigned char c0;
	s = splhigh();
	while (n <= 0) {
		n = OF_read(OF_stdin(), &c0, 1);
	}
	splx(s);
	c = c0;
	if (c == '\r')
		c = '\n';
	return (c);
}

void
cnputc(c)
	register int c;
{
	register int s;
	unsigned char c0;

	if (c == '\n')
		cnputc('\r');
	c0 = c;
	s = splhigh();
	OF_write(OF_stdout(), &c0, 1);
	splx(s);
}

int swallow_zsintrs;

void
cnpollc(on)
	int on;
{
	/* 
	 * Need to tell zs driver to acknowledge all interrupts or we get
	 * annoying spurious interrupt messages.
	 */

	if (on) swallow_zsintrs++;
	else swallow_zsintrs--;
}
