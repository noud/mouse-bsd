/*	$NetBSD: print-ipcomp.c,v 1.2 1999/09/04 03:36:41 itojun Exp $	*/

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
#if 0
static const char rcsid[] =
    "@(#) /master/usr.sbin/tcpdump/tcpdump/print-icmp.c,v 2.1 1995/02/03 18:14:42 polk Exp (LBL)";
#else
#include <sys/cdefs.h>
__RCSID("$NetBSD: print-ipcomp.c,v 1.2 1999/09/04 03:36:41 itojun Exp $");
#endif
#endif

#include <string.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>

#include <stdio.h>

#ifdef INET6
#include <netinet/ip6.h>
#endif
#ifdef HAVE_NETINET6_IPCOMP_H
#include <netinet6/ipcomp.h>
#else
struct ipcomp {
	u_int8_t comp_nxt;	/* Next Header */
	u_int8_t comp_flags;	/* Length of data, in 32bit */
	u_int16_t comp_cpi;	/* Compression parameter index */
};
#endif

#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
#include <zlib.h>
#endif

#include "interface.h"
#include "addrtoname.h"

int
ipcomp_print(register const u_char *bp, register const u_char *bp2, int *nhdr)
{
	register const struct ipcomp *ipcomp;
	register const u_char *ep;
	u_int16_t cpi;
#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
	int advance;
#endif

	ipcomp = (struct ipcomp *)bp;
	cpi = (u_int16_t)ntohs(ipcomp->comp_cpi);

	/* 'ep' points to the end of avaible data. */
	ep = snapend;

	if ((u_char *)(ipcomp + 1) >= ep - sizeof(struct ipcomp)) {
		fputs("[|IPCOMP]", stdout);
		goto fail;
	}
	printf("IPComp(cpi=%u)", cpi);

#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
	if (1)
		goto fail;

	/*
	 * We may want to decompress the packet here.  Packet buffer
	 * management is a headache (if we decompress, packet will become
	 * larger).
	 */
	if (nhdr)
		*nhdr = ipcomp->comp_nxt;
	advance = sizeof(struct ipcomp);

	printf(": ");
	return advance;

#endif
fail:
	if (nhdr)
		*nhdr = -1;
	return 65536;
}
