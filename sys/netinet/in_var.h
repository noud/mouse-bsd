/*	$NetBSD: in_var.h,v 1.37 2000/02/02 23:28:09 thorpej Exp $	*/

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
 * Copyright (c) 1985, 1986, 1993
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
 *	@(#)in_var.h	8.2 (Berkeley) 1/9/95
 */

#ifndef _NETINET_IN_VAR_H_
#define _NETINET_IN_VAR_H_

#include <net/if.h>
#include <sys/queue.h>
#include <sys/socketvar.h>

/*
 * Interface address, Internet version.  One of these structures
 * is allocated for each interface with an Internet address.
 * The ifaddr structure contains the protocol-independent part
 * of the structure and is assumed to be first.
 */
struct in_ifaddr {
	struct	ifaddr ia_ifa;		/* protocol-independent info */
#define	ia_ifp		ia_ifa.ifa_ifp
#define ia_flags	ia_ifa.ifa_flags
					/* ia_{,sub}net{,mask} in host order */
	u_int32_t ia_net;		/* network number of interface */
	u_int32_t ia_netmask;		/* mask of net part */
	u_int32_t ia_subnet;		/* subnet number, including net */
	u_int32_t ia_subnetmask;	/* mask of subnet part */
	struct	in_addr ia_netbroadcast; /* to recognize net broadcasts */
	LIST_ENTRY(in_ifaddr) ia_hash;	/* entry in bucket of inet addresses */
	TAILQ_ENTRY(in_ifaddr) ia_list;	/* list of internet addresses */
	struct	sockaddr_in ia_addr;	/* reserve space for interface name */
	struct	sockaddr_in ia_dstaddr;	/* reserve space for broadcast addr */
#define	ia_broadaddr	ia_dstaddr
	struct	sockaddr_in ia_sockmask; /* reserve space for general netmask */
	LIST_HEAD(, in_multi) ia_multiaddrs; /* list of multicast addresses */
};

struct	in_aliasreq {
	char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	sockaddr_in ifra_addr;
	struct	sockaddr_in ifra_dstaddr;
#define	ifra_broadaddr	ifra_dstaddr
	struct	sockaddr_in ifra_mask;
};
/*
 * Given a pointer to an in_ifaddr (ifaddr),
 * return a pointer to the addr as a sockaddr_in.
 */
#define	IA_SIN(ia) (&(((struct in_ifaddr *)(ia))->ia_addr))


#ifdef	_KERNEL
#ifndef IN_IFADDR_HASH_SIZE
#define IN_IFADDR_HASH_SIZE 509	/* 61, 127, 251, 509, 1021, 2039 are good */
#endif

/*
 * This is a bit unconventional, and wastes a little bit of space, but
 * because we want a very even hash function we don't use & in_ifaddrhash
 * here, but rather % the hash size, which should obviously be prime.
 */

#define	IN_IFADDR_HASH(x) in_ifaddrhashtbl[(u_long)(x) % IN_IFADDR_HASH_SIZE]

u_long in_ifaddrhash;				/* size of hash table - 1 */
int	in_ifaddrentries;			/* total number of addrs */
LIST_HEAD(in_ifaddrhashhead, in_ifaddr);	/* Type of the hash head */
TAILQ_HEAD(in_ifaddrhead, in_ifaddr);		/* Type of the list head */

extern  struct in_ifaddrhashhead *in_ifaddrhashtbl;	/* Hash table head */
extern  struct in_ifaddrhead in_ifaddr;		/* List head (in ip_input) */

extern	struct	ifqueue	ipintrq;		/* ip packet input queue */
extern	int	inetctlerrmap[];
void	in_socktrim __P((struct sockaddr_in *));


/*
 * Macro for finding whether an internet address (in_addr) belongs to one
 * of our interfaces (in_ifaddr).  NULL if the address isn't ours.
 *
 * Note that even if we find an interface with the address we're looking
 * for, we should skip that interface if it is not up.
 */
#define INADDR_TO_IA(addr, ia) \
	/* struct in_addr addr; */ \
	/* struct in_ifaddr *ia; */ \
{ \
	for (ia = IN_IFADDR_HASH((addr).s_addr).lh_first; \
	     ia != NULL; \
	     ia = ia->ia_hash.le_next) { \
		if (in_hosteq(ia->ia_addr.sin_addr, (addr)) && \
		    (ia->ia_ifp->if_flags & IFF_UP) != 0) \
			break; \
	} \
}

/*
 * Macro for finding the next in_ifaddr structure with the same internet
 * address as ia. Call only with a valid ia pointer.
 * Will set ia to NULL if none found.
 */

#define NEXT_IA_WITH_SAME_ADDR(ia) \
	/* struct in_ifaddr *ia; */ \
{ \
	struct in_addr addr; \
	addr = ia->ia_addr.sin_addr; \
	do { \
		ia = ia->ia_hash.le_next; \
	} while ((ia != NULL) && !in_hosteq(ia->ia_addr.sin_addr, addr)); \
}

/*
 * Macro for finding the interface (ifnet structure) corresponding to one
 * of our IP addresses.
 */
#define INADDR_TO_IFP(addr, ifp) \
	/* struct in_addr addr; */ \
	/* struct ifnet *ifp; */ \
{ \
	register struct in_ifaddr *ia; \
\
	INADDR_TO_IA(addr, ia); \
	(ifp) = (ia == NULL) ? NULL : ia->ia_ifp; \
}

/*
 * Macro for finding an internet address structure (in_ifaddr) corresponding
 * to a given interface (ifnet structure).
 */
#define IFP_TO_IA(ifp, ia) \
	/* struct ifnet *ifp; */ \
	/* struct in_ifaddr *ia; */ \
{ \
	register struct ifaddr *ifa; \
\
	for (ifa = (ifp)->if_addrlist.tqh_first; \
	    ifa != NULL && ifa->ifa_addr->sa_family != AF_INET; \
	    ifa = ifa->ifa_list.tqe_next) \
		continue; \
	(ia) = ifatoia(ifa); \
}
#endif

/*
 * Per-interface router version information.
 */
struct router_info {
	struct	ifnet *rti_ifp;
	int	rti_type;	/* type of router on this interface */
	int	rti_age;	/* time since last v1 query */
	struct	router_info *rti_next;
};

/*
 * Internet multicast address structure.  There is one of these for each IP
 * multicast group to which this host belongs on a given network interface.
 * They are kept in a linked list, rooted in the interface's in_ifaddr
 * structure.
 */
struct in_multi {
	struct	in_addr inm_addr;	/* IP multicast address */
	struct	ifnet *inm_ifp;		/* back pointer to ifnet */
	struct	in_ifaddr *inm_ia;	/* back pointer to in_ifaddr */
	u_int	inm_refcount;		/* no. membership claims by sockets */
	u_int	inm_timer;		/* IGMP membership report timer */
	LIST_ENTRY(in_multi) inm_list;	/* list of multicast addresses */
	u_int	inm_state;		/* state of membership */
	struct	router_info *inm_rti;	/* router version info */
};

#ifdef _KERNEL
/*
 * Structure used by macros below to remember position when stepping through
 * all of the in_multi records.
 */
struct in_multistep {
	struct in_ifaddr *i_ia;
	struct in_multi *i_inm;
};

/*
 * Macro for looking up the in_multi record for a given IP multicast address
 * on a given interface.  If no matching record is found, "inm" returns NULL.
 */
#define IN_LOOKUP_MULTI(addr, ifp, inm) \
	/* struct in_addr addr; */ \
	/* struct ifnet *ifp; */ \
	/* struct in_multi *inm; */ \
{ \
	register struct in_ifaddr *ia; \
\
	IFP_TO_IA((ifp), ia); 			/* multicast */ \
	if (ia == NULL) \
		(inm) = NULL; \
	else \
		for ((inm) = ia->ia_multiaddrs.lh_first; \
		    (inm) != NULL && !in_hosteq((inm)->inm_addr, (addr)); \
		     (inm) = inm->inm_list.le_next) \
			 continue; \
}

/*
 * Macro to step through all of the in_multi records, one at a time.
 * The current position is remembered in "step", which the caller must
 * provide.  IN_FIRST_MULTI(), below, must be called to initialize "step"
 * and get the first record.  Both macros return a NULL "inm" when there
 * are no remaining records.
 */
#define IN_NEXT_MULTI(step, inm) \
	/* struct in_multistep  step; */ \
	/* struct in_multi *inm; */ \
{ \
	if (((inm) = (step).i_inm) != NULL) \
		(step).i_inm = (inm)->inm_list.le_next; \
	else \
		while ((step).i_ia != NULL) { \
			(inm) = (step).i_ia->ia_multiaddrs.lh_first; \
			(step).i_ia = (step).i_ia->ia_list.tqe_next; \
			if ((inm) != NULL) { \
				(step).i_inm = (inm)->inm_list.le_next; \
				break; \
			} \
		} \
}

#define IN_FIRST_MULTI(step, inm) \
	/* struct in_multistep step; */ \
	/* struct in_multi *inm; */ \
{ \
	(step).i_ia = in_ifaddr.tqh_first; \
	(step).i_inm = NULL; \
	IN_NEXT_MULTI((step), (inm)); \
}

struct ifaddr;

int	in_ifinit __P((struct ifnet *,
	    struct in_ifaddr *, struct sockaddr_in *, int));
struct	in_multi *in_addmulti __P((struct in_addr *, struct ifnet *));
void	in_delmulti __P((struct in_multi *));
void	in_ifscrub __P((struct ifnet *, struct in_ifaddr *));
void	in_setmaxmtu __P((void));
const char *in_fmtaddr __P((struct in_addr));
int	in_control __P((struct socket *, u_long, caddr_t, struct ifnet *,
	    struct proc *));
void	in_purgeaddr __P((struct ifaddr *, struct ifnet *));
void	in_purgeif __P((struct ifnet *));
void	ip_input __P((struct mbuf *));
int	ipflow_fastforward __P((struct mbuf *));

#endif

/* INET6 stuff */
#include <netinet6/in6_var.h>

#endif /* _NETINET_IN_VAR_H_ */
