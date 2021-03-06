/*	$NetBSD: if.c,v 1.56 2000/02/06 16:43:33 thorpej Exp $	*/

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by William Studnemund and Jason R. Thorpe.
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

/*
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)if.c	8.5 (Berkeley) 1/9/95
 */

#include "opt_inet.h"

#include "opt_compat_linux.h"
#include "opt_compat_svr4.h"
#include "opt_compat_43.h"
#include "opt_atalk.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/radix.h>
#include <net/route.h>
#ifdef NETATALK
#include <netatalk/at_extern.h>
#include <netatalk/at.h>
#endif

#ifdef INET6
/*XXX*/
#include <netinet/in.h>
#endif

int	ifqmaxlen = IFQ_MAXLEN;
void	if_slowtimo __P((void *arg));

#ifdef INET6
/*
 * XXX: declare here to avoid to include many inet6 related files..
 * should be more generalized?
 */
extern void nd6_setmtu __P((struct ifnet *));
#endif

int	if_rt_walktree __P((struct radix_node *, void *));

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */
void
ifinit()
{

	if_slowtimo(NULL);
}

/*
 * Null routines used while an interface is going away.  These routines
 * just return an error.
 */
int	if_nulloutput __P((struct ifnet *, struct mbuf *,
	    struct sockaddr *, struct rtentry *));
void	if_nullinput __P((struct ifnet *, struct mbuf *));
void	if_nullstart __P((struct ifnet *));
int	if_nullioctl __P((struct ifnet *, u_long, caddr_t));
int	if_nullreset __P((struct ifnet *));
void	if_nullwatchdog __P((struct ifnet *));
void	if_nulldrain __P((struct ifnet *));

int
if_nulloutput(ifp, m, so, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *so;
	struct rtentry *rt;
{

	return (ENXIO);
}

void
if_nullinput(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{

	/* Nothing. */
}

void
if_nullstart(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

int
if_nullioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{

	return (ENXIO);
}

int
if_nullreset(ifp)
	struct ifnet *ifp;
{

	return (ENXIO);
}

void
if_nullwatchdog(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

void
if_nulldrain(ifp)
	struct ifnet *ifp;
{

	/* Nothing. */
}

int if_index = 0;
struct ifaddr **ifnet_addrs = NULL;
struct ifnet **ifindex2ifnet = NULL;

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
void
if_attach(ifp)
	struct ifnet *ifp;
{
	unsigned socksize, ifasize;
	int namelen, masklen;
	register struct sockaddr_dl *sdl;
	register struct ifaddr *ifa;
	static size_t if_indexlim = 8;

	if (if_index == 0)
		TAILQ_INIT(&ifnet);
	TAILQ_INIT(&ifp->if_addrlist);
	TAILQ_INSERT_TAIL(&ifnet, ifp, if_list);
	ifp->if_index = ++if_index;

	/*
	 * We have some arrays that should be indexed by if_index.
	 * since if_index will grow dynamically, they should grow too.
	 *	struct ifadd **ifnet_addrs
	 *	struct ifnet **ifindex2ifnet
	 */
	if (ifnet_addrs == 0 || ifindex2ifnet == 0 ||
	    ifp->if_index >= if_indexlim) {
		size_t n;
		caddr_t q;

		while (ifp->if_index >= if_indexlim)
			if_indexlim <<= 1;

		/* grow ifnet_addrs */
		n = if_indexlim * sizeof(ifa);
		q = (caddr_t)malloc(n, M_IFADDR, M_WAITOK);
		bzero(q, n);
		if (ifnet_addrs) {
			bcopy((caddr_t)ifnet_addrs, q, n/2);
			free((caddr_t)ifnet_addrs, M_IFADDR);
		}
		ifnet_addrs = (struct ifaddr **)q;

		/* grow ifindex2ifnet */
		n = if_indexlim * sizeof(struct ifnet *);
		q = (caddr_t)malloc(n, M_IFADDR, M_WAITOK);
		bzero(q, n);
		if (ifindex2ifnet) {
			bcopy((caddr_t)ifindex2ifnet, q, n/2);
			free((caddr_t)ifindex2ifnet, M_IFADDR);
		}
		ifindex2ifnet = (struct ifnet **)q;
	}

	ifindex2ifnet[ifp->if_index] = ifp;

	/*
	 * create a Link Level name for this device
	 */
	namelen = strlen(ifp->if_xname);
	masklen = offsetof(struct sockaddr_dl, sdl_data[0]) + namelen;
	socksize = masklen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))
	if (socksize < sizeof(*sdl))
		socksize = sizeof(*sdl);
	socksize = ROUNDUP(socksize);
	ifasize = sizeof(*ifa) + 2 * socksize;
	ifa = (struct ifaddr *)malloc(ifasize, M_IFADDR, M_WAITOK);
	bzero((caddr_t)ifa, ifasize);
	sdl = (struct sockaddr_dl *)(ifa + 1);
	sdl->sdl_len = socksize;
	sdl->sdl_family = AF_LINK;
	bcopy(ifp->if_xname, sdl->sdl_data, namelen);
	sdl->sdl_nlen = namelen;
	sdl->sdl_index = ifp->if_index;
	sdl->sdl_type = ifp->if_type;
	ifnet_addrs[ifp->if_index] = ifa;
	IFAREF(ifa);
	ifa->ifa_ifp = ifp;
	ifa->ifa_rtrequest = link_rtrequest;
	TAILQ_INSERT_HEAD(&ifp->if_addrlist, ifa, ifa_list);
	IFAREF(ifa);
	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifp->if_sadl = sdl;
	sdl = (struct sockaddr_dl *)(socksize + (caddr_t)sdl);
	ifa->ifa_netmask = (struct sockaddr *)sdl;
	sdl->sdl_len = masklen;
	while (namelen != 0)
		sdl->sdl_data[--namelen] = 0xff;
	if (ifp->if_snd.ifq_maxlen == 0)
	    ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_broadcastaddr = 0; /* reliably crash if used uninitialized */
}

/*
 * Deactivate an interface.  This points all of the procedure
 * handles at error stubs.  May be called from interrupt context.
 */
void
if_deactivate(ifp)
	struct ifnet *ifp;
{
	int s;

	s = splimp();

	ifp->if_output	 = if_nulloutput;
	ifp->if_input	 = if_nullinput;
	ifp->if_start	 = if_nullstart;
	ifp->if_ioctl	 = if_nullioctl;
	ifp->if_reset	 = if_nullreset;
	ifp->if_watchdog = if_nullwatchdog;
	ifp->if_drain	 = if_nulldrain;

	/* No more packets may be enqueued. */
	ifp->if_snd.ifq_maxlen = 0;

	splx(s);
}

/*
 * Detach an interface from the list of "active" interfaces,
 * freeing any resources as we go along.
 *
 * NOTE: This routine must be called with a valid thread context,
 * as it may block.
 */
void
if_detach(ifp)
	struct ifnet *ifp;
{
	struct socket so;
	struct ifaddr *ifa;
#ifdef IFAREF_DEBUG
	struct ifaddr *last_ifa = NULL;
#endif
	struct domain *dp;
	struct protosw *pr;
	struct radix_node_head *rnh;
	int s, i, family, purged;

	/*
	 * XXX It's kind of lame that we have to have the
	 * XXX socket structure...
	 */
	memset(&so, 0, sizeof(so));

	s = splimp();

	/*
	 * Do an if_down() to give protocols a chance to do something.
	 */
	if_down(ifp);

	/*
	 * Rip all the addresses off the interface.  This should make
	 * all of the routes go away.
	 */
	while ((ifa = TAILQ_FIRST(&ifp->if_addrlist)) != NULL) {
		family = ifa->ifa_addr->sa_family;
#ifdef IFAREF_DEBUG
		printf("if_detach: ifaddr %p, family %d, refcnt %d\n",
		    ifa, family, ifa->ifa_refcnt);
		if (last_ifa != NULL && ifa == last_ifa)
			panic("if_detach: loop detected");
		last_ifa = ifa;
#endif
		if (family == AF_LINK) {
			rtinit(ifa, RTM_DELETE, 0);
			TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
			IFAFREE(ifa);
		} else {
			dp = pffinddomain(family);
#ifdef DIAGNOSTIC
			if (dp == NULL)
				panic("if_detach: no domain for AF %d\n",
				    family);
#endif
			purged = 0;
			for (pr = dp->dom_protosw;
			     pr < dp->dom_protoswNPROTOSW; pr++) {
				so.so_proto = pr;
				if (pr->pr_usrreq != NULL) {
					(void) (*pr->pr_usrreq)(&so,
					    PRU_PURGEIF, NULL, NULL,
					    (struct mbuf *) ifp, curproc);
					purged = 1;
				}
			}
			if (purged == 0) {
				/*
				 * XXX What's really the best thing to do
				 * XXX here?  --thorpej@netbsd.org
				 */
				printf("if_detach: WARNING: AF %d not purged\n",
				    family);
			}
		}
	}

	/* Walk the routing table looking for straglers. */
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) != NULL)
			(void) (*rnh->rnh_walktree)(rnh, if_rt_walktree, ifp);
	}

	IFAFREE(ifnet_addrs[ifp->if_index]);
	ifnet_addrs[ifp->if_index] = NULL;

	TAILQ_REMOVE(&ifnet, ifp, if_list);

	splx(s);
}

/*
 * Callback for a radix tree walk to delete all references to an
 * ifnet.
 */
int
if_rt_walktree(rn, v)
	struct radix_node *rn;
	void *v;
{
	struct ifnet *ifp = (struct ifnet *)v;
	struct rtentry *rt = (struct rtentry *)rn;
	int error;

	if (rt->rt_ifp == ifp) {
		/* Delete the entry. */
		error = rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway,
		    rt_mask(rt), rt->rt_flags, NULL);
		if (error)
			printf("%s: warning: unable to delete rtentry @ %p, "
			    "error = %d\n", ifp->if_xname, rt, error);
	}
	return (0);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithaddr(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), ((struct sockaddr *)(a1))->sa_len) == 0)

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			if (ifa->ifa_addr->sa_family != addr->sa_family)
				continue;
			if (equal(addr, ifa->ifa_addr))
				return (ifa);
			if ((ifp->if_flags & IFF_BROADCAST) &&
			    ifa->ifa_broadaddr &&
			    /* IP6 doesn't have broadcast */
			    ifa->ifa_broadaddr->sa_len != 0 &&
			    equal(ifa->ifa_broadaddr, addr))
				return (ifa);
		}
	}
	return (NULL);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr)
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		if (ifp->if_flags & IFF_POINTOPOINT) {
			for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
			     ifa = TAILQ_NEXT(ifa, ifa_list)) {
				if (ifa->ifa_addr->sa_family !=
				      addr->sa_family ||
				    ifa->ifa_dstaddr == NULL)
					continue;
				if (equal(addr, ifa->ifa_dstaddr))
					return (ifa);
			}
		}
	}
	return (NULL);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *
ifa_ifwithnet(addr)
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register struct sockaddr_dl *sdl;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;
	char *addr_data = addr->sa_data, *cplim;

	if (af == AF_LINK) {
		sdl = (struct sockaddr_dl *)addr;
		if (sdl->sdl_index && sdl->sdl_index <= if_index &&
		    ifindex2ifnet[sdl->sdl_index]->if_output != if_nulloutput)
			return (ifnet_addrs[sdl->sdl_index]);
	}
#ifdef NETATALK
	if (af == AF_APPLETALK) {
		for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
		     ifp = TAILQ_NEXT(ifp, if_list)) {
			if (ifp->if_output == if_nulloutput)
				continue;
			ifa = at_ifawithnet((struct sockaddr_at *)addr, ifp);
			if (ifa)
				return (ifa);
		}
		return (NULL);
	}
#endif
	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			register char *cp, *cp2, *cp3;

			if (ifa->ifa_addr->sa_family != af ||
			    ifa->ifa_netmask == 0)
 next:				continue;
			cp = addr_data;
			cp2 = ifa->ifa_addr->sa_data;
			cp3 = ifa->ifa_netmask->sa_data;
			cplim = (char *)ifa->ifa_netmask +
			    ifa->ifa_netmask->sa_len;
			while (cp3 < cplim) {
				if ((*cp++ ^ *cp2++) & *cp3++) {
					/* want to continue for() loop */
					goto next;
				}
			}
			if (ifa_maybe == 0 ||
			    rn_refines((caddr_t)ifa->ifa_netmask,
			    (caddr_t)ifa_maybe->ifa_netmask))
				ifa_maybe = ifa;
		}
	}
	return (ifa_maybe);
}

/*
 * Find the interface of the addresss.
 */
struct ifaddr *
ifa_ifwithladdr(addr)
	struct sockaddr *addr;
{
	struct ifaddr *ia;

	if ((ia = ifa_ifwithaddr(addr)) || (ia = ifa_ifwithdstaddr(addr)) ||
	    (ia = ifa_ifwithnet(addr)))
		return (ia);
	return (NULL);
}

/*
 * Find an interface using a specific address family
 */
struct ifaddr *
ifa_ifwithaf(af)
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
		for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
		     ifa = TAILQ_NEXT(ifa, ifa_list)) {
			if (ifa->ifa_addr->sa_family == af)
				return (ifa);
		}
	}
	return (NULL);
}

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *
ifaof_ifpforaddr(addr, ifp)
	struct sockaddr *addr;
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	register char *cp, *cp2, *cp3;
	register char *cplim;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;

	if (ifp->if_output == if_nulloutput)
		return (NULL);

	if (af >= AF_MAX)
		return (NULL);

	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list)) {
		if (ifa->ifa_addr->sa_family != af)
			continue;
		ifa_maybe = ifa;
		if (ifa->ifa_netmask == 0) {
			if (equal(addr, ifa->ifa_addr) ||
			    (ifa->ifa_dstaddr &&
			     equal(addr, ifa->ifa_dstaddr)))
				return (ifa);
			continue;
		}
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++) {
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		}
		if (cp3 == cplim)
			return (ifa);
	}
	return (ifa_maybe);
}

/*
 * Default action when installing a route with a Link Level gateway.
 * Lookup an appropriate real ifa to point to.
 * This should be moved to /sys/net/link.c eventually.
 */
void
link_rtrequest(cmd, rt, sa)
	int cmd;
	register struct rtentry *rt;
	struct sockaddr *sa;
{
	register struct ifaddr *ifa;
	struct sockaddr *dst;
	struct ifnet *ifp;

	if (cmd != RTM_ADD || ((ifa = rt->rt_ifa) == 0) ||
	    ((ifp = ifa->ifa_ifp) == 0) || ((dst = rt_key(rt)) == 0))
		return;
	if ((ifa = ifaof_ifpforaddr(dst, ifp)) != NULL) {
		IFAFREE(rt->rt_ifa);
		rt->rt_ifa = ifa;
		IFAREF(ifa);
		if (ifa->ifa_rtrequest && ifa->ifa_rtrequest != link_rtrequest)
			ifa->ifa_rtrequest(cmd, rt, sa);
	}
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list))
		pfctlinput(PRC_IFDOWN, ifa->ifa_addr);
	if_qflush(&ifp->if_snd);
	rt_ifmsg(ifp);
}

/*
 * Mark an interface up and notify protocols of
 * the transition.
 * NOTE: must be called at splsoftnet or equivalent.
 */
void
if_up(ifp)
	register struct ifnet *ifp;
{
#ifdef notyet
	register struct ifaddr *ifa;
#endif

	ifp->if_flags |= IFF_UP;
#ifdef notyet
	/* this has no effect on IP, and will kill all ISO connections XXX */
	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL;
	     ifa = TAILQ_NEXT(ifa, ifa_list))
		pfctlinput(PRC_IFUP, ifa->ifa_addr);
#endif
	rt_ifmsg(ifp);
#ifdef INET6
	in6_if_up(ifp);
#endif
}

/*
 * Flush an interface queue.
 */
void
if_qflush(ifq)
	register struct ifqueue *ifq;
{
	register struct mbuf *m, *n;

	n = ifq->ifq_head;
	while ((m = n) != NULL) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(arg)
	void *arg;
{
	register struct ifnet *ifp;
	int s = splimp();

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp);
	}
	splx(s);
	timeout(if_slowtimo, NULL, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name)
	const char *name;
{
	register struct ifnet *ifp;

	for (ifp = TAILQ_FIRST(&ifnet); ifp != NULL;
	     ifp = TAILQ_NEXT(ifp, if_list)) {
		if (ifp->if_output == if_nulloutput)
			continue;
	 	if (strcmp(ifp->if_xname, name) == 0)
			return (ifp);
	}
	return (NULL);
}


/*
 * Map interface name in a sockaddr_dl to
 * interface structure pointer.
 */
struct ifnet *
if_withname(sa)
	struct sockaddr *sa;
{
	char ifname[IFNAMSIZ+1];
	struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;

	if ((sa->sa_family != AF_LINK) || (sdl->sdl_nlen == 0) ||
	     (sdl->sdl_nlen > IFNAMSIZ))
		return (NULL);

	/*
	 * ifunit wants a null-terminated name.  It may not be null-terminated
	 * in the sockaddr.  We don't want to change the caller's sockaddr,
	 * and there might not be room to put the trailing null anyway, so we
	 * make a local copy that we know we can null terminate safely.
	 */

	bcopy(sdl->sdl_data, ifname, sdl->sdl_nlen);
	ifname[sdl->sdl_nlen] = '\0';
	return ifunit(ifname);
}


/*
 * Interface ioctls.
 */
int
ifioctl(so, cmd, data, p)
	struct socket *so;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;
	int error = 0;
	short oif_flags;

	switch (cmd) {

	case SIOCGIFCONF:
	case OSIOCGIFCONF:
		return (ifconf(cmd, data));
	}
	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0)
		return (ENXIO);
	oif_flags = ifp->if_flags;
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCSIFFLAGS:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			int s = splimp();
			if_down(ifp);
			splx(s);
		}
		if (ifr->ifr_flags & IFF_UP && (ifp->if_flags & IFF_UP) == 0) {
			int s = splimp();
			if_up(ifp);
			splx(s);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSIFMETRIC:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCSIFMTU:
	{
		u_long oldmtu = ifp->if_mtu;

		error = suser(p->p_ucred, &p->p_acflag);
		if (error)
			return (error);
		if (ifp->if_ioctl == NULL)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);

		/*
		 * If the link MTU changed, do network layer specific procedure.
		 */
		if (ifp->if_mtu != oldmtu) {
#ifdef INET6
			nd6_setmtu(ifp);
#endif
		}
		break;
	}
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
		/* FALLTHROUGH */
	case SIOCGIFMEDIA:
		if (ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		error = (*ifp->if_ioctl)(ifp, cmd, data);
		break;

	case SIOCSDRVSPEC:
		/* XXX:  need to pass proc pointer through to driver... */
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
	/* FALLTHROUGH */
	default:
		if (so->so_proto == 0)
			return (EOPNOTSUPP);
#if !defined(COMPAT_43) && !defined(COMPAT_LINUX) && !defined(COMPAT_SVR4)
		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, p));
#else
	    {
		int ocmd = cmd;

		switch (cmd) {

		case SIOCSIFADDR:
		case SIOCSIFDSTADDR:
		case SIOCSIFBRDADDR:
		case SIOCSIFNETMASK:
#if BYTE_ORDER != BIG_ENDIAN
			if (ifr->ifr_addr.sa_family == 0 &&
			    ifr->ifr_addr.sa_len < 16) {
				ifr->ifr_addr.sa_family = ifr->ifr_addr.sa_len;
				ifr->ifr_addr.sa_len = 16;
			}
#else
			if (ifr->ifr_addr.sa_len == 0)
				ifr->ifr_addr.sa_len = 16;
#endif
			break;

		case OSIOCGIFADDR:
			cmd = SIOCGIFADDR;
			break;

		case OSIOCGIFDSTADDR:
			cmd = SIOCGIFDSTADDR;
			break;

		case OSIOCGIFBRDADDR:
			cmd = SIOCGIFBRDADDR;
			break;

		case OSIOCGIFNETMASK:
			cmd = SIOCGIFNETMASK;
		}

		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, p));

		switch (ocmd) {
		case OSIOCGIFADDR:
		case OSIOCGIFDSTADDR:
		case OSIOCGIFBRDADDR:
		case OSIOCGIFNETMASK:
			*(u_int16_t *)&ifr->ifr_addr = ifr->ifr_addr.sa_family;
		}
	    }
#endif /* COMPAT_43 */
		break;
	}

	if (((oif_flags ^ ifp->if_flags) & IFF_UP) != 0) {
#ifdef INET6
		if ((ifp->if_flags & IFF_UP) != 0) {
			int s = splimp();
			in6_if_up(ifp);
			splx(s);
		}
#endif
	}

	return (error);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
int
ifconf(cmd, data)
	u_long cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0;

	ifrp = ifc->ifc_req;
	for (ifp = ifnet.tqh_first;
	    space >= sizeof (ifr) && ifp != 0; ifp = ifp->if_list.tqe_next) {
		bcopy(ifp->if_xname, ifr.ifr_name, IFNAMSIZ);
		if ((ifa = ifp->if_addrlist.tqh_first) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
			    sizeof(ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		} else
		    for (; space >= sizeof (ifr) && ifa != 0; ifa = ifa->ifa_list.tqe_next) {
			register struct sockaddr *sa = ifa->ifa_addr;
#if defined(COMPAT_43) || defined(COMPAT_LINUX) || defined(COMPAT_SVR4)
			if (cmd == OSIOCGIFCONF) {
				struct osockaddr *osa =
					 (struct osockaddr *)&ifr.ifr_addr;
				ifr.ifr_addr = *sa;
				osa->sa_family = sa->sa_family;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr));
				ifrp++;
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				ifr.ifr_addr = *sa;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr));
				ifrp++;
			} else {
				space -= sa->sa_len - sizeof(*sa);
				if (space < sizeof (ifr))
					break;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr.ifr_name));
				if (error == 0)
				    error = copyout((caddr_t)sa,
				      (caddr_t)&ifrp->ifr_addr, sa->sa_len);
				ifrp = (struct ifreq *)
					(sa->sa_len + (caddr_t)&ifrp->ifr_addr);
			}
			if (error)
				break;
			space -= sizeof (ifr);
		}
	}
	ifc->ifc_len -= space;
	return (error);
}
