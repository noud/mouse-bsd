/*	$NetBSD: if_cue.c,v 1.7 2000/02/17 18:42:21 augustss Exp $	*/
/*
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Bill Paul <wpaul@ee.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/usb/if_cue.c,v 1.4 2000/01/16 22:45:06 wpaul Exp $
 */

/*
 * CATC USB-EL1201A USB to ethernet driver. Used in the CATC Netmate
 * adapters and others.
 *
 * Written by Bill Paul <wpaul@ee.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The CATC USB-EL1201A provides USB ethernet support at 10Mbps. The
 * RX filter uses a 512-bit multicast hash table, single perfect entry
 * for the station address, and promiscuous mode. Unlike the ADMtek
 * and KLSI chips, the CATC ASIC supports read and write combining
 * mode where multiple packets can be transfered using a single bulk
 * transaction, which helps performance a great deal.
 */

/*
 * Ported to NetBSD and somewhat rewritten by Lennart Augustsson.
 */

/*
 * TODO:
 * proper cleanup on errors
 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#if defined(__FreeBSD__)

#include <net/ethernet.h>
#include <machine/clock.h>	/* for DELAY */
#include <sys/bus.h>

#elif defined(__NetBSD__) || defined(__OpenBSD__)

#include <sys/device.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#endif

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>

#if defined(__NetBSD__) || defined(__OpenBSD__)
#include <net/if_ether.h>
#define BPF_MTAP(ifp, m) bpf_mtap((ifp)->if_bpf, (m))
#else
#define BPF_MTAP(ifp, m) bpf_mtap((ifp), (m))
#endif

#if defined(__FreeBSD__) || NBPFILTER > 0
#include <net/bpf.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdevs.h>

#ifdef __FreeBSD__
#include <dev/usb/usb_ethersubr.h>
#endif

#include <dev/usb/if_cuereg.h>
#include <sys/kthread.h>
#include <sys/proc.h>

#ifdef CUE_DEBUG
#define DPRINTF(x)	if (cuedebug) logprintf x
#define DPRINTFN(n,x)	if (cuedebug >= (n)) logprintf x
int	cuedebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

typedef struct cue_ticker_block CTB;

struct cue_ticker_block {
  struct cue_softc *sc;
  int flag;
  } ;

/*
 * Various supported device vendors/products.
 */
static struct cue_type cue_devs[] = {
	{ USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE },
	{ USB_VENDOR_CATC, USB_PRODUCT_CATC_NETMATE2 },
	/*{ USB_VENDOR_BELKIN, USB_PRODUCT_BELKIN_F5U111 },*/
	{ 0, 0 }
};

USB_DECLARE_DRIVER(cue);

static int cue_tx_list_init	__P((struct cue_softc *));
static int cue_rx_list_init	__P((struct cue_softc *));
static int cue_newbuf		__P((struct cue_softc *, struct cue_chain *,
				    struct mbuf *));
static int cue_send		__P((struct cue_softc *, struct mbuf *, int));
static void cue_rxeof		__P((usbd_xfer_handle,
				    usbd_private_handle, usbd_status));
static void cue_txeof		__P((usbd_xfer_handle,
				    usbd_private_handle, usbd_status));
static void cue_start		__P((struct ifnet *));
static int cue_ioctl		__P((struct ifnet *, u_long, caddr_t));
static void cue_init		__P((void *));
static void cue_stop		__P((struct cue_softc *));
static void cue_watchdog		__P((struct ifnet *));

static void cue_setmulti	__P((struct cue_softc *));
static u_int32_t cue_crc	__P((caddr_t));
static void cue_reset		__P((struct cue_softc *));

static int csr_read_1		__P((struct cue_softc *, int));
static int csr_write_1		__P((struct cue_softc *, int, int));
#ifdef CUE_USE_TICKER
static int csr_read_2		__P((struct cue_softc *, int));
#endif
#ifdef notdef
static int csr_write_2		__P((struct cue_softc *, int, int));
#endif
static int cue_mem		__P((struct cue_softc *, int,
				    int, void *, int));
static int cue_getmac		__P((struct cue_softc *, void *));

#ifdef __FreeBSD__
#ifndef lint
static const char rcsid[] =
  "$FreeBSD: src/sys/dev/usb/if_cue.c,v 1.4 2000/01/16 22:45:06 wpaul Exp $";
#endif

static void cue_rxstart		__P((struct ifnet *));
static void cue_shutdown		__P((device_t));

static struct usb_qdat cue_qdat;

static device_method_t cue_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		cue_match),
	DEVMETHOD(device_attach,	cue_attach),
	DEVMETHOD(device_detach,	cue_detach),
	DEVMETHOD(device_shutdown,	cue_shutdown),

	{ 0, 0 }
};

static driver_t cue_driver = {
	"cue",
	cue_methods,
	sizeof(struct cue_softc)
};

static devclass_t cue_devclass;

DRIVER_MODULE(if_cue, uhub, cue_driver, cue_devclass, usbd_driver_load, 0);

#endif /* defined(__FreeBSD__) */

#define CUE_SETBIT(sc, reg, x)				\
	csr_write_1(sc, reg, csr_read_1(sc, reg) | (x))

#define CUE_CLRBIT(sc, reg, x)				\
	csr_write_1(sc, reg, csr_read_1(sc, reg) & ~(x))

static int
csr_read_1(sc, reg)
	struct cue_softc	*sc;
	int			reg;
{
	usb_device_request_t	req;
	usbd_status		err;
	u_int8_t		val = 0;
	int			s;

	s = splusb();

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 1);

	err = usbd_do_request(sc->cue_udev, &req, &val);

	splx(s);

	if (err)
		return (0);

	return (val);
}

#ifdef CUE_USE_TICKER /* the ticker is all that uses this */
static int
csr_read_2(sc, reg)
	struct cue_softc	*sc;
	int			reg;
{
	usb_device_request_t	req;
	usbd_status		err;
	uWord			val;
	int			s;

	s = splusb();

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_READREG;
	USETW(req.wValue, 0);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 2);

	err = usbd_do_request(sc->cue_udev, &req, &val);

	splx(s);

	if (err)
		return (0);

	return (UGETW(val));
}
#endif

static int
csr_write_1(sc, reg, val)
	struct cue_softc	*sc;
	int			reg, val;
{
	usb_device_request_t	req;
	usbd_status		err;
	int			s;

	s = splusb();

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_WRITEREG;
	USETW(req.wValue, val);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->cue_udev, &req, NULL);

	splx(s);

	if (err)
		return (-1);

	return (0);
}

#ifdef notdef
static int
csr_write_2(sc, reg, val)
	struct cue_softc	*sc;
	int			reg, val;
{
	usb_device_request_t	req;
	usbd_status		err;
	int			s;

	s = splusb();

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_WRITEREG;
	USETW(req.wValue, val);
	USETW(req.wIndex, reg);
	USETW(req.wLength, 0);

	err = usbd_do_request(sc->cue_udev, &req, NULL);

	splx(s);

	if (err)
		return (-1);

	return (0);
}
#endif

static int
cue_mem(sc, cmd, addr, buf, len)
	struct cue_softc	*sc;
	int			cmd;
	int			addr;
	void			*buf;
	int			len;
{
	usb_device_request_t	req;
	usbd_status		err;
	int			s;

	s = splusb();

	if (cmd == CUE_CMD_READSRAM)
		req.bmRequestType = UT_READ_VENDOR_DEVICE;
	else
		req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = cmd;
	USETW(req.wValue, 0);
	USETW(req.wIndex, addr);
	USETW(req.wLength, len);

	err = usbd_do_request(sc->cue_udev, &req, buf);

	splx(s);

	if (err)
		return (-1);

	return (0);
}

static int
cue_getmac(sc, buf)
	struct cue_softc	*sc;
	void			*buf;
{
	usb_device_request_t	req;
	usbd_status		err;
	int			s;

	s = splusb();

	req.bmRequestType = UT_READ_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_GET_MACADDR;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, ETHER_ADDR_LEN);

	err = usbd_do_request(sc->cue_udev, &req, buf);

	splx(s);

	if (err) {
		printf("%s: read MAC address failed\n", USBDEVNAME(sc->cue_dev));
		return (-1);
	}

	return (0);
}

#define CUE_POLY	0xEDB88320
#define CUE_BITS	9

static u_int32_t
cue_crc(addr)
	caddr_t			addr;
{
	u_int32_t		idx, bit, data, crc;

	/* Compute CRC for the address value. */
	crc = 0xFFFFFFFF; /* initial value */

	for (idx = 0; idx < 6; idx++) {
		for (data = *addr++, bit = 0; bit < 8; bit++, data >>= 1)
			crc = (crc >> 1) ^ (((crc ^ data) & 1) ? CUE_POLY : 0);
	}

	return (crc & ((1 << CUE_BITS) - 1));
}

static void
cue_setmulti(sc)
	struct cue_softc	*sc;
{
	struct ifnet		*ifp;
	//struct ifmultiaddr	*ifma;
	u_int32_t		h = 0, i;

	ifp = GET_IFP(sc);

	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		for (i = 0; i < CUE_MCAST_TABLE_LEN; i++)
			sc->cue_mctab[i] = 0xFF;
			cue_mem(sc, CUE_CMD_WRITESRAM, CUE_MCAST_TABLE_ADDR,
			    &sc->cue_mctab, CUE_MCAST_TABLE_LEN);
		return;
	}

	/* first, zot all the existing hash bits */
	for (i = 0; i < CUE_MCAST_TABLE_LEN; i++)
		sc->cue_mctab[i] = 0;

#ifdef XXXX
	/* now program new ones */
	for (ifma = ifp->if_multiaddrs.lh_first; ifma != NULL;
	    ifma = ifma->ifma_link.le_next) {
		if (ifma->ifma_addr->sa_family != AF_LINK)
			continue;
		h = cue_crc(LLADDR((struct sockaddr_dl *)ifma->ifma_addr));
		sc->cue_mctab[h >> 3] |= 1 << (h & 0x7);
	}
#endif

	/*
	 * Also include the broadcast address in the filter
	 * so we can receive broadcast frames.
	 */
	if (ifp->if_flags & IFF_BROADCAST) {
		h = cue_crc(etherbroadcastaddr);
		sc->cue_mctab[h >> 3] |= 1 << (h & 0x7);
	}

	cue_mem(sc, CUE_CMD_WRITESRAM, CUE_MCAST_TABLE_ADDR,
	    &sc->cue_mctab, CUE_MCAST_TABLE_LEN);

	return;
}

static void
cue_reset(sc)
	struct cue_softc	*sc;
{
	usb_device_request_t	req;
	usbd_status		err;
	int			s;

	s = splusb();

	req.bmRequestType = UT_WRITE_VENDOR_DEVICE;
	req.bRequest = CUE_CMD_RESET;
	USETW(req.wValue, 0);
	USETW(req.wIndex, 0);
	USETW(req.wLength, 0);
	err = usbd_do_request(sc->cue_udev, &req, NULL);

	splx(s);

	if (err)
		printf("%s: reset failed\n", USBDEVNAME(sc->cue_dev));

	/* Wait a little while for the chip to get its brains in order. */
	DELAY(1000);
	return;
}

/*
 * Probe for a CATC chip.
 */
USB_MATCH(cue)
{
	USB_MATCH_START(cue, uaa);
	struct cue_type			*t;

	if (uaa->iface != NULL)
		return (UMATCH_NONE);

	for (t = cue_devs; t->cue_vid != 0; t++)
		if (uaa->vendor == t->cue_vid && uaa->product == t->cue_did)
			return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
USB_ATTACH(cue)
{
	USB_ATTACH_START(cue, sc, uaa);
	char			devinfo[1024];
	int			s;
	u_char			eaddr[ETHER_ADDR_LEN];
	usbd_device_handle	dev = uaa->device;
	usbd_interface_handle	iface;
	usbd_status		err;
	struct ifnet		*ifp;
	usb_interface_descriptor_t	*id;
	usb_endpoint_descriptor_t	*ed;
	int			i;

#ifdef __FreeBSD__
	bzero(sc, sizeof(struct cue_softc));
#endif

	sc->ticker_flag = 0;

	DPRINTFN(5,(" : cue_attach: sc=%p, dev=%p", sc, dev));

	usbd_devinfo(dev, 0, devinfo);
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->cue_dev), devinfo);

	err = usbd_set_config_no(dev, CUE_CONFIG_NO, 0);
	if (err) {
		printf("%s: setting config no failed\n",
		    USBDEVNAME(sc->cue_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	sc->cue_udev = dev;
	sc->cue_product = uaa->product;
	sc->cue_vendor = uaa->vendor;

	err = usbd_device2interface_handle(dev, CUE_IFACE_IDX, &iface);
	if (err) {
		printf("%s: getting interface handle failed\n",
		    USBDEVNAME(sc->cue_dev));
		USB_ATTACH_ERROR_RETURN;
	}

	sc->cue_iface = iface;
	id = usbd_get_interface_descriptor(iface);

	/* Find endpoints. */
	for (i = 0; i < id->bNumEndpoints; i++) {
		ed = usbd_interface2endpoint_descriptor(iface, i);
		if (ed == NULL) {
			printf("%s: couldn't get ep %d\n",
			    USBDEVNAME(sc->cue_dev), i);
			USB_ATTACH_ERROR_RETURN;
		}
		if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_BULK) {
			sc->cue_ed[CUE_ENDPT_RX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_OUT &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_BULK) {
			sc->cue_ed[CUE_ENDPT_TX] = ed->bEndpointAddress;
		} else if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN &&
		    (ed->bmAttributes & UE_XFERTYPE) == UE_INTERRUPT) {
			sc->cue_ed[CUE_ENDPT_INTR] = ed->bEndpointAddress;
		}
	}

#ifdef notdef
	/* Reset the adapter. */
	cue_reset(sc);
#endif
	/*
	 * Get station address.
	 */
	cue_getmac(sc, &eaddr);

	s = splimp();

	/*
	 * A CATC chip was detected. Inform the world.
	 */
#if defined(__FreeBSD__)
	printf("%s: Ethernet address: %6D\n", USBDEVNAME(sc->cue_dev), eaddr, ":");

	bcopy(eaddr, (char *)&sc->arpcom.ac_enaddr, ETHER_ADDR_LEN);

	ifp = &sc->arpcom.ac_if;
	ifp->if_softc = sc;
	ifp->if_unit = USBDEVNAME(sc->cue_dev);
	ifp->if_name = "cue";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = cue_ioctl;
	ifp->if_output = ether_output;
	ifp->if_start = cue_start;
	ifp->if_watchdog = cue_watchdog;
	ifp->if_init = cue_init;
	ifp->if_baudrate = 10000000;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;

	cue_qdat.ifp = ifp;
	cue_qdat.if_rxstart = cue_rxstart;

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp);
	callout_handle_init(&sc->cue_stat_ch);
	bpfattach(ifp, DLT_EN10MB, sizeof(struct ether_header));
	usb_register_netisr();

#elif defined(__NetBSD__) || defined(__OpenBSD__)

	printf("%s: Ethernet address %s\n", USBDEVNAME(sc->cue_dev),
	    ether_sprintf(eaddr));

	/* Initialize interface info.*/
	ifp = GET_IFP(sc);
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = cue_ioctl;
	ifp->if_start = cue_start;
	ifp->if_watchdog = cue_watchdog;
	ifp->if_baudrate = 10000000;
	strncpy(ifp->if_xname, USBDEVNAME(sc->cue_dev), IFNAMSIZ);

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB,
		  sizeof(struct ether_header));
#endif
#if NRND > 0
	rnd_attach_source(&sc->rnd_source, USBDEVNAME(sc->cue_dev),
	    RND_TYPE_NET, 0);
#endif

#endif /* __NetBSD__ */

	sc->cue_attached = 1;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->cue_udev,
			   USBDEV(sc->cue_dev));

	USB_ATTACH_SUCCESS_RETURN;
}

#ifdef CUE_USE_TICKER

/*
 * I don't know why, but I keep getting "usbd_transfer_cb: short transfer 1<2"
 *  if I let the stats ticker run.  That's why this isn't always on.
 *
 * Oh well, it's only a minor loss.
 */

static void ticker_done(CTB *b, int checksc)
{
 int s;

 s = splimp();
 if (checksc && (b->sc->ticker_flag == &b->flag)) b->sc->ticker_flag = 0;
 free(b,M_DEVBUF);
 splx(s);
 kthread_exit(0);
}

static void cue_ticker(void *ctbv)
{
 CTB *b;
 struct cue_softc *sc;
 int s;
 struct ifnet *i;

 b = ctbv;
 sc = b->sc;
 s = splimp();
 while (1)
  { if (! b->flag) ticker_done(b,0);
    if (sc->cue_dying) ticker_done(b,1);
    i = GET_IFP(sc);
    i->if_collisions += csr_read_2(sc,CUE_TX_SINGLECOLL);
    i->if_collisions += csr_read_2(sc,CUE_TX_MULTICOLL);
    i->if_collisions += csr_read_2(sc,CUE_TX_EXCESSCOLL);
    if (csr_read_2(sc,CUE_RX_FRAMEERR)) i->if_ierrors ++;
    /* Using hz here means one loop per second, which means at most one
       input error noticed per second.  This is arguably wrong, but it's
       clearly what the original code intended to do. */
    tsleep(b,PZERO,"cuetick",hz);
  }
}

static void cue_stop_ticker(struct cue_softc *sc)
{
 int s;

 s = splimp();
 if (sc->ticker_flag)
  { *sc->ticker_flag = 0;
    sc->ticker_flag = 0;
  }
 splx(s);
}

static void cue_start_ticker(struct cue_softc *sc)
{
 int s;

 s = splimp();
 if (! sc->ticker_flag)
  { CTB *b;
    b = malloc(sizeof(CTB),M_DEVBUF,M_NOWAIT);
    if (b == 0) panic("can't malloc cue ticker block");
    b->sc = sc;
    b->flag = 1;
    sc->ticker_flag = &b->flag;
    if (kthread_create1(&cue_ticker,b,0,"%stick",&sc->cue_dev.dv_xname[0])) panic("can't fork %s ticker",&sc->cue_dev.dv_xname[0]);
  }
 splx(s);
}

#else

#define cue_stop_ticker(x) /* nothing */
#define cue_start_ticker(x) /* nothing */

#endif

USB_DETACH(cue)
{
	USB_DETACH_START(cue, sc);
	struct ifnet		*ifp = GET_IFP(sc);
	int			s;

	s = splusb();

	cue_stop_ticker(sc);

	if (!sc->cue_attached) {
		/* Detached before attached finished, so just bail out. */
		splx(s);
		return (0);
	}

	if (ifp->if_flags & IFF_RUNNING)
		cue_stop(sc);

#if defined(__NetBSD__)
#if NRND > 0
	rnd_detach_source(&sc->rnd_source);
#endif
#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	ether_ifdetach(ifp);
#endif /* __NetBSD__ */

	if_detach(ifp);

#ifdef DIAGNOSTIC
	if (sc->cue_ep[CUE_ENDPT_TX] != NULL ||
	    sc->cue_ep[CUE_ENDPT_RX] != NULL ||
	    sc->cue_ep[CUE_ENDPT_INTR] != NULL)
		printf("%s: detach has active endpoints\n",
		       USBDEVNAME(sc->cue_dev));
#endif

	sc->cue_attached = 0;
	splx(s);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->cue_udev,
			   USBDEV(sc->cue_dev));

	return (0);
}

#if defined(__NetBSD__) || defined(__OpenBSD__)
int
cue_activate(self, act)
	device_ptr_t self;
	enum devact act;
{
	struct cue_softc *sc = (struct cue_softc *)self;

	DPRINTFN(2,("%s: %s: enter\n", USBDEVNAME(sc->cue_dev), __FUNCTION__));

	switch (act) {
	case DVACT_ACTIVATE:
		return (EOPNOTSUPP);
		break;

	case DVACT_DEACTIVATE:
		/* Deactivate the interface. */
		if_deactivate(&sc->cue_ec.ec_if);
		sc->cue_dying = 1;
		break;
	}
	return (0);
}
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

/*
 * Initialize an RX descriptor and attach an MBUF cluster.
 */
static int
cue_newbuf(sc, c, m)
	struct cue_softc	*sc;
	struct cue_chain	*c;
	struct mbuf		*m;
{
	struct mbuf		*m_new = NULL;

	if (m == NULL) {
		MGETHDR(m_new, M_DONTWAIT, MT_DATA);
		if (m_new == NULL) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->cue_dev));
			return (ENOBUFS);
		}

		MCLGET(m_new, M_DONTWAIT);
		if (!(m_new->m_flags & M_EXT)) {
			printf("%s: no memory for rx list "
			    "-- packet dropped!\n", USBDEVNAME(sc->cue_dev));
			m_freem(m_new);
			return (ENOBUFS);
		}
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
	} else {
		m_new = m;
		m_new->m_len = m_new->m_pkthdr.len = MCLBYTES;
		m_new->m_data = m_new->m_ext.ext_buf;
	}

	m_adj(m_new, ETHER_ALIGN);
	c->cue_mbuf = m_new;

	return (0);
}

static int
cue_rx_list_init(sc)
	struct cue_softc	*sc;
{
	struct cue_cdata	*cd;
	struct cue_chain	*c;
	int			i;

	cd = &sc->cue_cdata;
	for (i = 0; i < CUE_RX_LIST_CNT; i++) {
		c = &cd->cue_rx_chain[i];
		c->cue_sc = sc;
		c->cue_idx = i;
		if (cue_newbuf(sc, c, NULL) == ENOBUFS)
			return (ENOBUFS);
		if (c->cue_xfer == NULL) {
			c->cue_xfer = usbd_alloc_xfer(sc->cue_udev);
			if (c->cue_xfer == NULL)
				return (ENOBUFS);
			c->cue_buf = usbd_alloc_buffer(c->cue_xfer, CUE_BUFSZ);
			if (c->cue_buf == NULL)
				return (ENOBUFS); /* XXX free xfer */
		}
	}

	return (0);
}

static int
cue_tx_list_init(sc)
	struct cue_softc	*sc;
{
	struct cue_cdata	*cd;
	struct cue_chain	*c;
	int			i;

	cd = &sc->cue_cdata;
	for (i = 0; i < CUE_TX_LIST_CNT; i++) {
		c = &cd->cue_tx_chain[i];
		c->cue_sc = sc;
		c->cue_idx = i;
		c->cue_mbuf = NULL;
		if (c->cue_xfer == NULL) {
			c->cue_xfer = usbd_alloc_xfer(sc->cue_udev);
			if (c->cue_xfer == NULL)
				return (ENOBUFS);
			c->cue_buf = usbd_alloc_buffer(c->cue_xfer, CUE_BUFSZ);
			if (c->cue_buf == NULL)
				return (ENOBUFS);
		}
	}

	return (0);
}

#ifdef __FreeBSD__
static void
cue_rxstart(ifp)
	struct ifnet		*ifp;
{
	struct cue_softc	*sc;
	struct cue_chain	*c;

	sc = ifp->if_softc;
	c = &sc->cue_cdata.cue_rx_chain[sc->cue_cdata.cue_rx_prod];

	if (cue_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		return;
	}

	/* Setup new transfer. */
	usbd_setup_xfer(c->cue_xfer, sc->cue_ep[CUE_ENDPT_RX],
	    c, c->cue_buf, CUE_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY,
	    USBD_NO_TIMEOUT, cue_rxeof);
	usbd_transfer(c->cue_xfer);

	return;
}
#endif

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 */
static void
cue_rxeof(xfer, priv, status)
	usbd_xfer_handle	xfer;
	usbd_private_handle	priv;
	usbd_status		status;
{
	struct cue_chain	*c = priv;
	struct cue_softc	*sc = c->cue_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct mbuf		*m;
	int			total_len = 0;
	u_int16_t		len;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	int			s;
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

	DPRINTFN(10,("%s: %s: enter status=%d\n", USBDEVNAME(sc->cue_dev),
		     __FUNCTION__, status));

	if (sc->cue_dying)
		return;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED)
			return;
		printf("%s: usb error on rx: %s\n", USBDEVNAME(sc->cue_dev),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->cue_ep[CUE_ENDPT_RX]);
		goto done;
	}

	usbd_get_xfer_status(xfer, NULL, NULL, &total_len, NULL);

	memcpy(mtod(c->cue_mbuf, char *), c->cue_buf, total_len);

	m = c->cue_mbuf;
	len = UGETW(mtod(m, u_int8_t *));

	/* No errors; receive the packet. */
	total_len = len;

	if (len < sizeof(struct ether_header)) {
		ifp->if_ierrors++;
		goto done;
	}

	ifp->if_ipackets++;
	m_adj(m, sizeof(u_int16_t));
	m->m_pkthdr.len = m->m_len = total_len;

#if defined(__FreeBSD__)
	m->m_pkthdr.rcvif = (struct ifnet *)&cue_qdat;
	/* Put the packet on the special USB input queue. */
	usb_ether_input(m);

	return;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	m->m_pkthdr.rcvif = ifp;

	s = splimp();

	/* XXX ugly */
	if (cue_newbuf(sc, c, NULL) == ENOBUFS) {
		ifp->if_ierrors++;
		goto done1;
	}

	/*
	 * Handle BPF listeners. Let the BPF user see the packet, but
	 * don't pass it up to the ether_input() layer unless it's
	 * a broadcast packet, multicast packet, matches our ethernet
	 * address or the interface is in promiscuous mode.
	 */
	if (ifp->if_bpf) {
		struct ether_header *eh = mtod(m, struct ether_header *);
		BPF_MTAP(ifp, m);
		if ((ifp->if_flags & IFF_PROMISC) &&
		    memcmp(eh->ether_dhost, LLADDR(ifp->if_sadl),
			   ETHER_ADDR_LEN) &&
		    !(eh->ether_dhost[0] & 1)) {
			m_freem(m);
			goto done1;
		}
	}

	DPRINTFN(10,("%s: %s: deliver %d\n", USBDEVNAME(sc->cue_dev),
		    __FUNCTION__, m->m_len));
	(*ifp->if_input)(ifp, m);
 done1:
	splx(s);
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

done:
	/* Setup new transfer. */
	usbd_setup_xfer(c->cue_xfer, sc->cue_ep[CUE_ENDPT_RX],
	    c, c->cue_buf, CUE_BUFSZ, USBD_SHORT_XFER_OK | USBD_NO_COPY,
	    USBD_NO_TIMEOUT, cue_rxeof);
	usbd_transfer(c->cue_xfer);

	DPRINTFN(10,("%s: %s: start rx\n", USBDEVNAME(sc->cue_dev),
		    __FUNCTION__));
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */

static void
cue_txeof(xfer, priv, status)
	usbd_xfer_handle	xfer;
	usbd_private_handle	priv;
	usbd_status		status;
{
	struct cue_chain	*c = priv;
	struct cue_softc	*sc = c->cue_sc;
	struct ifnet		*ifp = GET_IFP(sc);
	int			s;

	if (sc->cue_dying)
		return;

	s = splimp();

	DPRINTFN(10,("%s: %s: enter status=%d\n", USBDEVNAME(sc->cue_dev),
		    __FUNCTION__, status));

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	if (status != USBD_NORMAL_COMPLETION) {
		if (status == USBD_NOT_STARTED || status == USBD_CANCELLED) {
			splx(s);
			return;
		}
		ifp->if_oerrors++;
		printf("%s: usb error on tx: %s\n", USBDEVNAME(sc->cue_dev),
		    usbd_errstr(status));
		if (status == USBD_STALLED)
			usbd_clear_endpoint_stall(sc->cue_ep[CUE_ENDPT_TX]);
		splx(s);
		return;
	}

	ifp->if_opackets++;

#if defined(__FreeBSD__)
	c->cue_mbuf->m_pkthdr.rcvif = ifp;
	usb_tx_done(c->cue_mbuf);
	c->cue_mbuf = NULL;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	m_freem(c->cue_mbuf);
	c->cue_mbuf = NULL;

	if (ifp->if_snd.ifq_head != NULL)
		cue_start(ifp);
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

	splx(s);
}

static int
cue_send(sc, m, idx)
	struct cue_softc	*sc;
	struct mbuf		*m;
	int			idx;
{
	int			total_len;
	struct cue_chain	*c;
	usbd_status		err;

	DPRINTFN(10,("%s: %s: enter\n", USBDEVNAME(sc->cue_dev),__FUNCTION__));

	c = &sc->cue_cdata.cue_tx_chain[idx];

	/*
	 * Copy the mbuf data into a contiguous buffer, leaving two
	 * bytes at the beginning to hold the frame length.
	 */
	m_copydata(m, 0, m->m_pkthdr.len, c->cue_buf + 2);
	c->cue_mbuf = m;

	total_len = m->m_pkthdr.len + 2;

	/* The first two bytes are the frame length */
	c->cue_buf[0] = (u_int8_t)m->m_pkthdr.len;
	c->cue_buf[1] = (u_int8_t)(m->m_pkthdr.len >> 8);

	usbd_setup_xfer(c->cue_xfer, sc->cue_ep[CUE_ENDPT_TX],
	    c, c->cue_buf, total_len, USBD_NO_COPY, 10000, cue_txeof);

	/* Transmit */
	err = usbd_transfer(c->cue_xfer);
	if (err != USBD_IN_PROGRESS) {
		cue_stop(sc);
		return (EIO);
	}

	sc->cue_cdata.cue_tx_cnt++;

	return (0);
}

static void
cue_start(ifp)
	struct ifnet		*ifp;
{
	struct cue_softc	*sc = ifp->if_softc;
	struct mbuf		*m_head = NULL;

	if (sc->cue_dying)
		return;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	IF_DEQUEUE(&ifp->if_snd, m_head);
	if (m_head == NULL)
		return;

	if (cue_send(sc, m_head, 0)) {
		IF_PREPEND(&ifp->if_snd, m_head);
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	/*
	 * If there's a BPF listener, bounce a copy of this frame
	 * to him.
	 */
	if (ifp->if_bpf)
		BPF_MTAP(ifp, m_head);

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

static void
cue_init(xsc)
	void			*xsc;
{
	struct cue_softc	*sc = xsc;
	struct ifnet		*ifp = GET_IFP(sc);
	struct cue_chain	*c;
	usbd_status		err;
	int			i, s;
	u_char			*eaddr;

	if (sc->cue_dying)
		return;

	if (ifp->if_flags & IFF_RUNNING)
		return;

	s = splimp();

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
#ifdef foo
	cue_reset(sc);
#endif

#if defined(__FreeBSD__)
	eaddr = sc->arpcom.ac_enaddr;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	eaddr = LLADDR(ifp->if_sadl);
#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */
	/* Set MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		csr_write_1(sc, CUE_PAR0 - i, eaddr[i]);

	/* Enable RX logic. */
	csr_write_1(sc, CUE_ETHCTL, CUE_ETHCTL_RX_ON|CUE_ETHCTL_MCAST_ON);

	 /* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC)
		CUE_SETBIT(sc, CUE_ETHCTL, CUE_ETHCTL_PROMISC);
	else
		CUE_CLRBIT(sc, CUE_ETHCTL, CUE_ETHCTL_PROMISC);

	/* Init TX ring. */
	if (cue_tx_list_init(sc) == ENOBUFS) {
		printf("%s: tx list init failed\n", USBDEVNAME(sc->cue_dev));
		splx(s);
		return;
	}

	/* Init RX ring. */
	if (cue_rx_list_init(sc) == ENOBUFS) {
		printf("%s: rx list init failed\n", USBDEVNAME(sc->cue_dev));
		splx(s);
		return;
	}

	/* Load the multicast filter. */
	cue_setmulti(sc);

	/*
	 * Set the number of RX and TX buffers that we want
	 * to reserve inside the ASIC.
	 */
	csr_write_1(sc, CUE_RX_BUFPKTS, CUE_RX_FRAMES);
	csr_write_1(sc, CUE_TX_BUFPKTS, CUE_TX_FRAMES);

	/* Set advanced operation modes. */
	csr_write_1(sc, CUE_ADVANCED_OPMODES,
	    CUE_AOP_EMBED_RXLEN|0x01); /* 1 wait state */

	/* Program the LED operation. */
	csr_write_1(sc, CUE_LEDCTL, CUE_LEDCTL_FOLLOW_LINK);

	if (sc->cue_ep[CUE_ENDPT_RX] == NULL) {
	/* Open RX and TX pipes. */
	err = usbd_open_pipe(sc->cue_iface, sc->cue_ed[CUE_ENDPT_RX],
	    USBD_EXCLUSIVE_USE, &sc->cue_ep[CUE_ENDPT_RX]);
	if (err) {
		printf("%s: open rx pipe failed: %s\n",
		    USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		splx(s);
		return;
	}
	err = usbd_open_pipe(sc->cue_iface, sc->cue_ed[CUE_ENDPT_TX],
	    USBD_EXCLUSIVE_USE, &sc->cue_ep[CUE_ENDPT_TX]);
	if (err) {
		printf("%s: open tx pipe failed: %s\n",
		    USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		splx(s);
		return;
	}

	/* Start up the receive pipe. */
	for (i = 0; i < CUE_RX_LIST_CNT; i++) {
		c = &sc->cue_cdata.cue_rx_chain[i];
		usbd_setup_xfer(c->cue_xfer, sc->cue_ep[CUE_ENDPT_RX],
		    c, c->cue_buf, CUE_BUFSZ,
		    USBD_SHORT_XFER_OK | USBD_NO_COPY, USBD_NO_TIMEOUT,
		    cue_rxeof);
		usbd_transfer(c->cue_xfer);
	}
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);

	cue_start_ticker(sc);
}

static int
cue_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	struct cue_softc	*sc = ifp->if_softc;
#if defined(__NetBSD__) || defined(__OpenBSD__)
	struct ifaddr 		*ifa = (struct ifaddr *)data;
	struct ifreq		*ifr = (struct ifreq *)data;
#endif
	int			s, error = 0;

	if (sc->cue_dying)
		return (EIO);

	s = splimp();

	switch(command) {
#if defined(__FreeBSD__)
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
		error = ether_ioctl(ifp, command, data);
		break;
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		cue_init(sc);

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif /* INET */
#ifdef NS
		case AF_NS:
		    {
			struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
					LLADDR(ifp->if_sadl);
			else
				memcpy(LLADDR(ifp->if_sadl),
				       ina->x_host.c_host,
				       ifp->if_addrlen);
			break;
		    }
#endif /* NS */
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

#endif /* defined(__NetBSD__) || defined(__OpenBSD__) */

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING &&
			    ifp->if_flags & IFF_PROMISC &&
			    !(sc->cue_if_flags & IFF_PROMISC)) {
				CUE_SETBIT(sc, CUE_ETHCTL, CUE_ETHCTL_PROMISC);
				cue_setmulti(sc);
			} else if (ifp->if_flags & IFF_RUNNING &&
			    !(ifp->if_flags & IFF_PROMISC) &&
			    sc->cue_if_flags & IFF_PROMISC) {
				CUE_CLRBIT(sc, CUE_ETHCTL, CUE_ETHCTL_PROMISC);
				cue_setmulti(sc);
			} else if (!(ifp->if_flags & IFF_RUNNING))
				cue_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				cue_stop(sc);
		}
		sc->cue_if_flags = ifp->if_flags;
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		cue_setmulti(sc);
		error = 0;
		break;
	default:
		error = EINVAL;
		break;
	}

	splx(s);

	return (error);
}

static void
cue_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct cue_softc	*sc = ifp->if_softc;

	DPRINTFN(5,("%s: %s: enter\n", USBDEVNAME(sc->cue_dev),__FUNCTION__));

	if (sc->cue_dying)
		return;

	ifp->if_oerrors++;
	printf("%s: watchdog timeout\n", USBDEVNAME(sc->cue_dev));

	/*
	 * The polling business is a kludge to avoid allowing the
	 * USB code to call tsleep() in usbd_delay_ms(), which will
	 * kill us since the watchdog routine is invoked from
	 * interrupt context.
	 */
	usbd_set_polling(sc->cue_udev, 1);
	cue_stop(sc);
	cue_init(sc);
	usbd_set_polling(sc->cue_udev, 0);

	if (ifp->if_snd.ifq_head != NULL)
		cue_start(ifp);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
static void
cue_stop(sc)
	struct cue_softc	*sc;
{
	usbd_status		err;
	struct ifnet		*ifp;
	int			i;

	ifp = GET_IFP(sc);
	ifp->if_timer = 0;

	csr_write_1(sc, CUE_ETHCTL, 0);
	cue_reset(sc);
	cue_stop_ticker(sc);

	/* Stop transfers. */
	if (sc->cue_ep[CUE_ENDPT_RX] != NULL) {
		err = usbd_abort_pipe(sc->cue_ep[CUE_ENDPT_RX]);
		if (err) {
			printf("%s: abort rx pipe failed: %s\n",
			USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->cue_ep[CUE_ENDPT_RX]);
		if (err) {
			printf("%s: close rx pipe failed: %s\n",
			USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		sc->cue_ep[CUE_ENDPT_RX] = NULL;
	}

	if (sc->cue_ep[CUE_ENDPT_TX] != NULL) {
		err = usbd_abort_pipe(sc->cue_ep[CUE_ENDPT_TX]);
		if (err) {
			printf("%s: abort tx pipe failed: %s\n",
			USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->cue_ep[CUE_ENDPT_TX]);
		if (err) {
			printf("%s: close tx pipe failed: %s\n",
			    USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		sc->cue_ep[CUE_ENDPT_TX] = NULL;
	}

	if (sc->cue_ep[CUE_ENDPT_INTR] != NULL) {
		err = usbd_abort_pipe(sc->cue_ep[CUE_ENDPT_INTR]);
		if (err) {
			printf("%s: abort intr pipe failed: %s\n",
			USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		err = usbd_close_pipe(sc->cue_ep[CUE_ENDPT_INTR]);
		if (err) {
			printf("%s: close intr pipe failed: %s\n",
			    USBDEVNAME(sc->cue_dev), usbd_errstr(err));
		}
		sc->cue_ep[CUE_ENDPT_INTR] = NULL;
	}

	/* Free RX resources. */
	for (i = 0; i < CUE_RX_LIST_CNT; i++) {
		if (sc->cue_cdata.cue_rx_chain[i].cue_mbuf != NULL) {
			m_freem(sc->cue_cdata.cue_rx_chain[i].cue_mbuf);
			sc->cue_cdata.cue_rx_chain[i].cue_mbuf = NULL;
		}
		if (sc->cue_cdata.cue_rx_chain[i].cue_xfer != NULL) {
			usbd_free_xfer(sc->cue_cdata.cue_rx_chain[i].cue_xfer);
			sc->cue_cdata.cue_rx_chain[i].cue_xfer = NULL;
		}
	}

	/* Free TX resources. */
	for (i = 0; i < CUE_TX_LIST_CNT; i++) {
		if (sc->cue_cdata.cue_tx_chain[i].cue_mbuf != NULL) {
			m_freem(sc->cue_cdata.cue_tx_chain[i].cue_mbuf);
			sc->cue_cdata.cue_tx_chain[i].cue_mbuf = NULL;
		}
		if (sc->cue_cdata.cue_tx_chain[i].cue_xfer != NULL) {
			usbd_free_xfer(sc->cue_cdata.cue_tx_chain[i].cue_xfer);
			sc->cue_cdata.cue_tx_chain[i].cue_xfer = NULL;
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	return;
}

#ifdef __FreeBSD__
/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
static void
cue_shutdown(dev)
	device_t		dev;
{
	struct cue_softc	*sc;

	sc = device_get_softc(dev);

	cue_reset(sc);
	cue_stop(sc);
}
#endif
