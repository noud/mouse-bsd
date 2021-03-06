/* $NetBSD: isp_netbsd.c,v 1.23 2000/02/12 02:25:28 mjacob Exp $ */
/*
 * Platform (NetBSD) dependent common attachment code for Qlogic adapters.
 * Matthew Jacob <mjacob@nas.nasa.gov>
 */
/*
 * Copyright (C) 1997, 1998, 1999 National Aeronautics & Space Administration
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <dev/ic/isp_netbsd.h>
#include <sys/scsiio.h>

static void ispminphys __P((struct buf *));
static int32_t ispcmd_slow __P((ISP_SCSI_XFER_T *));
static int32_t ispcmd __P((ISP_SCSI_XFER_T *));
static int
ispioctl __P((struct scsipi_link *, u_long, caddr_t, int, struct proc *));

static struct scsipi_device isp_dev = { NULL, NULL, NULL, NULL };
static int isp_poll __P((struct ispsoftc *, ISP_SCSI_XFER_T *, int));
static void isp_watch __P((void *));
static void isp_command_requeue __P((void *));
static void isp_internal_restart __P((void *));

/*
 * Complete attachment of hardware, include subdevices.
 */
void
isp_attach(isp)
	struct ispsoftc *isp;
{

	isp->isp_osinfo._adapter.scsipi_minphys = ispminphys;
	isp->isp_osinfo._adapter.scsipi_ioctl = ispioctl;

	isp->isp_state = ISP_RUNSTATE;
	isp->isp_osinfo._link.scsipi_scsi.channel =
	    (IS_DUALBUS(isp))? 0 : SCSI_CHANNEL_ONLY_ONE;
	isp->isp_osinfo._link.adapter_softc = isp;
	isp->isp_osinfo._link.device = &isp_dev;
	isp->isp_osinfo._link.adapter = &isp->isp_osinfo._adapter;
	isp->isp_osinfo._link.openings = isp->isp_maxcmds;
	TAILQ_INIT(&isp->isp_osinfo.waitq);	/* XXX 2nd Bus? */

	if (IS_FC(isp)) {
		/*
		 * Give it another chance here to come alive...
		 */
		isp->isp_osinfo._adapter.scsipi_cmd = ispcmd;
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_FC_TARG-1;
#ifdef	ISP2100_SCCLUN
		/*
		 * 16 bits worth, but let's be reasonable..
		 */
		isp->isp_osinfo._link.scsipi_scsi.max_lun = 255;
#else
		isp->isp_osinfo._link.scsipi_scsi.max_lun = 15;
#endif
		/* set below */
	} else {
		sdparam *sdp = isp->isp_param;
		isp->isp_osinfo._adapter.scsipi_cmd = ispcmd_slow;
		isp->isp_osinfo._link.scsipi_scsi.max_target = MAX_TARGETS-1;
		isp->isp_osinfo._link.scsipi_scsi.max_lun = 7;
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
		    sdp->isp_initiator_id;
		isp->isp_osinfo.discovered[0] = 1 << sdp->isp_initiator_id;
		if (IS_DUALBUS(isp)) {
			isp->isp_osinfo._link_b = isp->isp_osinfo._link;
			sdp++;
			isp->isp_osinfo.discovered[1] =
			    1 << sdp->isp_initiator_id;
			isp->isp_osinfo._link_b.scsipi_scsi.adapter_target =
			    sdp->isp_initiator_id;
			isp->isp_osinfo._link_b.scsipi_scsi.channel = 1;
		}
	}
	isp->isp_osinfo._link.type = BUS_SCSI;

	/*
	 * Send a SCSI Bus Reset (used to be done as part of attach,
	 * but now left to the OS outer layers).
	 */
	if (IS_SCSI(isp)) {
		int bus = 0;
		(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		if (IS_DUALBUS(isp)) {
			bus++;
			(void) isp_control(isp, ISPCTL_RESET_BUS, &bus);
		}
		SYS_DELAY(2*1000000);
	} else {
		int i, j;
		fcparam *fcp = isp->isp_param;
		delay(2 * 1000000);
		for (j = 0; j < 5; j++) {
			for (i = 0; i < 5; i++) {
				if (isp_control(isp, ISPCTL_FCLINK_TEST, NULL))
					continue;
#ifdef	ISP2100_FABRIC
				/*
				 * Wait extra time to see if the f/w
				 * eventually completed an FLOGI that
				 * will allow us to know we're on a
				 * fabric.
				 */
				if (fcp->isp_onfabric == 0) {
					delay(1 * 1000000);
					continue;
				}
#endif
				break;
			}
			if (fcp->isp_fwstate == FW_READY &&
			    fcp->isp_loopstate >= LOOP_PDB_RCVD) {
				break;
			}
		}
		isp->isp_osinfo._link.scsipi_scsi.adapter_target =
			fcp->isp_loopid;
	}

	/*
	 * Start the watchdog.
	 */
	isp->isp_dogactive = 1;
	timeout(isp_watch, isp, WATCH_INTERVAL * hz);

	/*
	 * And attach children (if any).
	 */
	config_found((void *)isp, &isp->isp_osinfo._link, scsiprint);
	if (IS_DUALBUS(isp)) {
		config_found((void *)isp, &isp->isp_osinfo._link_b, scsiprint);
	}
}

/*
 * minphys our xfers
 *
 * Unfortunately, the buffer pointer describes the target device- not the
 * adapter device, so we can't use the pointer to find out what kind of
 * adapter we are and adjust accordingly.
 */

static void
ispminphys(bp)
	struct buf *bp;
{
	/*
	 * XX: Only the 1020 has a 24 bit limit.
	 */
	if (bp->b_bcount >= (1 << 24)) {
		bp->b_bcount = (1 << 24);
	}
	minphys(bp);
}

static int32_t
ispcmd_slow(xs)
	ISP_SCSI_XFER_T *xs;
{
	sdparam *sdp;
	int tgt, chan, s;
	u_int16_t flags;
	struct ispsoftc *isp = XS_ISP(xs);

	/*
	 * Have we completed discovery for this target on this adapter?
	 */
	tgt = XS_TGT(xs);
	chan = XS_CHANNEL(xs);
	if ((xs->xs_control & XS_CTL_DISCOVERY) != 0 ||
	    (isp->isp_osinfo.discovered[chan] & (1 << tgt)) != 0) {
		return (ispcmd(xs));
	}

	flags = DPARM_DEFAULT;
	if (xs->sc_link->quirks & SDEV_NOSYNC) {
		flags ^= DPARM_SYNC;
#ifdef	DEBUG
	} else {
		printf("%s: channel %d target %d can do SYNC xfers\n",
		    isp->isp_name, chan, tgt);
#endif
	}
	if (xs->sc_link->quirks & SDEV_NOWIDE) {
		flags ^= DPARM_WIDE;
#ifdef	DEBUG
	} else {
		printf("%s: channel %d target %d can do WIDE xfers\n",
		    isp->isp_name, chan, tgt);
#endif
	}
	if (xs->sc_link->quirks & SDEV_NOTAG) {
		flags ^= DPARM_TQING;
#ifdef	DEBUG
	} else {
		printf("%s: channel %d target %d can do TAGGED xfers\n",
		    isp->isp_name, chan, tgt);
#endif
	}
	/*
	 * Okay, we know about this device now,
	 * so mark parameters to be updated for it.
	 */
	s = splbio();
	isp->isp_osinfo.discovered[chan] |= (1 << tgt);
	sdp = isp->isp_param;
	sdp += chan;
	sdp->isp_devparam[tgt].dev_flags = flags;
	sdp->isp_devparam[tgt].dev_update = 1;
	isp->isp_update |= (1 << chan);
	splx(s);
	return (ispcmd(xs));
}

static int
ispioctl(sc_link, cmd, addr, flag, p)
	struct scsipi_link *sc_link;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct ispsoftc *isp = sc_link->adapter_softc;
	int s, chan, retval = ENOTTY;

	switch (cmd) {
	case SCBUSIORESET:
		chan = sc_link->scsipi_scsi.channel;
		s = splbio();
		if (isp_control(isp, ISPCTL_RESET_BUS, &chan))
			retval = EIO;
		else
			retval = 0;
		(void) splx(s);
		break;
	default:
		break;
	}
	return (retval);
}


static int32_t
ispcmd(xs)
	ISP_SCSI_XFER_T *xs;
{
	struct ispsoftc *isp;
	int result, s;

	isp = XS_ISP(xs);
	s = splbio();
	if (isp->isp_state < ISP_RUNSTATE) {
		DISABLE_INTS(isp);
		isp_init(isp);
                if (isp->isp_state != ISP_INITSTATE) {
			ENABLE_INTS(isp);
                        (void) splx(s);
                        XS_SETERR(xs, HBA_BOTCH);
                        return (COMPLETE);
                }
                isp->isp_state = ISP_RUNSTATE;
		ENABLE_INTS(isp);
        }

	/*
	 * Check for queue blockage...
	 */
	if (isp->isp_osinfo.blocked) {
		if (xs->xs_control & XS_CTL_POLL) {
			xs->error = XS_DRIVER_STUFFUP;
			splx(s);
			return (TRY_AGAIN_LATER);
		}
		TAILQ_INSERT_TAIL(&isp->isp_osinfo.waitq, xs, adapter_q);
		splx(s);
		return (SUCCESSFULLY_QUEUED);
	}
	DISABLE_INTS(isp);
	result = ispscsicmd(xs);
	ENABLE_INTS(isp);

	if ((xs->xs_control & XS_CTL_POLL) == 0) {
		switch (result) {
		case CMD_QUEUED:
			result = SUCCESSFULLY_QUEUED;
			break;
		case CMD_EAGAIN:
			result = TRY_AGAIN_LATER;
			break;
		case CMD_RQLATER:
			result = SUCCESSFULLY_QUEUED;
			timeout(isp_command_requeue, xs, hz);
			break;
		case CMD_COMPLETE:
			result = COMPLETE;
			break;
		}
		(void) splx(s);
		return (result);
	}

	switch (result) {
	case CMD_QUEUED:
		result = SUCCESSFULLY_QUEUED;
		break;
	case CMD_RQLATER:
	case CMD_EAGAIN:
		if (XS_NOERR(xs)) {
			xs->error = XS_DRIVER_STUFFUP;
		}
		result = TRY_AGAIN_LATER;
		break;
	case CMD_COMPLETE:
		result = COMPLETE;
		break;

	}
	/*
	 * If we can't use interrupts, poll on completion.
	 */
	if (result == SUCCESSFULLY_QUEUED) {
		if (isp_poll(isp, xs, XS_TIME(xs))) {
			/*
			 * If no other error occurred but we didn't finish,
			 * something bad happened.
			 */
			if (XS_IS_CMD_DONE(xs) == 0) {
				if (isp_control(isp, ISPCTL_ABORT_CMD, xs)) {
					isp_restart(isp);
				}
				if (XS_NOERR(xs)) {
					XS_SETERR(xs, HBA_BOTCH);
				}
			}
		}
		result = COMPLETE;
	}
	(void) splx(s);
	return (result);
}

static int
isp_poll(isp, xs, mswait)
	struct ispsoftc *isp;
	ISP_SCSI_XFER_T *xs;
	int mswait;
{

	while (mswait) {
		/* Try the interrupt handling routine */
		(void)isp_intr((void *)isp);

		/* See if the xs is now done */
		if (XS_IS_CMD_DONE(xs)) {
			return (0);
		}
		SYS_DELAY(1000);	/* wait one millisecond */
		mswait--;
	}
	return (1);
}

static void
isp_watch(arg)
	void *arg;
{
	int i;
	struct ispsoftc *isp = arg;
	struct scsipi_xfer *xs;
	int s;

	/*
	 * Look for completely dead commands (but not polled ones).
	 */
	s = splbio();
	for (i = 0; i < isp->isp_maxcmds; i++) {
		xs = isp->isp_xflist[i];
		if (xs == NULL) {
			continue;
		}
		if (xs->timeout == 0 || (xs->xs_control & XS_CTL_POLL)) {
			continue;
		}
		xs->timeout -= (WATCH_INTERVAL * 1000);
		/*
		 * Avoid later thinking that this
		 * transaction is not being timed.
		 * Then give ourselves to watchdog
		 * periods of grace.
		 */
		if (xs->timeout == 0) {
			xs->timeout = 1;
		} else if (xs->timeout > -(2 * WATCH_INTERVAL * 1000)) {
			continue;
		}
		if (isp_control(isp, ISPCTL_ABORT_CMD, xs)) {
			printf("%s: isp_watch failed to abort command\n",
			    isp->isp_name);
			isp_restart(isp);
			break;
		}
	}
	timeout(isp_watch, isp, WATCH_INTERVAL * hz);
	isp->isp_dogactive = 1;
	(void) splx(s);
}

/*
 * Free any associated resources prior to decommissioning and
 * set the card to a known state (so it doesn't wake up and kick
 * us when we aren't expecting it to).
 *
 * Locks are held before coming here.
 */
void
isp_uninit(isp)
	struct ispsoftc *isp;
{
	ISP_ILOCKVAL_DECL;
	ISP_ILOCK(isp);
	/*
	 * Leave with interrupts disabled.
	 */
	DISABLE_INTS(isp);

	/*
	 * Turn off the watchdog (if active).
	 */
	if (isp->isp_dogactive) {
		untimeout(isp_watch, isp);
		isp->isp_dogactive = 0;
	}

	ISP_IUNLOCK(isp);
}

/*
 * Restart function for a command to be requeued later.
 */
static void
isp_command_requeue(arg)
	void *arg;
{
	struct scsipi_xfer *xs = arg;
	struct ispsoftc *isp = XS_ISP(xs);
	int s = splbio();
	switch (ispcmd_slow(xs)) {
	case SUCCESSFULLY_QUEUED:
		printf("%s: isp_command_requeue: requeued for %d.%d\n",
		    isp->isp_name, XS_TGT(xs), XS_LUN(xs));
		break;
	case TRY_AGAIN_LATER:
		printf("%s: EAGAIN for %d.%d\n",
		    isp->isp_name, XS_TGT(xs), XS_LUN(xs));
		/* FALLTHROUGH */
	case COMPLETE:
		/* can only be an error */
		xs->xs_status |= XS_STS_DONE;
		if (XS_NOERR(xs)) {
			XS_SETERR(xs, HBA_BOTCH);
		}
		scsipi_done(xs);
		break;
	}
	(void) splx(s);
}

/*
 * Restart function after a LOOP UP event (e.g.),
 * done as a timeout for some hysteresis.
 */
static void
isp_internal_restart(arg)
	void *arg;
{
	struct ispsoftc *isp = arg;
	int result, nrestarted = 0, s;

	s = splbio();
	if (isp->isp_osinfo.blocked == 0) {
		struct scsipi_xfer *xs;
		while ((xs = TAILQ_FIRST(&isp->isp_osinfo.waitq)) != NULL) {
			TAILQ_REMOVE(&isp->isp_osinfo.waitq, xs, adapter_q);
			DISABLE_INTS(isp);
			result = ispscsicmd(xs);
			ENABLE_INTS(isp);
			if (result != CMD_QUEUED) {
				printf("%s: botched command restart (0x%x)\n",
				    isp->isp_name, result);
				xs->xs_status |= XS_STS_DONE;
				if (xs->error == XS_NOERROR)
					xs->error = XS_DRIVER_STUFFUP;
				scsipi_done(xs);
			}
			nrestarted++;
		}
		printf("%s: requeued %d commands\n", isp->isp_name, nrestarted);
	}
	(void) splx(s);
}

int
isp_async(isp, cmd, arg)
	struct ispsoftc *isp;
	ispasync_t cmd;
	void *arg;
{
	int bus, tgt;
	int s = splbio();
	switch (cmd) {
	case ISPASYNC_NEW_TGT_PARAMS:
	if (IS_SCSI(isp) && isp->isp_dblev) {
		sdparam *sdp = isp->isp_param;
		char *wt;
		int mhz, flags, period;

		tgt = *((int *) arg);
		bus = (tgt >> 16) & 0xffff;
		tgt &= 0xffff;
		sdp += bus;
		flags = sdp->isp_devparam[tgt].cur_dflags;
		period = sdp->isp_devparam[tgt].cur_period;

		if ((flags & DPARM_SYNC) && period &&
		    (sdp->isp_devparam[tgt].cur_offset) != 0) {
#if	0
			/* CAUSES PANICS */
			static char *m = "%s: bus %d now %s mode\n";
			u_int16_t r, l;
			if (bus == 1)
				r = SXP_PINS_DIFF | SXP_BANK1_SELECT;
			else
				r = SXP_PINS_DIFF;
			l = ISP_READ(isp, r) & ISP1080_MODE_MASK;
			switch (l) {
			case ISP1080_LVD_MODE:
				sdp->isp_lvdmode = 1;
				printf(m, isp->isp_name, bus, "LVD");
				break;
			case ISP1080_HVD_MODE:
				sdp->isp_diffmode = 1;
				printf(m, isp->isp_name, bus, "Differential");
				break;
			case ISP1080_SE_MODE:
				sdp->isp_ultramode = 1;
				printf(m, isp->isp_name, bus, "Single-Ended");
				break;
			default:
				printf("%s: unknown mode on bus %d (0x%x)\n",
				    isp->isp_name, bus, l);
				break;
			}
#endif
			/*
			 * There's some ambiguity about our negotiated speed
			 * if we haven't detected LVD mode correctly (which
			 * seems to happen, unfortunately). If we're in LVD
			 * mode, then different rules apply about speed.
			 */
			if (sdp->isp_lvdmode || period < 0xc) {
				switch (period) {
				case 0x9:
					mhz = 80;
					break;
				case 0xa:
					mhz = 40;
					break;
				case 0xb:
					mhz = 33;
					break;
				case 0xc:
					mhz = 25;
					break;
				default:
					mhz = 1000 / (period * 4);
					break;
				}
			} else {
				mhz = 1000 / (period * 4);
			}
		} else {
			mhz = 0;
		}
		switch (flags & (DPARM_WIDE|DPARM_TQING)) {
		case DPARM_WIDE:
			wt = ", 16 bit wide\n";
			break;
		case DPARM_TQING:
			wt = ", Tagged Queueing Enabled\n";
			break;
		case DPARM_WIDE|DPARM_TQING:
			wt = ", 16 bit wide, Tagged Queueing Enabled\n";
			break;
		default:
			wt = "\n";
			break;
		}
		if (mhz) {
			CFGPRINTF("%s: Bus %d Target %d at %dMHz Max "
			    "Offset %d%s", isp->isp_name, bus, tgt, mhz,
			    sdp->isp_devparam[tgt].cur_offset, wt);
		} else {
			CFGPRINTF("%s: Bus %d Target %d Async Mode%s",
			    isp->isp_name, bus, tgt, wt);
		}
		break;
	}
	case ISPASYNC_BUS_RESET:
		if (arg)
			bus = *((int *) arg);
		else
			bus = 0;
		printf("%s: SCSI bus %d reset detected\n", isp->isp_name, bus);
		break;
	case ISPASYNC_LOOP_DOWN:
		/*
		 * Hopefully we get here in time to minimize the number
		 * of commands we are firing off that are sure to die.
		 */
		isp->isp_osinfo.blocked = 1;
		printf("%s: Loop DOWN\n", isp->isp_name);
		break;
        case ISPASYNC_LOOP_UP:
		isp->isp_osinfo.blocked = 0;
		timeout(isp_internal_restart, isp, 1);
		printf("%s: Loop UP\n", isp->isp_name);
		break;
	case ISPASYNC_PDB_CHANGED:
	if (IS_FC(isp) && isp->isp_dblev) {
		const char *fmt = "%s: Target %d (Loop 0x%x) Port ID 0x%x "
		    "role %s %s\n Port WWN 0x%08x%08x\n Node WWN 0x%08x%08x\n";
		const static char *roles[4] = {
		    "No", "Target", "Initiator", "Target/Initiator"
		};
		char *ptr;
		fcparam *fcp = isp->isp_param;
		int tgt = *((int *) arg);
		struct lportdb *lp = &fcp->portdb[tgt];

		if (lp->valid) {
			ptr = "arrived";
		} else {
			ptr = "disappeared";
		}
		printf(fmt, isp->isp_name, tgt, lp->loopid, lp->portid,
		    roles[lp->roles & 0x3], ptr,
		    (u_int32_t) (lp->port_wwn >> 32),
		    (u_int32_t) (lp->port_wwn & 0xffffffffLL),
		    (u_int32_t) (lp->node_wwn >> 32),
		    (u_int32_t) (lp->node_wwn & 0xffffffffLL));
		break;
	}
#ifdef	ISP2100_FABRIC
	case ISPASYNC_CHANGE_NOTIFY:
		printf("%s: Name Server Database Changed\n", isp->isp_name);
		break;
	case ISPASYNC_FABRIC_DEV:
	{
		int target;
		struct lportdb *lp;
		sns_scrsp_t *resp = (sns_scrsp_t *) arg;
		u_int32_t portid;
		u_int64_t wwn;
		fcparam *fcp = isp->isp_param;

		portid =
		    (((u_int32_t) resp->snscb_port_id[0]) << 16) |
		    (((u_int32_t) resp->snscb_port_id[1]) << 8) |
		    (((u_int32_t) resp->snscb_port_id[2]));
		wwn =
		    (((u_int64_t)resp->snscb_portname[0]) << 56) |
		    (((u_int64_t)resp->snscb_portname[1]) << 48) |
		    (((u_int64_t)resp->snscb_portname[2]) << 40) |
		    (((u_int64_t)resp->snscb_portname[3]) << 32) |
		    (((u_int64_t)resp->snscb_portname[4]) << 24) |
		    (((u_int64_t)resp->snscb_portname[5]) << 16) |
		    (((u_int64_t)resp->snscb_portname[6]) <<  8) |
		    (((u_int64_t)resp->snscb_portname[7]));
		printf("%s: Fabric Device (Type 0x%x)@PortID 0x%x WWN "
		    "0x%08x%08x\n", isp->isp_name, resp->snscb_port_type,
		    portid, ((u_int32_t)(wwn >> 32)),
		    ((u_int32_t)(wwn & 0xffffffff)));
		if (resp->snscb_port_type != 2)
			break;
		for (target = FC_SNS_ID+1; target < MAX_FC_TARG; target++) {
			lp = &fcp->portdb[target];
			if (lp->port_wwn == wwn)
				break;
		}
		if (target < MAX_FC_TARG) {
			break;
		}
		for (target = FC_SNS_ID+1; target < MAX_FC_TARG; target++) {
			lp = &fcp->portdb[target];
			if (lp->port_wwn == 0)
				break;
		}
		if (target == MAX_FC_TARG) {
			printf("%s: no more space for fabric devices\n",
			    isp->isp_name);
			return (-1);
		}
		lp->port_wwn = lp->node_wwn = wwn;
		lp->portid = portid;
		break;
	}
#endif
	default:
		break;
	}
	(void) splx(s);
	return (0);
}
