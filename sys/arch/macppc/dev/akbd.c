/*	$NetBSD: akbd.c,v 1.9 1999/09/05 05:30:30 tsubai Exp $	*/

/*
 * Copyright (C) 1998	Colin Wood
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
 *	This product includes software developed by Colin Wood.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/signalvar.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <machine/autoconf.h>
#define KEYBOARD_ARRAY
#include <machine/keyboard.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/aedvar.h>
#include <macppc/dev/akbdmap.h>
#include <macppc/dev/akbdvar.h>
#include <macppc/dev/amsvar.h>

#include "aed.h"

/*
 * Function declarations.
 */
static int	akbdmatch __P((struct device *, struct cfdata *, void *));
static void	akbdattach __P((struct device *, struct device *, void *));
void		kbd_adbcomplete __P((caddr_t buffer, caddr_t data_area, int adb_command));
static void	kbd_processevent __P((adb_event_t *event, struct akbd_softc *));
#ifdef notyet
static u_char	getleds __P((int));
static int	setleds __P((struct akbd_softc *, u_char));
static void	blinkleds __P((struct akbd_softc *));
#endif

/*
 * Local variables.
 */
static volatile int kbd_done;  /* Did ADBOp() complete? */

/* Driver definition. */
struct cfattach akbd_ca = {
	sizeof(struct akbd_softc), akbdmatch, akbdattach
};

extern struct cfdriver akbd_cd;

int akbd_enable __P((void *, int));
void akbd_set_leds __P((void *, int));
int akbd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));

struct wskbd_accessops akbd_accessops = {
	akbd_enable,
	akbd_set_leds,
	akbd_ioctl,
};

void akbd_cngetc __P((void *, u_int *, int *));
void akbd_cnpollc __P((void *, int));

struct wskbd_consops akbd_consops = {
	akbd_cngetc,
	akbd_cnpollc,
};

struct wskbd_mapdata akbd_keymapdata = {
	akbd_keydesctab,
	KB_US,
};

static int akbd_is_console;

static int
akbdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void   *aux;
{
	struct adb_attach_args *aa_args = (struct adb_attach_args *)aux;

	if (aa_args->origaddr == ADBADDR_KBD)
		return 1;
	else
		return 0;
}

static void
akbdattach(parent, self, aux)
	struct device *parent, *self;
	void   *aux;
{
	ADBSetInfoBlock adbinfo;
	struct akbd_softc *sc = (struct akbd_softc *)self;
	struct adb_attach_args *aa_args = (struct adb_attach_args *)aux;
	int count, error;
	short cmd;
	u_char buffer[9];
	struct wskbddev_attach_args a;

	sc->origaddr = aa_args->origaddr;
	sc->adbaddr = aa_args->adbaddr;
	sc->handler_id = aa_args->handler_id;

	sc->sc_leds = (u_int8_t)0x00;	/* initially off */

	adbinfo.siServiceRtPtr = (Ptr)kbd_adbcomplete;
	adbinfo.siDataAreaAddr = (caddr_t)sc;

	switch (sc->handler_id) {
	case ADB_STDKBD:
		printf("standard keyboard\n");
		break;
	case ADB_ISOKBD:
		printf("standard keyboard (ISO layout)\n");
		break;
	case ADB_EXTKBD:
		kbd_done = 0;
		cmd = (((sc->adbaddr << 4) & 0xf0) | 0x0d ); /* talk R1 */
		ADBOp((Ptr)buffer, (Ptr)extdms_complete,
		    (Ptr)&kbd_done, cmd);

		/* Wait until done, but no more than 2 secs */
		count = 40000;
		while (!kbd_done && count-- > 0)
			delay(50);

		/* Ignore Logitech MouseMan/Trackman pseudo keyboard */
		if (kbd_done && buffer[1] == 0x9a && buffer[2] == 0x20) {
			printf("Mouseman (non-EMP) pseudo keyboard\n");
			adbinfo.siServiceRtPtr = (Ptr)0;
			adbinfo.siDataAreaAddr = (Ptr)0;
		} else if (kbd_done && buffer[1] == 0x9a && buffer[2] == 0x21) {
			printf("Trackman (non-EMP) pseudo keyboard\n");
			adbinfo.siServiceRtPtr = (Ptr)0;
			adbinfo.siDataAreaAddr = (Ptr)0;
		} else {
			printf("extended keyboard\n");
#ifdef notyet
			blinkleds(sc);
#endif
		}
		break;
	case ADB_EXTISOKBD:
		printf("extended keyboard (ISO layout)\n");
#ifdef notyet
		blinkleds(sc);
#endif
		break;
	case ADB_KBDII:
		printf("keyboard II\n");
		break;
	case ADB_ISOKBDII:
		printf("keyboard II (ISO layout)\n");
		break;
	case ADB_PBKBD:
		printf("PowerBook keyboard\n");
		break;
	case ADB_PBISOKBD:
		printf("PowerBook keyboard (ISO layout)\n");
		break;
	case ADB_ADJKPD:
		printf("adjustable keypad\n");
		break;
	case ADB_ADJKBD:
		printf("adjustable keyboard\n");
		break;
	case ADB_ADJISOKBD:
		printf("adjustable keyboard (ISO layout)\n");
		break;
	case ADB_ADJJAPKBD:
		printf("adjustable keyboard (Japanese layout)\n");
		break;
	case ADB_PBEXTISOKBD:
		printf("PowerBook extended keyboard (ISO layout)\n");
		break;
	case ADB_PBEXTJAPKBD:
		printf("PowerBook extended keyboard (Japanese layout)\n");
		break;
	case ADB_JPKBDII:
		printf("keyboard II (Japanese layout)\n");
		break;
	case ADB_PBEXTKBD:
		printf("PowerBook extended keyboard\n");
		break;
	case ADB_DESIGNKBD:
		printf("extended keyboard\n");
#ifdef notyet
		blinkleds(sc);
#endif
		break;
	case ADB_PBJPKBD:
		printf("PowerBook keyboard (Japanese layout)\n");
		break;
	case ADB_PBG3JPKBD:
		printf("PowerBook G3 keyboard (Japanese layout)\n");
		break;
	default:
		printf("mapped device (%d)\n", sc->handler_id);
		break;
	}
	error = SetADBInfo(&adbinfo, sc->adbaddr);
#ifdef ADB_DEBUG
	if (adb_debug)
		printf("kbd: returned %d from SetADBInfo\n", error);
#endif

	a.console = akbd_is_console;
	a.keymap = &akbd_keymapdata;
	a.accessops = &akbd_accessops;
	a.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &a, wskbddevprint);
}


/*
 * Handle putting the keyboard data received from the ADB into
 * an ADB event record.
 */
void
kbd_adbcomplete(buffer, data_area, adb_command)
	caddr_t buffer;
	caddr_t data_area;
	int adb_command;
{
	adb_event_t event;
	struct akbd_softc *ksc;
	int adbaddr;
#ifdef ADB_DEBUG
	int i;

	if (adb_debug)
		printf("adb: transaction completion\n");
#endif

	adbaddr = (adb_command & 0xf0) >> 4;
	ksc = (struct akbd_softc *)data_area;

	event.addr = adbaddr;
	event.hand_id = ksc->handler_id;
	event.def_addr = ksc->origaddr;
	event.byte_count = buffer[0];
	memcpy(event.bytes, buffer + 1, event.byte_count);

#ifdef ADB_DEBUG
	if (adb_debug) {
		printf("kbd: from %d at %d (org %d) %d:", event.addr,
		    event.hand_id, event.def_addr, buffer[0]);
		for (i = 1; i <= buffer[0]; i++)
			printf(" %x", buffer[i]);
		printf("\n");
	}
#endif

	microtime(&event.timestamp);

	kbd_processevent(&event, ksc);
}

/*
 * Given a keyboard ADB event, record the keycodes and call the key
 * repeat handler, optionally passing the event through the mouse
 * button emulation handler first.
 */
static void
kbd_processevent(event, ksc)
        adb_event_t *event;
        struct akbd_softc *ksc;
{
        adb_event_t new_event;

        new_event = *event;
	new_event.u.k.key = event->bytes[0];
	new_event.bytes[1] = 0xff;
	kbd_intr(&new_event);
#if NAED > 0
	aed_input(&new_event);
#endif
	if (event->bytes[1] != 0xff) {
		new_event.u.k.key = event->bytes[1];
		new_event.bytes[0] = event->bytes[1];
		new_event.bytes[1] = 0xff;
		kbd_intr(&new_event);
#if NAED > 0
		aed_input(&new_event);
#endif
	}

}

#ifdef notyet
/*
 * Get the actual hardware LED state and convert it to softc format.
 */
static u_char
getleds(addr)
	int	addr;
{
	short cmd;
	u_char buffer[9], leds;

	leds = 0x00;	/* all off */
	buffer[0] = 0;
	kbd_done = 0;

	/* talk R2 */
	cmd = ((addr & 0xf) << 4) | 0x0c | 0x02;
	ADBOp((Ptr)buffer, (Ptr)extdms_complete, (Ptr)&kbd_done, cmd);
	while (!kbd_done)
		/* busy-wait until done */ ;

	if (buffer[0] > 0)
		leds = ~(buffer[2]) & 0x07;

	return (leds);
}

/*
 * Set the keyboard LED's.
 *
 * Automatically translates from ioctl/softc format to the
 * actual keyboard register format
 */
static int
setleds(ksc, leds)
	struct akbd_softc *ksc;
	u_char	leds;
{
	int addr;
	short cmd;
	u_char buffer[9];

	if ((leds & 0x07) == (ksc->sc_leds & 0x07))
		return (0);

	addr = ksc->adbaddr;
	buffer[0] = 0;
	kbd_done = 0;

	/* talk R2 */
	cmd = ((addr & 0xf) << 4) | 0x0c | 0x02;
	ADBOp((Ptr)buffer, (Ptr)extdms_complete, (Ptr)&kbd_done, cmd);
	while (!kbd_done)
		/* busy-wait until done */ ;

	if (buffer[0] == 0)
		return (EIO);

	leds = ~leds & 0x07;
	buffer[2] &= 0xf8;
	buffer[2] |= leds;

	/* listen R2 */
	cmd = ((addr & 0xf) << 4) | 0x08 | 0x02;
	ADBOp((Ptr)buffer, (Ptr)extdms_complete, (Ptr)&kbd_done, cmd);
	while (!kbd_done)
		/* busy-wait until done */ ;

	/* talk R2 */
	cmd = ((addr & 0xf) << 4) | 0x0c | 0x02;
	ADBOp((Ptr)buffer, (Ptr)extdms_complete, (Ptr)&kbd_done, cmd);
	while (!kbd_done)
		/* busy-wait until done */ ;

	if (buffer[0] == 0)
		return (EIO);

	ksc->sc_leds = ~((u_int8_t)buffer[2]) & 0x07;

	if ((buffer[2] & 0xf8) != leds)
		return (EIO);
	else
		return (0);
}

/*
 * Toggle all of the LED's on and off, just for show.
 */
static void
blinkleds(ksc)
	struct akbd_softc *ksc;
{
	int addr, i;
	u_char blinkleds, origleds;

	addr = ksc->adbaddr;
	origleds = getleds(addr);
	blinkleds = LED_NUMLOCK | LED_CAPSLOCK | LED_SCROLL_LOCK;

	(void)setleds(ksc, blinkleds);

	for (i = 0; i < 10000; i++)
		delay(50);

	/* make sure that we restore the LED settings */
	i = 10;
	do {
		(void)setleds(ksc, (u_char)0x00);
	} while (setleds(ksc, (u_char)0x00) && (i-- > 0));

	return;
}
#endif

int
akbd_enable(v, on)
	void *v;
	int on;
{
	return 0;
}

void
akbd_set_leds(v, on)
	void *v;
	int on;
{
}

int
akbd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {

	case WSKBDIO_GTYPE:
		*(int *)data = 0;		/* XXX */
		return 0;
	case WSKBDIO_SETLEDS:
		return 0;
	case WSKBDIO_GETLEDS:
		*(int *)data = 0;
		return 0;
	}
	/* kbdioctl(...); */

	return -1;
}

static int polledkey;
extern int adb_polling;

int
kbd_intr(event)
	adb_event_t *event;
{
	int key, press, val;
	int type;

	struct akbd_softc *sc = akbd_cd.cd_devs[0];

	key = event->u.k.key;
	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	type = press ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;

	switch (key) {
	case 185:	/* Caps Lock released */
		type = WSCONS_EVENT_KEY_DOWN;
		wskbd_input(sc->sc_wskbddev, type, val);
		type = WSCONS_EVENT_KEY_UP;
		break;
	case 245:
		pm_eject_pcmcia(0);
		break;
	case 244:
		pm_eject_pcmcia(1);
		break;
	}

	if (adb_polling)
		polledkey = key;
	else
		wskbd_input(sc->sc_wskbddev, type, val);

	return 0;
}

int
akbd_cnattach()
{

	akbd_is_console = 1;
	wskbd_cnattach(&akbd_consops, NULL, &akbd_keymapdata);
	return 0;
}

void
akbd_cngetc(v, type, data)
	void *v;
	u_int *type;
	int *data;
{
	int key, press, val;
	int s;

	s = splhigh();

	polledkey = -1;
	adb_polling = 1;

	while (polledkey == -1) {
		adb_intr();
		DELAY(10000);				/* XXX */
	}

	adb_polling = 0;
	splx(s);

	key = polledkey;
	press = ADBK_PRESS(key);
	val = ADBK_KEYVAL(key);

	*data = val;
	*type = press ? WSCONS_EVENT_KEY_DOWN : WSCONS_EVENT_KEY_UP;
}

void
akbd_cnpollc(v, on)
	void *v;
	int on;
{
}
