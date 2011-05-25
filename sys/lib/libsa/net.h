/*	$NetBSD: net.h,v 1.13 1999/05/07 14:49:56 drochner Exp $	*/

/*
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	California, Lawrence Berkeley Laboratory and its contributors.
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
 */

#ifndef _KERNEL	/* XXX - see <netinet/in.h> */
#undef __IPADDR
#define __IPADDR(x)	htonl((u_int32_t)(x))
#endif

#include "iodesc.h"

#define BA { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }

/* Returns true if n_long's on the same net */
#define	SAMENET(a1, a2, m) ((a1.s_addr & m) == (a2.s_addr & m))

#define MACPY(s, d) bcopy((char *)s, (char *)d, 6)

#define MAXTMO 20	/* seconds */
#define MINTMO 2	/* seconds */

#define FNAME_SIZE 128
#define	IFNAME_SIZE 16
#define RECV_SIZE 1536	/* XXX delete this */

/*
 * How much room to leave for headers:
 *  14: struct ether_header
 *  20: struct ip
 *   8: struct udphdr
 * That's 42 but let's pad it out to 48 bytes.
 */
#define ETHER_SIZE 14
#define	HEADER_SIZE 48

extern	u_char bcea[6];
extern	char rootpath[FNAME_SIZE];
extern	char bootfile[FNAME_SIZE];
extern	char hostname[FNAME_SIZE];
extern	int hostnamelen;
extern	char domainname[FNAME_SIZE];
extern	int domainnamelen;
extern	char ifname[IFNAME_SIZE];

/* All of these are in network order. */
extern	struct in_addr myip;
extern	struct in_addr rootip;
extern	struct in_addr swapip;
extern	struct in_addr gateip;
extern	struct in_addr nameip;
extern	n_long netmask;

extern	int debug;			/* defined in the machdep sources */

extern struct iodesc sockets[SOPEN_MAX];

/* ARP/RevARP functions: */
u_char	*arpwhohas __P((struct iodesc *, struct in_addr));
void	arp_reply __P((struct iodesc *, void *));
int	rarp_getipaddress __P((int));

/* Link functions: */
ssize_t sendether __P((struct iodesc *d, void *pkt, size_t len,
			u_char *dea, int etype));
ssize_t readether __P((struct iodesc *d, void *pkt, size_t len,
			time_t tleft, u_int16_t *etype));

ssize_t	sendudp __P((struct iodesc *, void *, size_t));
ssize_t	readudp __P((struct iodesc *, void *, size_t, time_t));
ssize_t	sendrecv __P((struct iodesc *,
		      ssize_t (*)(struct iodesc *, void *, size_t),
			void *, size_t,
		        ssize_t (*)(struct iodesc *, void *, size_t, time_t),
			void *, size_t));

/* Utilities: */
char	*ether_sprintf __P((u_char *));
int	in_cksum __P((void *, int));
int	in_cksum __P((void *, int));

/* Machine-dependent functions: */
time_t	getsecs __P((void));
