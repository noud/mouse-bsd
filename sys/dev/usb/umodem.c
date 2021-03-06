/*	$NetBSD: umodem.c,v 1.22 2000/02/08 09:18:02 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
 * Carlstedt Research & Technology.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Comm Class spec: http://www.usb.org/developers/data/usbcdc11.pdf
 */

/*
 * TODO:
 * - Add error recovery in various places; the big problem is what
 *   to do in a callback if there is an error.
 * - Implement a Call Device for modems without multiplexed commands.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbcdc.h>

#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/usbdevs.h>
#include <dev/usb/ucomvar.h>

#ifdef UMODEM_DEBUG
#define DPRINTFN(n, x)	if (umodemdebug > (n)) logprintf x
int	umodemdebug = 0;
#else
#define DPRINTFN(n, x)
#endif
#define DPRINTF(x) DPRINTFN(0, x)

struct umodem_softc {
	USBBASEDEVICE		sc_dev;		/* base device */

	usbd_device_handle	sc_udev;	/* USB device */

	int			sc_ctl_iface_no;
	usbd_interface_handle	sc_ctl_iface;	/* control interface */
	int			sc_data_iface_no;
	usbd_interface_handle	sc_data_iface;	/* data interface */

	int			sc_cm_cap;	/* CM capabilities */
	int			sc_acm_cap;	/* ACM capabilities */

	int			sc_cm_over_data;

	usb_cdc_line_state_t	sc_line_state;	/* current line state */
	u_char			sc_dtr;		/* current DTR state */
	u_char			sc_rts;		/* current RTS state */

	device_ptr_t		sc_subdev;	/* ucom device */

	u_char			sc_opening;	/* lock during open */
	u_char			sc_dying;	/* disconnecting */
};

static void	*umodem_get_desc
		__P((usbd_device_handle dev, int type, int subtype));
static usbd_status umodem_set_comm_feature
		__P((struct umodem_softc *sc, int feature, int state));
static usbd_status umodem_set_line_coding
		__P((struct umodem_softc *sc, usb_cdc_line_state_t *state));

static void	umodem_get_caps	__P((usbd_device_handle, int *, int *));

static void	umodem_get_status
		__P((void *, int portno, u_char *lsr, u_char *msr));
static void	umodem_set	__P((void *, int, int, int));
static void	umodem_dtr	__P((struct umodem_softc *, int));
static void	umodem_rts	__P((struct umodem_softc *, int));
static void	umodem_break	__P((struct umodem_softc *, int));
static void	umodem_set_line_state __P((struct umodem_softc *));
static int	umodem_param	__P((void *, int, struct termios *));
static int	umodem_ioctl	__P((void *, int, u_long, caddr_t, int,
				     struct proc *));

static struct ucom_methods umodem_methods = {
	umodem_get_status,
	umodem_set,
	umodem_param,
	umodem_ioctl,
	NULL,
	NULL,
};

USB_DECLARE_DRIVER(umodem);

USB_MATCH(umodem)
{
	USB_MATCH_START(umodem, uaa);
	usb_interface_descriptor_t *id;
	int cm, acm;

	if (uaa->iface == NULL)
		return (UMATCH_NONE);

	id = usbd_get_interface_descriptor(uaa->iface);
	if (id == NULL ||
	    id->bInterfaceClass != UCLASS_CDC ||
	    id->bInterfaceSubClass != USUBCLASS_ABSTRACT_CONTROL_MODEL ||
	    id->bInterfaceProtocol != UPROTO_CDC_AT)
		return (UMATCH_NONE);

	umodem_get_caps(uaa->device, &cm, &acm);
	if (!(cm & USB_CDC_CM_DOES_CM) ||
	    !(cm & USB_CDC_CM_OVER_DATA) ||
	    !(acm & USB_CDC_ACM_HAS_LINE))
		return (UMATCH_NONE);

	return (UMATCH_IFACECLASS_IFACESUBCLASS_IFACEPROTO);
}

USB_ATTACH(umodem)
{
	USB_ATTACH_START(umodem, sc, uaa);
	usbd_device_handle dev = uaa->device;
	usb_interface_descriptor_t *id;
	usb_endpoint_descriptor_t *ed;
	usb_cdc_cm_descriptor_t *cmd;
	char devinfo[1024];
	usbd_status err;
	int data_ifcno;
	int i;
	struct ucom_attach_args uca;

	usbd_devinfo(uaa->device, 0, devinfo);
	USB_ATTACH_SETUP;

	sc->sc_udev = dev;
	sc->sc_ctl_iface = uaa->iface;

	id = usbd_get_interface_descriptor(sc->sc_ctl_iface);
	printf("%s: %s, iclass %d/%d\n", USBDEVNAME(sc->sc_dev),
	       devinfo, id->bInterfaceClass, id->bInterfaceSubClass);
	sc->sc_ctl_iface_no = id->bInterfaceNumber;

	umodem_get_caps(dev, &sc->sc_cm_cap, &sc->sc_acm_cap);

	/* Get the data interface no. */
	cmd = umodem_get_desc(dev, UDESC_CS_INTERFACE, UDESCSUB_CDC_CM);
	if (cmd == NULL) {
		printf("%s: no CM descriptor\n", USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_data_iface_no = data_ifcno = cmd->bDataInterface;

	printf("%s: data interface %d, has %sCM over data, has %sbreak\n",
	       USBDEVNAME(sc->sc_dev), data_ifcno,
	       sc->sc_cm_cap & USB_CDC_CM_OVER_DATA ? "" : "no ",
	       sc->sc_acm_cap & USB_CDC_ACM_HAS_BREAK ? "" : "no ");


	/* Get the data interface too. */
	for (i = 0; i < uaa->nifaces; i++) {
		if (uaa->ifaces[i] != NULL) {
			id = usbd_get_interface_descriptor(uaa->ifaces[i]);
			if (id != NULL && id->bInterfaceNumber == data_ifcno) {
				sc->sc_data_iface = uaa->ifaces[i];
				uaa->ifaces[i] = NULL;
			}
		}
	}
	if (sc->sc_data_iface == NULL) {
		printf("%s: no data interface\n", USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	/*
	 * Find the bulk endpoints.
	 * Iterate over all endpoints in the data interface and take note.
	 */
	uca.bulkin = uca.bulkout = -1;

	id = usbd_get_interface_descriptor(sc->sc_data_iface);
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_data_iface, i);
		if (ed == NULL) {
			printf("%s: no endpoint descriptor for %d\n",
				USBDEVNAME(sc->sc_dev), i);
			goto bad;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_BULK) {
                        uca.bulkin = ed->bEndpointAddress;
                } else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
			   (ed->bmAttributes & UE_XFERTYPE) == UE_BULK) {
                        uca.bulkout = ed->bEndpointAddress;
                }
        }

	if (uca.bulkin == -1) {
		printf("%s: Could not find data bulk in\n",
		       USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (uca.bulkout == -1) {
		printf("%s: Could not find data bulk out\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	if (sc->sc_cm_cap & USB_CDC_CM_OVER_DATA) {
		err = umodem_set_comm_feature(sc, UCDC_ABSTRACT_STATE,
					      UCDC_DATA_MULTIPLEXED);
		if (err) {
			printf("%s: could not set data multiplex mode\n",
			       USBDEVNAME(sc->sc_dev));
			goto bad;
		}
		sc->sc_cm_over_data = 1;
	}

	sc->sc_dtr = -1;

	uca.portno = UCOM_UNK_PORTNO;
	/* bulkin, bulkout set above */
	uca.device = sc->sc_udev;
	uca.iface = sc->sc_data_iface;
	uca.methods = &umodem_methods;
	uca.arg = sc;

	DPRINTF(("umodem_attach: sc=%p\n", sc));
	sc->sc_subdev = config_found_sm(self, &uca, ucomprint, ucomsubmatch);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	USB_ATTACH_SUCCESS_RETURN;

 bad:
	sc->sc_dying = 1;
	USB_ATTACH_ERROR_RETURN;
}

void
umodem_get_caps(dev, cm, acm)
	usbd_device_handle dev;
	int *cm, *acm;
{
	usb_cdc_cm_descriptor_t *cmd;
	usb_cdc_acm_descriptor_t *cad;

	*cm = *acm = 0;

	cmd = umodem_get_desc(dev, UDESC_CS_INTERFACE, UDESCSUB_CDC_CM);
	if (cmd == NULL) {
		DPRINTF(("umodem_get_desc: no CM desc\n"));
		return;
	}
	*cm = cmd->bmCapabilities;

	cad = umodem_get_desc(dev, UDESC_CS_INTERFACE, UDESCSUB_CDC_ACM);
	if (cad == NULL) {
		DPRINTF(("umodem_get_desc: no ACM desc\n"));
		return;
	}
	*acm = cad->bmCapabilities;
}

void
umodem_get_status(addr, portno, lsr, msr)
	void *addr;
	int portno;
	u_char *lsr, *msr;
{
	DPRINTF(("umodem_get_status:\n"));

	if (lsr != NULL)
		*lsr = 0;	/* XXX */
	if (msr != NULL)
		*msr = 0;	/* XXX */
}

int
umodem_param(addr, portno, t)
	void *addr;
	int portno;
	struct termios *t;
{
	struct umodem_softc *sc = addr;
	usbd_status err;
	usb_cdc_line_state_t ls;

	DPRINTF(("umodem_param: sc=%p\n", sc));

	USETDW(ls.dwDTERate, t->c_ospeed);
	if (ISSET(t->c_cflag, CSTOPB))
		ls.bCharFormat = UCDC_STOP_BIT_2;
	else
		ls.bCharFormat = UCDC_STOP_BIT_1;
	if (ISSET(t->c_cflag, PARENB)) {
		if (ISSET(t->c_cflag, PARODD))
			ls.bParityType = UCDC_PARITY_ODD;
		else
			ls.bParityType = UCDC_PARITY_EVEN;
	} else
		ls.bParityType = UCDC_PARITY_NONE;
	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		ls.bDataBits = 5;
		break;
	case CS6:
		ls.bDataBits = 6;
		break;
	case CS7:
		ls.bDataBits = 7;
		break;
	case CS8:
		ls.bDataBits = 8;
		break;
	}

	err = umodem_set_line_coding(sc, &ls);
	if (err) {
		DPRINTF(("umodem_param: err=%s\n", usbd_errstr(err)));
		return (1);
	}
	return (0);
}

int
umodem_ioctl(addr, portno, cmd, data, flag, p)
	void *addr;
	int portno;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct umodem_softc *sc = addr;
	int error;

	if (sc->sc_dying)
		return (EIO);

	DPRINTF(("umodemioctl: cmd=0x%08lx\n", cmd));

	switch (cmd) {
	case USB_GET_CM_OVER_DATA:
		*(int *)data = sc->sc_cm_over_data;
		break;

	case USB_SET_CM_OVER_DATA:
		if (*(int *)data != sc->sc_cm_over_data) {
			/* XXX change it */
		}
		break;

	default:
		DPRINTF(("umodemioctl: unknown\n"));
		error = ENOTTY;
		break;
	}

	return (error);
}

void
umodem_dtr(sc, onoff)
	struct umodem_softc *sc;
	int onoff;
{
	DPRINTF(("umodem_modem: onoff=%d\n", onoff));

	if (sc->sc_dtr == onoff)
		return;
	sc->sc_dtr = onoff;

	umodem_set_line_state(sc);
}

void
umodem_rts(sc, onoff)
	struct umodem_softc *sc;
	int onoff;
{
	DPRINTF(("umodem_modem: onoff=%d\n", onoff));

	if (sc->sc_rts == onoff)
		return;
	sc->sc_rts = onoff;

	umodem_set_line_state(sc);
}

void
umodem_set_line_state(sc)
	struct umodem_softc *sc;
{
	usb_device_request_t req;
	int ls;

	ls = (sc->sc_dtr ? UCDC_LINE_DTR : 0) |
	     (sc->sc_rts ? UCDC_LINE_RTS : 0);
	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_CONTROL_LINE_STATE;
	USETW(req.wValue, ls);
	USETW(req.wIndex, sc->sc_ctl_iface_no);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);

}

void
umodem_break(sc, onoff)
	struct umodem_softc *sc;
	int onoff;
{
	usb_device_request_t req;

	DPRINTF(("umodem_break: onoff=%d\n", onoff));

	if (!(sc->sc_acm_cap & USB_CDC_ACM_HAS_BREAK))
		return;

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SEND_BREAK;
	USETW(req.wValue, onoff ? UCDC_BREAK_ON : UCDC_BREAK_OFF);
	USETW(req.wIndex, sc->sc_ctl_iface_no);
	USETW(req.wLength, 0);

	(void)usbd_do_request(sc->sc_udev, &req, 0);
}

void
umodem_set(addr, portno, reg, onoff)
	void *addr;
	int portno;
	int onoff;
{
	struct umodem_softc *sc = addr;

	switch (reg) {
	case UCOM_SET_DTR:
		umodem_dtr(sc, onoff);
		break;
	case UCOM_SET_RTS:
		umodem_rts(sc, onoff);
		break;
	case UCOM_SET_BREAK:
		umodem_break(sc, onoff);
		break;
	default:
		break;
	}
}

usbd_status
umodem_set_line_coding(sc, state)
	struct umodem_softc *sc;
	usb_cdc_line_state_t *state;
{
	usb_device_request_t req;
	usbd_status err;

	DPRINTF(("umodem_set_line_coding: rate=%d fmt=%d parity=%d bits=%d\n",
		 UGETDW(state->dwDTERate), state->bCharFormat,
		 state->bParityType, state->bDataBits));

	if (memcmp(state, &sc->sc_line_state, UCDC_LINE_STATE_LENGTH) == 0) {
		DPRINTF(("umodem_set_line_coding: already set\n"));
		return (USBD_NORMAL_COMPLETION);
	}

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_LINE_CODING;
	USETW(req.wValue, 0);
	USETW(req.wIndex, sc->sc_ctl_iface_no);
	USETW(req.wLength, UCDC_LINE_STATE_LENGTH);

	err = usbd_do_request(sc->sc_udev, &req, state);
	if (err) {
		DPRINTF(("umodem_set_line_coding: failed, err=%s\n",
			 usbd_errstr(err)));
		return (err);
	}

	sc->sc_line_state = *state;

	return (USBD_NORMAL_COMPLETION);
}

void *
umodem_get_desc(dev, type, subtype)
	usbd_device_handle dev;
	int type;
	int subtype;
{
	usb_descriptor_t *desc;
	usb_config_descriptor_t *cd = usbd_get_config_descriptor(dev);
        uByte *p = (uByte *)cd;
        uByte *end = p + UGETW(cd->wTotalLength);

	while (p < end) {
		desc = (usb_descriptor_t *)p;
		if (desc->bDescriptorType == type &&
		    desc->bDescriptorSubtype == subtype)
			return (desc);
		p += desc->bLength;
	}

	return (0);
}

usbd_status
umodem_set_comm_feature(sc, feature, state)
	struct umodem_softc *sc;
	int feature;
	int state;
{
	usb_device_request_t req;
	usbd_status err;
	usb_cdc_abstract_state_t ast;

	DPRINTF(("umodem_set_comm_feature: feature=%d state=%d\n", feature,
		 state));

	req.bmRequestType = UT_WRITE_CLASS_INTERFACE;
	req.bRequest = UCDC_SET_COMM_FEATURE;
	USETW(req.wValue, feature);
	USETW(req.wIndex, sc->sc_ctl_iface_no);
	USETW(req.wLength, UCDC_ABSTRACT_STATE_LENGTH);
	USETW(ast.wState, state);

	err = usbd_do_request(sc->sc_udev, &req, &ast);
	if (err) {
		DPRINTF(("umodem_set_comm_feature: feature=%d, err=%s\n",
			 feature, usbd_errstr(err)));
		return (err);
	}

	return (USBD_NORMAL_COMPLETION);
}

int
umodem_activate(self, act)
	device_ptr_t self;
	enum devact act;
{
	struct umodem_softc *sc = (struct umodem_softc *)self;
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		if (sc->sc_subdev)
			rv = config_deactivate(sc->sc_subdev);
		break;
	}
	return (rv);
}

USB_DETACH(umodem)
{
	USB_DETACH_START(umodem, sc);
	int rv = 0;

	DPRINTF(("umodem_detach: sc=%p flags=%d\n", sc, flags));

	sc->sc_dying = 1;

	if (sc->sc_subdev != NULL)
		rv = config_detach(sc->sc_subdev, flags);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			   USBDEV(sc->sc_dev));

	return (rv);
}
