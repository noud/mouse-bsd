/*	$NetBSD: miivar.h,v 1.16 2000/02/02 17:50:45 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _DEV_MII_MIIVAR_H_
#define	_DEV_MII_MIIVAR_H_

#include <sys/queue.h>

/*
 * Media Independent Interface datat structure defintions.
 */

struct mii_softc;

/*
 * Callbacks from MII layer into network interface device driver.
 */
typedef	int (*mii_readreg_t) __P((struct device *, int, int));
typedef	void (*mii_writereg_t) __P((struct device *, int, int, int));
typedef	void (*mii_statchg_t) __P((struct device *));

/*
 * A network interface driver has one of these structures in its softc.
 * It is the interface from the network interface driver to the MII
 * layer.
 */
struct mii_data {
	struct ifmedia mii_media;	/* media information */
	struct ifnet *mii_ifp;		/* pointer back to network interface */

	int mii_flags;			/* misc. flags; see below */

	/*
	 * For network interfaces with multiple PHYs, a list of all
	 * PHYs is required so they can all be notified when a media
	 * request is made.
	 */
	LIST_HEAD(mii_listhead, mii_softc) mii_phys;
	int mii_instance;

	/*
	 * PHY driver fills this in with active media status.
	 */
	int mii_media_status;
	int mii_media_active;

	/*
	 * Calls from MII layer into network interface driver.
	 */
	mii_readreg_t mii_readreg;
	mii_writereg_t mii_writereg;
	mii_statchg_t mii_statchg;
};
typedef struct mii_data mii_data_t;

/*
 * This call is used by the MII layer to call into the PHY driver
 * to perform a `service request'.
 */
typedef	int (*mii_downcall_t) __P((struct mii_softc *, struct mii_data *, int));

/*
 * This is a call back into the PHY driver made by a `status request'.
 */
typedef void (*mii_statusreq_t) __P((struct mii_softc *));

/*
 * Requests that can be made to the downcall.
 */
#define	MII_TICK	1	/* once-per-second tick */
#define	MII_MEDIACHG	2	/* user changed media; perform the switch */
#define	MII_POLLSTAT	3	/* user requested media status; fill it in */
#define	MII_DOWN	4	/* interface is down */

/*
 * Each PHY driver's softc has one of these as the first member.
 * XXX This would be better named "phy_softc", but this is the name
 * XXX BSDI used, and we would like to have the same interface.
 */
struct mii_softc {
	struct device mii_dev;		/* generic device glue */

	LIST_ENTRY(mii_softc) mii_list;	/* entry on parent's PHY list */

	int mii_phy;			/* our MII address */
	int mii_offset;			/* first PHY, second PHY, etc. */
	int mii_inst;			/* instance for ifmedia */

	mii_downcall_t mii_service;	/* our downcall */
	mii_statusreq_t mii_status;	/* our status request fn */
	struct mii_data *mii_pdata;	/* pointer to parent's mii_data */

	int mii_flags;			/* misc. flags; see below */
	int mii_capabilities;		/* capabilities from BMSR */
	int mii_ticks;			/* MII_TICK counter */
	int mii_active;			/* last active media */
};
typedef struct mii_softc mii_softc_t;

/* mii_flags */
#define	MIIF_INITDONE	0x0001		/* has been initialized (mii_data) */
#define	MIIF_NOISOLATE	0x0002		/* do not isolate the PHY */
#define	MIIF_NOLOOP	0x0004		/* no loopback capability */
#define	MIIF_DOINGAUTO	0x0008		/* doing autonegotiation (mii_softc) */

#define	MIIF_INHERIT_MASK	(MIIF_NOISOLATE|MIIF_NOLOOP)

/*
 * Special `locators' passed to mii_attach().  If one of these is not
 * an `any' value, we look for *that* PHY and configure it.  If both
 * are not `any', that is an error, and mii_attach() will panic.
 */
#define	MII_OFFSET_ANY		-1
#define	MII_PHY_ANY		-1

/*
 * Used to attach a PHY to a parent.
 */
struct mii_attach_args {
	struct mii_data *mii_data;	/* pointer to parent data */
	int mii_phyno;			/* MII address */
	int mii_id1;			/* PHY ID register 1 */
	int mii_id2;			/* PHY ID register 2 */
	int mii_capmask;		/* capability mask from BMSR */
	int mii_flags;			/* flags from parent */
};
typedef struct mii_attach_args mii_attach_args_t;

/*
 * An array of these structures map MII media types to BMCR/ANAR settings.
 */
struct mii_media {
	int	mm_bmcr;		/* BMCR settings for this media */
	int	mm_anar;		/* ANAR settings for this media */
};

#define	MII_MEDIA_NONE		0
#define	MII_MEDIA_10_T		1
#define	MII_MEDIA_10_T_FDX	2
#define	MII_MEDIA_100_T4	3
#define	MII_MEDIA_100_TX	4
#define	MII_MEDIA_100_TX_FDX	5
#define	MII_NMEDIA		6

#ifdef _KERNEL
#include "locators.h"

#define	PHY_READ(p, r) \
	(*(p)->mii_pdata->mii_readreg)((p)->mii_dev.dv_parent, \
	    (p)->mii_phy, (r))

#define	PHY_WRITE(p, r, v) \
	(*(p)->mii_pdata->mii_writereg)((p)->mii_dev.dv_parent, \
	    (p)->mii_phy, (r), (v))

void	mii_attach __P((struct device *, struct mii_data *, int, int,
	    int, int));
void	mii_activate __P((struct mii_data *, enum devact, int, int));
void	mii_detach __P((struct mii_data *, int, int));

int	mii_mediachg __P((struct mii_data *));
void	mii_tick __P((struct mii_data *));
void	mii_pollstat __P((struct mii_data *));
void	mii_down __P((struct mii_data *));

int	mii_phy_activate __P((struct device *, enum devact));
int	mii_phy_detach __P((struct device *, int));

void	mii_phy_add_media __P((struct mii_softc *));
void	mii_phy_delete_media __P((struct mii_softc *));

void	mii_phy_setmedia __P((struct mii_softc *));
int	mii_phy_auto __P((struct mii_softc *, int));
void	mii_phy_reset __P((struct mii_softc *));
void	mii_phy_down __P((struct mii_softc *));

void	mii_phy_status __P((struct mii_softc *));

void	ukphy_status __P((struct mii_softc *));
#endif /* _KERNEL */

#endif /* _DEV_MII_MIIVAR_H_ */
