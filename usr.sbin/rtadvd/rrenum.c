/*	$NetBSD: rrenum.c,v 1.2 1999/07/06 13:02:09 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif /* __FreeBSD__ >= 3 */
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>

#include <arpa/inet.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include "rrenum.h"
#include "if.h"

#define	RR_ISSET_SEGNUM(segnum_bits, segnum) \
	((((segnum_bits)[(segnum) >> 5]) & (1 << ((segnum) & 31))) != 0)
#define	RR_SET_SEGNUM(segnum_bits, segnum) \
	(((segnum_bits)[(segnum) >> 5]) |= (1 << ((segnum) & 31)))

struct rr_operation {
	u_long	rro_seqnum;
	u_long	rro_segnum_bits[8];
};

static struct rr_operation rro;
static int rr_rcvifindex;
static int rrcmd2pco[RPM_PCO_MAX] = {0,
				     SIOCAIFPREFIX_IN6,
				     SIOCCIFPREFIX_IN6,
				     SIOCSGIFPREFIX_IN6
				    };
static int s;

/*
 * Check validity of a Prefix Control Operation(PCO).
 * Return 0 on success, 1 on failure.
 */
static int
rr_pco_check(int len, struct rr_pco_match *rpm)
{
	struct rr_pco_use *rpu, *rpulim;
	int checklen;

	/* rpm->rpm_len must be (4N * 3) as router-renum-05.txt */
	if ((rpm->rpm_len - 3) < 0 || /* must be at least 3 */
	    (rpm->rpm_len - 3) & 0x3) { /* must be multiple of 4 */
		syslog(LOG_WARNING, "<%s> rpm_len %d is not 4N * 3",
		       __FUNCTION__, rpm->rpm_len);
		return 1;
	}
	/* rpm->rpm_code must be valid value */
	switch(rpm->rpm_code) {
	case RPM_PCO_ADD:
	case RPM_PCO_CHANGE:
	case RPM_PCO_SETGLOBAL:
		break;
	default:
		syslog(LOG_WARNING, "<%s> unknown rpm_code %d", __FUNCTION__,
		       rpm->rpm_code);
		return 1;
	}
	/* rpm->rpm_matchlen must be 0 to 128 inclusive */
	if (rpm->rpm_matchlen > 128) {
		syslog(LOG_WARNING, "<%s> rpm_matchlen %d is over 128",
		       __FUNCTION__, rpm->rpm_matchlen);
		return 1;
	}

	/*
	 * rpu->rpu_uselen, rpu->rpu_keeplen, and sum of them must be
	 * between 0 and 128 inclusive
	 */
	for (rpu = (struct rr_pco_use *)(rpm + 1),
	     rpulim = (struct rr_pco_use *)((char *)rpm + len);
	     rpu < rpulim;
	     rpu += 1) {
		checklen = rpu->rpu_uselen;
		checklen += rpu->rpu_keeplen;
		/*
		 * omit these check, because either of rpu_uselen
		 * and rpu_keeplen is unsigned char
		 *  (128 > rpu_uselen > 0)
		 *  (128 > rpu_keeplen > 0)
		 *  (rpu_uselen + rpu_keeplen > 0)
		 */
		if (checklen > 128) {
			syslog(LOG_WARNING, "<%s> sum of rpu_uselen %d and"
			       " rpu_keeplen %d is %d(over 128)",
			       __FUNCTION__, rpu->rpu_uselen,
			       rpu->rpu_keeplen,
			       rpu->rpu_uselen + rpu->rpu_keeplen);
			return 1;
		}
	}
	return 0;
}

static void
do_use_prefix(int len, struct rr_pco_match *rpm, struct in6_rrenumreq *irr) {
	struct rr_pco_use *rpu, *rpulim;

	rpu = (struct rr_pco_use *)(rpm + 1);
	rpulim = (struct rr_pco_use *)((char *)rpm + len);

	if (rpu == rpulim) {
		if (rpm->rpm_code == RPM_PCO_ADD)
			return;

		irr->irr_u_uselen = 0;
		irr->irr_u_keeplen = 0;
		irr->irr_raf_mask_onlink = 0;
		irr->irr_raf_mask_auto = 0;
		irr->irr_vltime = 0;
		irr->irr_pltime = 0;
		memset(&irr->irr_flags, 0, sizeof(irr->irr_flags));
		irr->irr_useprefix.sin6_len = 0; /* let it mean, no addition */
		irr->irr_useprefix.sin6_family = 0;
		irr->irr_useprefix.sin6_addr = in6addr_any;
		if (ioctl(s, rrcmd2pco[rpm->rpm_code], (caddr_t)irr) < 0 &&
		    errno != EADDRNOTAVAIL)
			syslog(LOG_ERR, "<%s> ioctl: %s", __FUNCTION__,
			       strerror(errno));
		return;
	}

	for (rpu = (struct rr_pco_use *)(rpm + 1),
	     rpulim = (struct rr_pco_use *)((char *)rpm + len);
	     rpu < rpulim;
	     rpu += 1) {
		/* init in6_rrenumreq fields */
		irr->irr_u_uselen = rpu->rpu_uselen;
		irr->irr_u_keeplen = rpu->rpu_keeplen;
		irr->irr_raf_mask_onlink = rpu->rpu_mask_onlink;
		irr->irr_raf_mask_auto = rpu->rpu_mask_autonomous;
		irr->irr_vltime = rpu->rpu_vltime;
		irr->irr_pltime = rpu->rpu_pltime;
		irr->irr_raf_onlink = rpu->rpu_onlink;
		irr->irr_raf_auto = rpu->rpu_autonomous;
		irr->irr_rrf_decrvalid = rpu->rpu_decr_vltime;
		irr->irr_rrf_decrprefd = rpu->rpu_decr_pltime;
		irr->irr_useprefix.sin6_len = sizeof(irr->irr_useprefix);
		irr->irr_useprefix.sin6_family = AF_INET6;
		irr->irr_useprefix.sin6_addr = rpu->rpu_prefix;

		if (ioctl(s, rrcmd2pco[rpm->rpm_code], (caddr_t)irr) < 0 &&
		    errno != EADDRNOTAVAIL)
			syslog(LOG_ERR, "<%s> ioctl: %s", __FUNCTION__,
			       strerror(errno));
	}
}

/*
 * process a Prefix Control Operation(PCO).
 * return 0 on success, 1 on failure
 */
static int
do_pco(struct icmp6_router_renum *rr, int len, struct rr_pco_match *rpm)
{
	int ifindex = 0;
	struct in6_rrenumreq irr;

	if ((rr_pco_check(len, rpm) != NULL))
		return 1;

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "<%s> socket: %s", __FUNCTION__,
		       strerror(errno));
		exit(1);
	}

	memset(&irr, 0, sizeof(irr));
	irr.irr_origin = PR_ORIG_RR;
	irr.irr_m_len = rpm->rpm_matchlen;
	irr.irr_m_minlen = rpm->rpm_minlen;
	irr.irr_m_maxlen = rpm->rpm_maxlen;
	irr.irr_matchprefix.sin6_len = sizeof(irr.irr_matchprefix);
	irr.irr_matchprefix.sin6_family = AF_INET6;
	irr.irr_matchprefix.sin6_addr = rpm->rpm_prefix;

	while (if_indextoname(++ifindex, irr.irr_name)) {
		/*
		 * if rr_forceapply(A flag) is 0 and IFF_UP is off,
		 * the interface is not applied
		 */
		if (!rr->rr_forceapply &&
		    (iflist[ifindex]->ifm_flags & IFF_UP) == 0)
			continue;
		/* TODO: interface scope check */
		do_use_prefix(len, rpm, &irr);
	}
	if (errno == ENXIO)
		return 0;
	else if (errno) {
		syslog(LOG_ERR, "<%s> if_indextoname: %s", __FUNCTION__,
		       strerror(errno));
		return 1;
	}
	return 0;
}

/*
 * call do_pco() for each Prefix Control Operations(PCOs) in a received
 * Router Renumbering Command packet.
 * return 0 on success, 1 on failure
 */
static int
do_rr(int len, struct icmp6_router_renum *rr)
{
	struct rr_pco_match *rpm;
	char *cp, *lim;

	lim = (char *)rr + len;
	cp = (char *)(rr + 1);
	len -= sizeof(struct icmp6_router_renum);

	/* get iflist block from kernel again, to get up-to-date information */
	init_iflist();

	while (cp < lim) {
		int rpmlen;

		rpm = (struct rr_pco_match *)cp;
		if (len < sizeof(struct rr_pco_match)) {
		    tooshort:
			syslog(LOG_ERR, "<%s> pkt too short. left len = %d. "
			       "gabage at end of pkt?", __FUNCTION__, len);
			return 1;
		}
		rpmlen = rpm->rpm_len << 3;
		if (len < rpmlen)
			goto tooshort;

		if (do_pco(rr, rpmlen, rpm)) {
			syslog(LOG_WARNING, "<%s> invalid PCO", __FUNCTION__);
			goto next;
		}

	    next:
		cp += rpmlen;
		len -= rpmlen;
	}

	return 0;
}

/*
 * check validity of a router renumbering command packet
 * return 0 on success, 1 on failure
 */
static int
rr_command_check(int len, struct icmp6_router_renum *rr, struct in6_addr *from,
		 struct in6_addr *dst)
{
	u_char ntopbuf[INET6_ADDRSTRLEN];

	/* omit rr minimal length check. hope kernel have done it. */
	/* rr_command length check */
	if (len < (sizeof(struct icmp6_router_renum) +
		   sizeof(struct rr_pco_match))) {
		syslog(LOG_ERR,	"<%s> rr_command len %d is too short",
		       __FUNCTION__, len);
		return 1;
	}

	/* destination check. only for multicast. omit unicast check. */
	if (IN6_IS_ADDR_MULTICAST(dst) && !IN6_IS_ADDR_MC_LINKLOCAL(dst) &&
	    !IN6_IS_ADDR_MC_SITELOCAL(dst)) {
		syslog(LOG_ERR,	"<%s> dst mcast addr %s is illegal",
		       __FUNCTION__,
		       inet_ntop(AF_INET6, dst, ntopbuf, INET6_ADDRSTRLEN));
		return 1;
	}

	/* seqnum and segnum check */
	if (rro.rro_seqnum > rr->rr_seqnum) {
		syslog(LOG_WARNING,
		       "<%s> rcvd old seqnum %d from %s",
		       __FUNCTION__, (u_int32_t)ntohl(rr->rr_seqnum),
		       inet_ntop(AF_INET6, from, ntopbuf, INET6_ADDRSTRLEN));
		return 1;
	}
	if (rro.rro_seqnum == rr->rr_seqnum &&
	    rr->rr_test == 0 &&
	    RR_ISSET_SEGNUM(rro.rro_segnum_bits, rr->rr_segnum)) {
		if (rr->rr_reqresult)
			syslog(LOG_WARNING,
			       "<%s> rcvd duped segnum %d from %s",
			       __FUNCTION__, rr->rr_segnum,
			       inet_ntop(AF_INET6, from, ntopbuf,
					 INET6_ADDRSTRLEN));
		return 0;
	}

	/* update seqnum */
	if (rro.rro_seqnum != rr->rr_seqnum) {
		/* then must be "<" */

		/* init rro_segnum_bits */
		memset(rro.rro_segnum_bits, 0,
		       sizeof(rro.rro_segnum_bits));
	}
	rro.rro_seqnum = rr->rr_seqnum;

	return 0;
}

static void
rr_command_input(int len, struct icmp6_router_renum *rr,
		 struct in6_addr *from, struct in6_addr *dst)
{
	/* rr_command validity check */
	if (rr_command_check(len, rr, from, dst))
		goto failed;
	if (rr->rr_test && !rr->rr_reqresult)
		return;

	/* do router renumbering */
	if (do_rr(len, rr)) {
		goto failed;
	}

	/* update segnum */
	RR_SET_SEGNUM(rro.rro_segnum_bits, rr->rr_segnum);

	return;

    failed:
	syslog(LOG_ERR, "<%s> received RR was invalid", __FUNCTION__);
	return;
}

void
rr_input(int len, struct icmp6_router_renum *rr, struct in6_pktinfo *pi,
	 struct sockaddr_in6 *from, struct in6_addr *dst)
{
	u_char ntopbuf[2][INET6_ADDRSTRLEN], ifnamebuf[IFNAMSIZ];

	syslog(LOG_DEBUG,
	       "<%s> RR received from %s to %s on %s",
	       __FUNCTION__,
	       inet_ntop(AF_INET6, &from->sin6_addr,
			 ntopbuf[0], INET6_ADDRSTRLEN),
	       inet_ntop(AF_INET6, &dst, ntopbuf[1], INET6_ADDRSTRLEN),
	       if_indextoname(pi->ipi6_ifindex, ifnamebuf));

	rr_rcvifindex = pi->ipi6_ifindex;

	/* TODO: some consistency check. */

	switch (rr->rr_code) {
	case ICMP6_ROUTER_RENUMBERING_COMMAND:
		rr_command_input(len, rr, &from->sin6_addr, dst);
		/* TODO: send reply msg */
		break;
	case ICMP6_ROUTER_RENUMBERING_RESULT:
		/* RESULT will be processed by rrenumd */
		break;
	case ICMP6_ROUTER_RENUMBERING_SEQNUM_RESET:
		/* TODO: sequence number reset */
		break;
	default:
		syslog(LOG_ERR,	"<%s> received unknown code %d",
		       __FUNCTION__, rr->rr_code);
		break;

	}

	return;
}
