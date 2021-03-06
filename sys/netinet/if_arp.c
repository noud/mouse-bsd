/*	$NetBSD: if_arp.c,v 1.66 1999/09/25 17:49:29 is Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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

/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)if_ether.c	8.2 (Berkeley) 9/26/94
 */

/*
 * Ethernet address resolution protocol.
 * TODO:
 *	add "inuse/lock" bit (or ref. count) along with valid bit
 */

#include "opt_ddb.h"
#include "opt_inet.h"

#ifdef INET

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/domain.h>

#include <net/ethertypes.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_token.h>
#include <net/if_types.h>
#include <net/route.h>


#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>

#include "loop.h"
#include "arc.h"
#if NARC > 0
#include <net/if_arc.h>
#endif
#include "fddi.h"
#if NFDDI > 0
#include <net/if_fddi.h>
#endif
#include "token.h"
#include "token.h"

#define SIN(s) ((struct sockaddr_in *)s)
#define SDL(s) ((struct sockaddr_dl *)s)
#define SRP(s) ((struct sockaddr_inarp *)s)

/*
 * ARP trailer negotiation.  Trailer protocol is not IP specific,
 * but ARP request/response use IP addresses.
 */
#define ETHERTYPE_IPTRAILERS ETHERTYPE_TRAIL

/* timer values */
int	arpt_prune = (5*60*1);	/* walk list every 5 minutes */
int	arpt_keep = (20*60);	/* once resolved, good for 20 more minutes */
int	arpt_down = 20;		/* once declared down, don't send for 20 secs */
#define	rt_expire rt_rmx.rmx_expire

static	void arprequest __P((struct ifnet *,
	    struct in_addr *, struct in_addr *, u_int8_t *));
static	void arptfree __P((struct llinfo_arp *));
static	void arptimer __P((void *));
static	struct llinfo_arp *arplookup __P((struct in_addr *, int, struct ifnet *));
static	void in_arpinput __P((struct mbuf *));

#if NLOOP > 0
extern	struct ifnet loif[NLOOP];
#endif
LIST_HEAD(, llinfo_arp) llinfo_arp;
struct	ifqueue arpintrq = {0, 0, 0, 50};
int	arp_inuse, arp_allocated, arp_intimer;
int	arp_maxtries = 5;
int	useloopback = 1;	/* use loopback interface for local traffic */
int	arpinit_done = 0;

/* revarp state */
static struct	in_addr myip, srv_ip;
static int	myip_initialized = 0;
static int	revarp_in_progress = 0;
static struct	ifnet *myip_ifp = NULL;

#ifdef DDB
static void db_print_sa __P((struct sockaddr *));
static void db_print_ifa __P((struct ifaddr *));
static void db_print_llinfo __P((caddr_t));
static int db_show_radix_node __P((struct radix_node *, void *));
#endif

/*
 * this should be elsewhere.
 */

static char *
lla_snprintf __P((u_int8_t *, int));

static char *
lla_snprintf(adrp, len)
	u_int8_t *adrp;
	int len;
{
 static char bufs[4][16*3];
 static int bufx = 0;
	static const char hexdigits[] = {
	    '0','1','2','3','4','5','6','7',
	    '8','9','a','b','c','d','e','f'
	};

	int i;
	char *p;
 char *p0;

 p0 = &bufs[bufx][0];
 p = p0;
 bufx = ((bufx > 0) ? : (sizeof(bufs)/sizeof(bufs[0]))) - 1;

	*p++ = hexdigits[(*adrp)>>4];
	*p++ = hexdigits[(*adrp++)&0xf];

	for (i=1; i<len && i<16; i++) {
		*p++ = ':';
		*p++ = hexdigits[(*adrp)>>4];
		*p++ = hexdigits[(*adrp++)&0xf];
	}

	*p = 0;
 return(p0);
}

struct protosw arpsw[] = {
	{ 0, 0, 0, 0,
	  0, 0, 0, 0,
	  0,
	  0, 0, 0, arp_drain,
	}
};


struct domain arpdomain =
{ 	PF_ARP,  "arp", 0, 0, 0,
	arpsw, &arpsw[sizeof(arpsw)/sizeof(arpsw[0])]
};

/*
 * ARP table locking.
 *
 * to prevent lossage vs. the arp_drain routine (which may be called at
 * any time, including in a device driver context), we do two things:
 *
 * 1) manipulation of la->la_hold is done at splimp() (for all of
 * about two instructions).
 *
 * 2) manipulation of the arp table's linked list is done under the
 * protection of the ARP_LOCK; if arp_drain() or arptimer is called
 * while the arp table is locked, we punt and try again later.
 */

int	arp_locked;

static __inline int arp_lock_try __P((int));
static __inline void arp_unlock __P((void));

static __inline int
arp_lock_try(int recurse)
{
	int s;

	s = splimp();
	if (!recurse && arp_locked) {
		splx(s);
		return (0);
	}
	arp_locked++;
	splx(s);
	return (1);
}

static __inline void
arp_unlock()
{
	int s;

	s = splimp();
	arp_locked--;
	splx(s);
}

#ifdef DIAGNOSTIC
#define	ARP_LOCK(recurse)						\
do {									\
	if (arp_lock_try(recurse) == 0) {				\
		printf("%s:%d: arp already locked\n", __FILE__, __LINE__); \
		panic("arp_lock");					\
	}								\
} while (0)
#define	ARP_LOCK_CHECK()						\
do {									\
	if (arp_locked == 0) {						\
		printf("%s:%d: arp lock not held\n", __FILE__, __LINE__); \
		panic("arp lock check");				\
	}								\
} while (0)
#else
#define	ARP_LOCK(x)		(void) arp_lock_try(x)
#define	ARP_LOCK_CHECK()	/* nothing */
#endif

#define	ARP_UNLOCK()		arp_unlock()

/*
 * ARP protocol drain routine.  Called when memory is in short supply.
 * Called at splimp();
 */

void
arp_drain()
{
	register struct llinfo_arp *la, *nla;
	int count = 0;
	struct mbuf *mold;

	if (arp_lock_try(0) == 0) {
		printf("arp_drain: locked; punting\n");
		return;
	}

	for (la = llinfo_arp.lh_first; la != 0; la = nla) {
		nla = la->la_list.le_next;

		mold = la->la_hold;
		la->la_hold = 0;

		if (mold) {
			m_freem(mold);
			count++;
		}
	}
	ARP_UNLOCK();
}


/*
 * Timeout routine.  Age arp_tab entries periodically.
 */
/* ARGSUSED */
static void
arptimer(arg)
	void *arg;
{
	int s;
	register struct llinfo_arp *la, *nla;

	s = splsoftnet();

	if (arp_lock_try(0) == 0) {
		/* get it later.. */
		splx(s);
		return;
	}

	timeout(arptimer, NULL, arpt_prune * hz);
	for (la = llinfo_arp.lh_first; la != 0; la = nla) {
		register struct rtentry *rt = la->la_rt;

		nla = la->la_list.le_next;
		if (rt->rt_expire && rt->rt_expire <= time.tv_sec)
			arptfree(la); /* timer has expired; clear */
	}

	ARP_UNLOCK();

	splx(s);
}

/*
 * Parallel to llc_rtrequest.
 */
void
arp_rtrequest(req, rt, sa)
	int req;
	register struct rtentry *rt;
	struct sockaddr *sa;
{
	register struct sockaddr *gate = rt->rt_gateway;
	register struct llinfo_arp *la = (struct llinfo_arp *)rt->rt_llinfo;
	static struct sockaddr_dl null_sdl = {sizeof(null_sdl), AF_LINK};
	size_t allocsize;
	struct mbuf *mold;
	int s;

	if (!arpinit_done) {
		arpinit_done = 1;
		/*
		 * We generate expiration times from time.tv_sec
		 * so avoid accidently creating permanent routes.
		 */
		if (time.tv_sec == 0) {
			time.tv_sec++;
		}
		timeout(arptimer, (caddr_t)0, hz);
	}
	if (rt->rt_flags & RTF_GATEWAY)
		return;

	ARP_LOCK(1);		/* we may already be locked here. */

	switch (req) {

	case RTM_ADD:
		/*
		 * XXX: If this is a manually added route to interface
		 * such as older version of routed or gated might provide,
		 * restore cloning bit.
		 */
		if ((rt->rt_flags & RTF_HOST) == 0 &&
		    SIN(rt_mask(rt))->sin_addr.s_addr != 0xffffffff)
			rt->rt_flags |= RTF_CLONING;
		if (rt->rt_flags & RTF_CLONING) {
			/*
			 * Case 1: This route should come from a route to iface.
			 */
			rt_setgate(rt, rt_key(rt),
					(struct sockaddr *)&null_sdl);
			gate = rt->rt_gateway;
			SDL(gate)->sdl_type = rt->rt_ifp->if_type;
			SDL(gate)->sdl_index = rt->rt_ifp->if_index;
			/*
			 * Give this route an expiration time, even though
			 * it's a "permanent" route, so that routes cloned
			 * from it do not need their expiration time set.
			 */
			rt->rt_expire = time.tv_sec;
#if NFDDI > 0
			if (rt->rt_ifp->if_type == IFT_FDDI
			    && (rt->rt_rmx.rmx_mtu > FDDIIPMTU
				|| (rt->rt_rmx.rmx_mtu == 0
				    && rt->rt_ifp->if_mtu > FDDIIPMTU))) {
				rt->rt_rmx.rmx_mtu = FDDIIPMTU;
			}
#endif
#if NARC > 0
			if (rt->rt_ifp->if_type == IFT_ARCNET) {
				int arcipifmtu;

				if (rt->rt_ifp->if_flags & IFF_LINK0)
					arcipifmtu = arc_ipmtu;
				else
					arcipifmtu = ARCMTU;

			    	if (rt->rt_rmx.rmx_mtu > arcipifmtu ||
				    (rt->rt_rmx.rmx_mtu == 0 &&
				     rt->rt_ifp->if_mtu > arcipifmtu))

					rt->rt_rmx.rmx_mtu = arcipifmtu;
			}
#endif
			break;
		}
		/* Announce a new entry if requested. */
		if (rt->rt_flags & RTF_ANNOUNCE)
			arprequest(rt->rt_ifp,
			    &SIN(rt_key(rt))->sin_addr,
			    &SIN(rt_key(rt))->sin_addr,
			    (u_char *)LLADDR(SDL(gate)));
		/*FALLTHROUGH*/
	case RTM_RESOLVE:
		if (gate->sa_family != AF_LINK ||
		    gate->sa_len < sizeof(null_sdl)) {
			log(LOG_DEBUG, "arp_rtrequest: bad gateway value\n");
			break;
		}
		SDL(gate)->sdl_type = rt->rt_ifp->if_type;
		SDL(gate)->sdl_index = rt->rt_ifp->if_index;
		if (la != 0)
			break; /* This happens on a route change */
		/*
		 * Case 2:  This route may come from cloning, or a manual route
		 * add with a LL address.
		 */
		switch (SDL(gate)->sdl_type) {
#if NTOKEN > 0
		case IFT_ISO88025:
			allocsize = sizeof(*la) + sizeof(struct token_rif);
			break;
#endif /* NTOKEN > 0 */
		default:
			allocsize = sizeof(*la);
		}
		R_Malloc(la, struct llinfo_arp *, allocsize);
		rt->rt_llinfo = (caddr_t)la;
		if (la == 0) {
			log(LOG_DEBUG, "arp_rtrequest: malloc failed\n");
			break;
		}
		arp_inuse++, arp_allocated++;
		Bzero(la, allocsize);
		la->la_rt = rt;
		rt->rt_flags |= RTF_LLINFO;
		LIST_INSERT_HEAD(&llinfo_arp, la, la_list);
		if (in_hosteq(SIN(rt_key(rt))->sin_addr,
		    (IA_SIN(rt->rt_ifa))->sin_addr)) {
			/*
			 * This test used to be
			 *	if (loif.if_flags & IFF_UP)
			 * It allowed local traffic to be forced through
			 * the hardware by configuring the loopback down.
			 * However, it causes problems during network
			 * configuration for boards that can't receive
			 * packets they send.  It is now necessary to clear
			 * "useloopback" and remove the route to force
			 * traffic out to the hardware.
			 */
			rt->rt_expire = 0;
			Bcopy(LLADDR(rt->rt_ifp->if_sadl),
			    LLADDR(SDL(gate)),
			    SDL(gate)->sdl_alen =
			    rt->rt_ifp->if_data.ifi_addrlen);
#if NLOOP > 0
			if (useloopback)
				rt->rt_ifp = &loif[0];
#endif
		}
		break;

	case RTM_DELETE:
		if (la == 0)
			break;
		arp_inuse--;
		LIST_REMOVE(la, la_list);
		rt->rt_llinfo = 0;
		rt->rt_flags &= ~RTF_LLINFO;

		s = splimp();
		mold = la->la_hold;
		la->la_hold = 0;
		splx(s);

		if (mold)
			m_freem(mold);

		Free((caddr_t)la);
	}
	ARP_UNLOCK();
}

/*
 * Broadcast an ARP request. Caller specifies:
 *	- arp header source ip address
 *	- arp header target ip address
 *	- arp header source ethernet address
 */
static void
arprequest(ifp, sip, tip, enaddr)
	register struct ifnet *ifp;
	register struct in_addr *sip, *tip;
	register u_int8_t *enaddr;
{
	register struct mbuf *m;
	struct arphdr *ah;
	struct sockaddr sa;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_data.ifi_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_data.ifi_addrlen;	/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REQUEST);
	bcopy((caddr_t)enaddr, (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy((caddr_t)sip, (caddr_t)ar_spa(ah), ah->ar_pln);
	bcopy((caddr_t)tip, (caddr_t)ar_tpa(ah), ah->ar_pln);
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
}

/*
 * Resolve an IP address into an ethernet address.  If success,
 * desten is filled in.  If there is no entry in arptab,
 * set one up and broadcast a request for the IP address.
 * Hold onto this mbuf and resend it once the address
 * is finally resolved.  A return value of 1 indicates
 * that desten has been filled in and the packet should be sent
 * normally; a 0 return indicates that the packet has been
 * taken over here, either now or for later transmission.
 */
int
arpresolve(ifp, rt, m, dst, desten)
	register struct ifnet *ifp;
	register struct rtentry *rt;
	struct mbuf *m;
	register struct sockaddr *dst;
	register u_char *desten;
{
	register struct llinfo_arp *la;
	struct sockaddr_dl *sdl;
	struct mbuf *mold;
	int s;

	if (rt)
		la = (struct llinfo_arp *)rt->rt_llinfo;
	else {
		if ((la = arplookup(&SIN(dst)->sin_addr, 1, 0)) != NULL)
			rt = la->la_rt;
	}
	if (la == 0 || rt == 0) {
		log(LOG_DEBUG, "arpresolve: can't allocate llinfo\n");
		m_freem(m);
		return (0);
	}
	sdl = SDL(rt->rt_gateway);
	/*
	 * Check the address family and length is valid, the address
	 * is resolved; otherwise, try to resolve.
	 */
	if ((rt->rt_expire == 0 || rt->rt_expire > time.tv_sec) &&
	    sdl->sdl_family == AF_LINK && sdl->sdl_alen != 0) {
		bcopy(LLADDR(sdl), desten,
		    min(sdl->sdl_alen, ifp->if_data.ifi_addrlen));
		return 1;
	}
	/*
	 * There is an arptab entry, but no ethernet address
	 * response yet.  Replace the held mbuf with this
	 * latest one.
	 */

	s = splimp();
	mold = la->la_hold;
	la->la_hold = m;
	splx(s);

	if (mold)
		m_freem(mold);


	/*
	 * Re-send the ARP request when appropriate.
	 */
#ifdef	DIAGNOSTIC
	if (rt->rt_expire == 0) {
		/* This should never happen. (Should it? -gwr) */
		printf("arpresolve: unresolved and rt_expire == 0\n");
		/* Set expiration time to now (expired). */
		rt->rt_expire = time.tv_sec;
	}
#endif
	if (rt->rt_expire) {
		rt->rt_flags &= ~RTF_REJECT;
		if (la->la_asked == 0 || rt->rt_expire != time.tv_sec) {
			rt->rt_expire = time.tv_sec;
			if (la->la_asked++ < arp_maxtries)
				arprequest(ifp,
				    &SIN(rt->rt_ifa->ifa_addr)->sin_addr,
				    &SIN(dst)->sin_addr,
				    LLADDR(ifp->if_sadl));
			else {
				rt->rt_flags |= RTF_REJECT;
				rt->rt_expire += arpt_down;
				la->la_asked = 0;
			}
		}
	}
	return (0);
}

/*
 * Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
void
arpintr()
{
	register struct mbuf *m;
	register struct arphdr *ar;
	int s;

	while (arpintrq.ifq_head) {
		s = splimp();
		IF_DEQUEUE(&arpintrq, m);
		splx(s);
		if (m == 0 || (m->m_flags & M_PKTHDR) == 0)
			panic("arpintr");

		if (m->m_len >= sizeof(struct arphdr) &&
		    (ar = mtod(m, struct arphdr *)) &&
		    /* XXX ntohs(ar->ar_hrd) == ARPHRD_ETHER && */
		    m->m_len >=
		      sizeof(struct arphdr) + 2 * (ar->ar_hln + ar->ar_pln))
			switch (ntohs(ar->ar_pro)) {

			case ETHERTYPE_IP:
			case ETHERTYPE_IPTRAILERS:
				in_arpinput(m);
				continue;
			}
		m_freem(m);
	}
}

/*
 * ARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 826.
 * In addition, a sanity check is performed on the sender
 * protocol address, to catch impersonators.
 * We no longer handle negotiations for use of trailer protocol:
 * Formerly, ARP replied for protocol type ETHERTYPE_TRAIL sent
 * along with IP replies if we wanted trailers sent to us,
 * and also sent them in response to IP replies.
 * This allowed either end to announce the desire to receive
 * trailer packets.
 * We no longer reply to requests for ETHERTYPE_TRAIL protocol either,
 * but formerly didn't normally send requests.
 */
static void
in_arpinput(m)
	struct mbuf *m;
{
	struct arphdr *ah;
	register struct ifnet *ifp = m->m_pkthdr.rcvif;
	register struct llinfo_arp *la = 0;
	register struct rtentry  *rt;
	struct in_ifaddr *ia;
	struct sockaddr_dl *sdl;
	struct sockaddr sa;
	struct in_addr isaddr, itaddr, myaddr;
	int op;
	struct mbuf *mold;
	int s;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);
	bcopy((caddr_t)ar_spa(ah), (caddr_t)&isaddr, sizeof (isaddr));
	bcopy((caddr_t)ar_tpa(ah), (caddr_t)&itaddr, sizeof (itaddr));

	/*
	 * If the target IP address is zero, ignore the packet.
	 * This prevents the code below from tring to answer
	 * when we are using IP address zero (booting).
	 */
	if (in_nullhost(itaddr))
		goto out;

	/*
	 * If the source IP address is zero, this is most likely a
	 * confused host trying to use IP address zero. (Windoze?)
	 * XXX: Should we bother trying to reply to these?
	 */
	if (in_nullhost(isaddr))
		goto out;

	/*
	 * Search for a matching interface address
	 * or any address on the interface to use
	 * as a dummy address in the rest of this function
	 */
	INADDR_TO_IA(itaddr, ia);
	while ((ia != NULL) && ia->ia_ifp != m->m_pkthdr.rcvif)
		NEXT_IA_WITH_SAME_ADDR(ia);

	if (ia == NULL) {
		INADDR_TO_IA(isaddr, ia);
		while ((ia != NULL) && ia->ia_ifp != m->m_pkthdr.rcvif)
			NEXT_IA_WITH_SAME_ADDR(ia);

		if (ia == NULL) {
			IFP_TO_IA(ifp, ia);
			if (ia == NULL)
				goto out;
		}
	}

	myaddr = ia->ia_addr.sin_addr;

	if (!bcmp((caddr_t)ar_sha(ah), LLADDR(ifp->if_sadl),
	    ifp->if_data.ifi_addrlen))
		goto out;	/* it's from me, ignore it. */

	if (!bcmp((caddr_t)ar_sha(ah), (caddr_t)ifp->if_broadcastaddr,
	    ifp->if_data.ifi_addrlen)) {
		log(LOG_ERR,
		    "%s: arp: link address is broadcast for IP address %s!\n",
		    ifp->if_xname, in_fmtaddr(isaddr));
		goto out;
	}

	if (in_hosteq(isaddr, myaddr)) {
		log(LOG_ERR,
		   "duplicate IP address %s sent from link address %s\n",
		   in_fmtaddr(isaddr), lla_snprintf(ar_sha(ah), ah->ar_hln));
		itaddr = myaddr;
		goto reply;
	}
	la = arplookup(&isaddr, in_hosteq(itaddr, myaddr), 0);
	if (la && (rt = la->la_rt) && (sdl = SDL(rt->rt_gateway))) {
		if (sdl->sdl_alen &&
		    bcmp((caddr_t)ar_sha(ah), LLADDR(sdl), sdl->sdl_alen)) {
			if (rt->rt_flags & RTF_STATIC) {
				log(LOG_INFO,
				    "%s tried to overwrite permanent arp info"
				    " %s for %s\n",
				    lla_snprintf(ar_sha(ah), ah->ar_hln),
				    lla_snprintf(LLADDR(sdl), sdl->sdl_alen),
				    in_fmtaddr(isaddr));
				goto out;
			} else if (rt->rt_ifp != ifp) {
				log(LOG_INFO,
				    "%s on %s tried to overwrite "
				    "arp info %s for %s on %s\n",
				    lla_snprintf(ar_sha(ah), ah->ar_hln),
				    ifp->if_xname,
				    lla_snprintf(LLADDR(sdl), sdl->sdl_alen),
				    in_fmtaddr(isaddr),
				    rt->rt_ifp->if_xname);
				    goto out;
			} else {
				log(LOG_INFO,
				    "arp info %s overwritten for %s by %s\n",
				    lla_snprintf(LLADDR(sdl), sdl->sdl_alen),
				    in_fmtaddr(isaddr),
				    lla_snprintf(ar_sha(ah), ah->ar_hln));
			}
		}
		/*
		 * sanity check for the address length.
		 * XXX this does not work for protocols with variable address
		 * length. -is
		 */
		if (sdl->sdl_alen &&
		    sdl->sdl_alen != ah->ar_hln) {
			log(LOG_WARNING,
			    "arp from %s: new addr len %d, was %d",
			    in_fmtaddr(isaddr), ah->ar_hln, sdl->sdl_alen);
		}
		if (ifp->if_data.ifi_addrlen != ah->ar_hln) {
			log(LOG_WARNING,
			    "arp from %s: addr len: new %d, i/f %d (ignored)",
			    in_fmtaddr(isaddr), ah->ar_hln,
			    ifp->if_data.ifi_addrlen);
			goto reply;
		}
#if NTOKEN > 0
		/*
		 * XXX uses m_data and assumes the complete answer including
		 * XXX token-ring headers is in the same buf
		 */
		if (ifp->if_type == IFT_ISO88025) {
			struct token_header *trh;

			trh = (struct token_header *)M_TRHSTART(m);
			if (trh->token_shost[0] & TOKEN_RI_PRESENT) {
				struct token_rif	*rif;
				size_t	riflen;

				rif = TOKEN_RIF(trh);
				riflen = (ntohs(rif->tr_rcf) &
				    TOKEN_RCF_LEN_MASK) >> 8;

				if (riflen > 2 &&
				    riflen < sizeof(struct token_rif) &&
				    (riflen & 1) == 0) {
					rif->tr_rcf ^= htons(TOKEN_RCF_DIRECTION);
					rif->tr_rcf &= htons(~TOKEN_RCF_BROADCAST_MASK);
					bcopy(rif, TOKEN_RIF(la), riflen);
				}
			}
		}
#endif /* NTOKEN > 0 */
		bcopy((caddr_t)ar_sha(ah), LLADDR(sdl),
		    sdl->sdl_alen = ah->ar_hln);
		if (rt->rt_expire)
			rt->rt_expire = time.tv_sec + arpt_keep;
		rt->rt_flags &= ~RTF_REJECT;
		la->la_asked = 0;

		s = splimp();
		mold = la->la_hold;
		la->la_hold = 0;
		splx(s);

		if (mold)
			(*ifp->if_output)(ifp, mold, rt_key(rt), rt);
	}
reply:
	if (op != ARPOP_REQUEST) {
	out:
		m_freem(m);
		return;
	}
	if (in_hosteq(itaddr, myaddr)) {
		/* I am the target */
		bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah), ah->ar_hln);
		bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_sha(ah), ah->ar_hln);
	} else {
		la = arplookup(&itaddr, 0, ifp);
		if (la == 0)
			goto out;
		rt = la->la_rt;
		bcopy((caddr_t)ar_sha(ah), (caddr_t)ar_tha(ah), ah->ar_hln);
		sdl = SDL(rt->rt_gateway);
		bcopy(LLADDR(sdl), (caddr_t)ar_sha(ah), ah->ar_hln);
	}

	bcopy((caddr_t)ar_spa(ah), (caddr_t)ar_tpa(ah), ah->ar_pln);
	bcopy((caddr_t)&itaddr, (caddr_t)ar_spa(ah), ah->ar_pln);
	ah->ar_op = htons(ARPOP_REPLY);
	ah->ar_pro = htons(ETHERTYPE_IP); /* let's be sure! */
	m->m_flags &= ~(M_BCAST|M_MCAST); /* never reply by broadcast */
	m->m_len = sizeof(*ah) + (2 * ah->ar_pln) + (2 * ah->ar_hln);
	m->m_pkthdr.len = m->m_len;
	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);
	return;
}

/*
 * Free an arp entry.
 */
static void
arptfree(la)
	register struct llinfo_arp *la;
{
	register struct rtentry *rt = la->la_rt;
	register struct sockaddr_dl *sdl;

	ARP_LOCK_CHECK();

	if (rt == 0)
		panic("arptfree");
	if (rt->rt_refcnt > 0 && (sdl = SDL(rt->rt_gateway)) &&
	    sdl->sdl_family == AF_LINK) {
		sdl->sdl_alen = 0;
		la->la_asked = 0;
		rt->rt_flags &= ~RTF_REJECT;
		return;
	}
	rtrequest(RTM_DELETE, rt_key(rt), (struct sockaddr *)0, rt_mask(rt),
	    0, (struct rtentry **)0);
}

/*
 * Lookup or enter a new address in arptab.
 *
 * Don't complain about "not on local network" if the RTF_GATEWAY route
 *  is also RTF_REJECT; such routes are typically there to block
 *  communication and do not deserve complaining about.
 */
static struct llinfo_arp *arplookup(struct in_addr *addr, int create, struct ifnet *proxyif)
{
	register struct rtentry *rt;
	static struct sockaddr_inarp sin;
	const char *why = 0;

	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_addr = *addr;
	sin.sin_other = proxyif ? proxyif->if_index : 0;
	rt = rtalloc1(sintosa(&sin), create);
	if (rt == 0)
		why = "rtalloc1 failed";
	else {
		rt->rt_refcnt--;
		if (rt->rt_flags & RTF_GATEWAY) {
			if (rt->rt_flags & RTF_REJECT)
				return(0);
			why = "host is not on local network";
		} else if ((rt->rt_flags & RTF_LLINFO) == 0)
			why = "could not allocate llinfo";
		else if (rt->rt_gateway->sa_family != AF_LINK)
			why = "gateway route is not ours";
		else if (proxyif && (rt->rt_ifp != proxyif))
			why = "wrong interface";
		else
			return ((struct llinfo_arp *)rt->rt_llinfo);
	}
	if (create)
		log(LOG_DEBUG, "arplookup: unable to enter address"
		    " for %s (%s)\n",
		    in_fmtaddr(*addr), why);
	return (0);
}

int
arpioctl(cmd, data)
	u_long cmd;
	caddr_t data;
{

	return (EOPNOTSUPP);
}

void
arp_ifinit(ifp, ifa)
	struct ifnet *ifp;
	struct ifaddr *ifa;
{
	struct in_addr *ip;

	/*
	 * Warn the user if another station has this IP address,
	 * but only if the interface IP address is not zero.
	 */
	ip = &IA_SIN(ifa)->sin_addr;
	if (!in_nullhost(*ip))
		arprequest(ifp, ip, ip, LLADDR(ifp->if_sadl));

	ifa->ifa_rtrequest = arp_rtrequest;
	ifa->ifa_flags |= RTF_CLONING;
}

/*
 * Called from 10 Mb/s Ethernet interrupt handlers
 * when ether packet type ETHERTYPE_REVARP
 * is received.  Common length and type checks are done here,
 * then the protocol-specific routine is called.
 */
void
revarpinput(m)
	struct mbuf *m;
{
	struct arphdr *ar;

	if (m->m_len < sizeof(struct arphdr))
		goto out;
	ar = mtod(m, struct arphdr *);
#if 0 /* XXX I don't think we need this... and it will prevent other LL */
	if (ntohs(ar->ar_hrd) != ARPHRD_ETHER)
		goto out;
#endif
	if (m->m_len < sizeof(struct arphdr) + 2 * (ar->ar_hln + ar->ar_pln))
		goto out;
	switch (ntohs(ar->ar_pro)) {

	case ETHERTYPE_IP:
	case ETHERTYPE_IPTRAILERS:
		in_revarpinput(m);
		return;

	default:
		break;
	}
out:
	m_freem(m);
}

/*
 * RARP for Internet protocols on 10 Mb/s Ethernet.
 * Algorithm is that given in RFC 903.
 * We are only using for bootstrap purposes to get an ip address for one of
 * our interfaces.  Thus we support no user-interface.
 *
 * Since the contents of the RARP reply are specific to the interface that
 * sent the request, this code must ensure that they are properly associated.
 *
 * Note: also supports ARP via RARP packets, per the RFC.
 */
void
in_revarpinput(m)
	struct mbuf *m;
{
	struct ifnet *ifp;
	struct arphdr *ah;
	int op;

	ah = mtod(m, struct arphdr *);
	op = ntohs(ah->ar_op);
	switch (op) {
	case ARPOP_REQUEST:
	case ARPOP_REPLY:	/* per RFC */
		in_arpinput(m);
		return;
	case ARPOP_REVREPLY:
		break;
	case ARPOP_REVREQUEST:	/* handled by rarpd(8) */
	default:
		goto out;
	}
	if (!revarp_in_progress)
		goto out;
	ifp = m->m_pkthdr.rcvif;
	if (ifp != myip_ifp) /* !same interface */
		goto out;
	if (myip_initialized)
		goto wake;
	if (bcmp(ar_tha(ah), LLADDR(ifp->if_sadl), ifp->if_sadl->sdl_alen))
		goto out;
	bcopy((caddr_t)ar_spa(ah), (caddr_t)&srv_ip, sizeof(srv_ip));
	bcopy((caddr_t)ar_tpa(ah), (caddr_t)&myip, sizeof(myip));
	myip_initialized = 1;
wake:	/* Do wakeup every time in case it was missed. */
	wakeup((caddr_t)&myip);

out:
	m_freem(m);
}

/*
 * Send a RARP request for the ip address of the specified interface.
 * The request should be RFC 903-compliant.
 */
void
revarprequest(ifp)
	struct ifnet *ifp;
{
	struct sockaddr sa;
	struct mbuf *m;
	struct arphdr *ah;

	if ((m = m_gethdr(M_DONTWAIT, MT_DATA)) == NULL)
		return;
	m->m_len = sizeof(*ah) + 2*sizeof(struct in_addr) +
	    2*ifp->if_data.ifi_addrlen;
	m->m_pkthdr.len = m->m_len;
	MH_ALIGN(m, m->m_len);
	ah = mtod(m, struct arphdr *);
	bzero((caddr_t)ah, m->m_len);
	ah->ar_pro = htons(ETHERTYPE_IP);
	ah->ar_hln = ifp->if_data.ifi_addrlen;	/* hardware address length */
	ah->ar_pln = sizeof(struct in_addr);	/* protocol address length */
	ah->ar_op = htons(ARPOP_REVREQUEST);

	bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_sha(ah), ah->ar_hln);
	bcopy(LLADDR(ifp->if_sadl), (caddr_t)ar_tha(ah), ah->ar_hln);

	sa.sa_family = AF_ARP;
	sa.sa_len = 2;
	m->m_flags |= M_BCAST;
	(*ifp->if_output)(ifp, m, &sa, (struct rtentry *)0);

}

/*
 * RARP for the ip address of the specified interface, but also
 * save the ip address of the server that sent the answer.
 * Timeout if no response is received.
 */
int
revarpwhoarewe(ifp, serv_in, clnt_in)
	struct ifnet *ifp;
	struct in_addr *serv_in;
	struct in_addr *clnt_in;
{
	int result, count = 20;

	myip_initialized = 0;
	myip_ifp = ifp;

	revarp_in_progress = 1;
	while (count--) {
		revarprequest(ifp);
		result = tsleep((caddr_t)&myip, PSOCK, "revarp", hz/2);
		if (result != EWOULDBLOCK)
			break;
	}
	revarp_in_progress = 0;

	if (!myip_initialized)
		return ENETUNREACH;

	bcopy((caddr_t)&srv_ip, serv_in, sizeof(*serv_in));
	bcopy((caddr_t)&myip, clnt_in, sizeof(*clnt_in));
	return 0;
}



#ifdef DDB

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
static void
db_print_sa(sa)
	struct sockaddr *sa;
{
	int len;
	u_char *p;

	if (sa == 0) {
		db_printf("[NULL]");
		return;
	}

	p = (u_char*)sa;
	len = sa->sa_len;
	db_printf("[");
	while (len > 0) {
		db_printf("%d", *p);
		p++; len--;
		if (len) db_printf(",");
	}
	db_printf("]\n");
}
static void
db_print_ifa(ifa)
	struct ifaddr *ifa;
{
	if (ifa == 0)
		return;
	db_printf("  ifa_addr=");
	db_print_sa(ifa->ifa_addr);
	db_printf("  ifa_dsta=");
	db_print_sa(ifa->ifa_dstaddr);
	db_printf("  ifa_mask=");
	db_print_sa(ifa->ifa_netmask);
	db_printf("  flags=0x%x,refcnt=%d,metric=%d\n",
			  ifa->ifa_flags,
			  ifa->ifa_refcnt,
			  ifa->ifa_metric);
}
static void
db_print_llinfo(li)
	caddr_t li;
{
	struct llinfo_arp *la;

	if (li == 0)
		return;
	la = (struct llinfo_arp *)li;
	db_printf("  la_rt=%p la_hold=%p, la_asked=0x%lx\n",
			  la->la_rt, la->la_hold, la->la_asked);
}
/*
 * Function to pass to rn_walktree().
 * Return non-zero error to abort walk.
 */
static int
db_show_radix_node(rn, w)
	struct radix_node *rn;
	void *w;
{
	struct rtentry *rt = (struct rtentry *)rn;

	db_printf("rtentry=%p", rt);

	db_printf(" flags=0x%x refcnt=%d use=%ld expire=%ld\n",
			  rt->rt_flags, rt->rt_refcnt,
			  rt->rt_use, rt->rt_expire);

	db_printf(" key="); db_print_sa(rt_key(rt));
	db_printf(" mask="); db_print_sa(rt_mask(rt));
	db_printf(" gw="); db_print_sa(rt->rt_gateway);

	db_printf(" ifp=%p ", rt->rt_ifp);
	if (rt->rt_ifp)
		db_printf("(%s)", rt->rt_ifp->if_xname);
	else
		db_printf("(NULL)");

	db_printf(" ifa=%p\n", rt->rt_ifa);
	db_print_ifa(rt->rt_ifa);

	db_printf(" genmask="); db_print_sa(rt->rt_genmask);

	db_printf(" gwroute=%p llinfo=%p\n",
			  rt->rt_gwroute, rt->rt_llinfo);
	db_print_llinfo(rt->rt_llinfo);

	return (0);
}
/*
 * Function to print all the route trees.
 * Use this from ddb:  "call db_show_arptab"
 */
int
db_show_arptab()
{
	struct radix_node_head *rnh;
	rnh = rt_tables[AF_INET];
	db_printf("Route tree for AF_INET\n");
	if (rnh == NULL) {
		db_printf(" (not initialized)\n");
		return (0);
	}
	rn_walktree(rnh, db_show_radix_node, NULL);
	return (0);
}
#endif
#endif /* INET */
