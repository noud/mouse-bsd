/*	$NetBSD: wire.c,v 1.1.1.5 1999/09/04 22:24:57 christos Exp $	*/

/*
 * Copyright (c) 1997-1999 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      %W% (Berkeley) %G%
 *
 * Id: wire.c,v 1.4 1999/08/22 21:12:34 ezk Exp
 *
 */

/*
 * This function returns the subnet (address&netmask) for the primary network
 * interface.  If the resulting address has an entry in the hosts file, the
 * corresponding name is returned, otherwise the address is returned in
 * standard internet format.
 * As a side-effect, a list of local IP/net address is recorded for use
 * by the islocalnet() function.
 *
 * Derived from original by Paul Anderson (23/4/90)
 * Updates from Dirk Grunwald (11/11/91)
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>


#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif /* HAVE_IFADDRS_H */

#ifdef HAVE_IRS_H
# include <irs.h>
#endif /* HAVE_IRS_H */

/*
 * List of locally connected networks
 */
typedef struct addrlist addrlist;
struct addrlist {
  addrlist *ip_next;
  u_long ip_addr;		/* address of network */
  u_long ip_mask;
  char *ip_net_num;		/* number of network */
  char *ip_net_name;		/* name of network */
};
static addrlist *localnets = NULL;

#if defined(IFF_LOCAL_LOOPBACK) && !defined(IFF_LOOPBACK)
# define IFF_LOOPBACK	IFF_LOCAL_LOOPBACK
#endif /* defined(IFF_LOCAL_LOOPBACK) && !defined(IFF_LOOPBACK) */

#define C(x)		((x) & 0xff)
#define GFBUFLEN	1024
#define	S2IN(s)		(((struct sockaddr_in *)(s))->sin_addr.s_addr)


/* return malloc'ed buffer.  caller must free it */
char *
print_wires(void)
{
  addrlist *al;
  char s[256];
  int i;
  char *buf;
  int bufcount = 0;
  int buf_size = 1024;

  buf = SALLOC(1024);
  if (!buf)
    return NULL;

  if (!localnets) {
    sprintf(buf, "No networks.\n");
    return buf;
  }
  /* check if there's more than one network */
  if (!localnets->ip_next) {
    sprintf(buf,
	    "Network: wire=\"%s\" (netnumber=%s).\n",
	    localnets->ip_net_name, localnets->ip_net_num);
    return buf;
  }
  buf[0] = '\0';		/* null out buffer before appending */
  for (i = 1, al = localnets; al; al = al->ip_next, i++) {
    sprintf(s, "Network %d: wire=\"%s\" (netnumber=%s).\n",
	    i, al->ip_net_name, al->ip_net_num);
    bufcount += strlen(s);
    if (bufcount > buf_size) {
      buf_size *= 2;
      buf = xrealloc(buf, buf_size);
    }
    strcat(buf, s);
  }
  return buf;
}


static struct addrlist *
getwire_lookup(u_long address, u_long netmask, int ishost)
{
  struct addrlist *al;
  u_long subnet;
  char netNumberBuf[64];
  char buf[GFBUFLEN], *s;
#ifdef HAVE_IRS_H
  struct nwent *np;
#else /* not HAVE_IRS_H */
  struct netent *np;
#endif /* not HAVE_IRS_H */

  /*
   * Add interface to local network singly linked list
   */
  al = ALLOC(struct addrlist);
  al->ip_addr = address;
  al->ip_mask = netmask;
  al->ip_net_name = NO_SUBNET; /* fill in a bit later */
  al->ip_net_num = "0.0.0.0"; /* fill in a bit later */
  al->ip_next = NULL;

  subnet = ntohl(address) & ntohl(netmask);

  if (ishost)
    np = NULL;
  else {
#ifdef HAVE_IRS_H
    u_long mask = ntohl(netmask);
    static struct irs_acc *irs_gen;
    static struct irs_nw *irs_nw;
    u_long net;
    int maskbits;
    u_char addr[4];

    if (irs_gen == NULL)
      irs_gen = irs_gen_acc("");
    if (irs_gen && irs_nw == NULL)
      irs_nw = (*irs_gen->nw_map)(irs_gen);
    net = ntohl(address) & (mask = ntohl(netmask));
    addr[0] = (0xFF000000 & net) >> 24;
    addr[1] = (0x00FF0000 & net) >> 16;
    addr[2] = (0x0000FF00 & net) >> 8;
    addr[3] = (0x000000FF & net);
    for (maskbits = 32; !(mask & 1); mask >>= 1)
      maskbits--;
    np = (*irs_nw->byaddr)(irs_nw, addr, maskbits, AF_INET);
#else /* not HAVE_IRS_H */
    np = getnetbyaddr(subnet, AF_INET);
    /*
     * Some systems (IRIX 6.4) cannot getnetbyaddr on networks such as
     * "128.59.16.0".  Instead, they need to look for the short form of
     * the network, "128.59.16".  So if the first getnetbyaddr failed, we
     * shift the subnet way from zeros and try again.
     */
    if (!np) {
      u_long short_subnet = subnet;
      while(short_subnet && (short_subnet & 0x000000ff) == 0)
	short_subnet >>= 8;
      np = getnetbyaddr(short_subnet, AF_INET);
      if (np)
	plog(XLOG_WARNING, "getnetbyaddr failed on 0x%x, suceeded on 0x%x",
	     (u_int) subnet, (u_int) short_subnet);
    }
#endif /* not HAVE_IRS_H */
  }

  if ((subnet & 0xffffff) == 0) {
    sprintf(netNumberBuf, "%lu", C(subnet >> 24));
  } else if ((subnet & 0xffff) == 0) {
    sprintf(netNumberBuf, "%lu.%lu",
	C(subnet >> 24), C(subnet >> 16));
  } else if ((subnet & 0xff) == 0) {
    sprintf(netNumberBuf, "%lu.%lu.%lu",
	C(subnet >> 24), C(subnet >> 16),
	C(subnet >> 8));
  } else {
    sprintf(netNumberBuf, "%lu.%lu.%lu.%lu",
	C(subnet >> 24), C(subnet >> 16),
	C(subnet >> 8), C(subnet));
  }

  /* fill in network number (string) */
  al->ip_net_num = strdup(netNumberBuf);

  if (np != NULL)
    s = np->n_name;
  else {
    struct hostent *hp;

    subnet = address & netmask;
    hp = gethostbyaddr((char *) &subnet, 4, AF_INET);
    if (hp != NULL)
      s = (char *) hp->h_name;
    else
      s = inet_dquad(buf, subnet);
  }

  /* fill in network name (string) */
  al->ip_net_name = strdup(s);

  return (al);
}


/*
 * Make a dotted quad from a 32bit IP address
 * addr is in network byte order.
 * sizeof(buf) needs to be at least 16.
 */
char *
inet_dquad(char *buf, u_long addr)
{
  addr = ntohl(addr);
  sprintf(buf, "%ld.%ld.%ld.%ld",
	  ((addr >> 24) & 0xff),
	  ((addr >> 16) & 0xff),
	  ((addr >> 8) & 0xff),
	  ((addr >> 0) & 0xff));
  return buf;
}


/*
 * Determine whether a network is on a local network
 * (addr) is in network byte order.
 */
int
islocalnet(u_long addr)
{
  addrlist *al;
#ifdef DEBUG
  char buf[16];
#endif /* DEBUG */

  for (al = localnets; al; al = al->ip_next)
    if (((addr ^ al->ip_addr) & al->ip_mask) == 0)
      return TRUE;

#ifdef DEBUG
    plog(XLOG_INFO, "%s is on a remote network", inet_dquad(buf, addr));
#endif /* DEBUG */

  return FALSE;
}


/*
 * Determine whether a network name is one of the local networks
 * of a host.
 */
int
is_network_member(const char *net)
{
  addrlist *al;

  for (al = localnets; al; al = al->ip_next)
    if (STREQ(net, al->ip_net_name) || STREQ(net, al->ip_net_num))
      return TRUE;

  return FALSE;
}


#ifdef HAVE_GETIFADDRS
void
getwire(char **name1, char **number1)
{
  addrlist *al = NULL, *tail = NULL;
  struct ifaddrs *ifaddrs, *ifap;

  ifaddrs = NULL;
  if (getifaddrs(&ifaddrs) < 0)
    goto out;

  for (ifap = ifaddrs; ifap != NULL; ifap = ifap->ifa_next) {

    if (ifap->ifa_addr->sa_family != AF_INET)
      continue;

    /*
     * If the interface is a loopback, or its not running
     * then ignore it.
     */
    if ((ifap->ifa_flags & IFF_LOOPBACK) != 0)
      continue;
    if ((ifap->ifa_flags & IFF_RUNNING) == 0)
      continue;

    if ((ifap->ifa_flags & IFF_POINTOPOINT) == 0)
      al = getwire_lookup(S2IN(ifap->ifa_addr), S2IN(ifap->ifa_netmask), 0);
    else
      al = getwire_lookup(S2IN(ifap->ifa_dstaddr), 0xffffffff, 1);

    /* append to the end of the list */
    if (!localnets) {
      localnets = tail = al;
      tail->ip_next = NULL;
    } else {
      tail->ip_next = al;
      tail = al;
    }
  }

out:
  if (ifaddrs)
    XFREE(ifaddrs);

  if (localnets) {
    *name1 = localnets->ip_net_name;
    *number1 = localnets->ip_net_num;
  } else {
    *name1 = NO_SUBNET;
    *number1 = "0.0.0.0";
  }
}

#else /* not HAVE_GETIFADDRS */

#if defined(HAVE_FIELD_STRUCT_IFREQ_IFR_ADDR) && defined(HAVE_FIELD_STRUCT_SOCKADDR_SA_LEN)
# define SIZE(ifr)	(MAX((ifr)->ifr_addr.sa_len, sizeof((ifr)->ifr_addr)) + sizeof(ifr->ifr_name))
#else /* not defined(HAVE_FIELD_STRUCT_IFREQ_IFR_ADDR) && defined(HAVE_FIELD_STRUCT_SOCKADDR_SA_LEN) */
# define SIZE(ifr)	sizeof(struct ifreq)
#endif /* not defined(HAVE_FIELD_STRUCT_IFREQ_IFR_ADDR) && defined(HAVE_FIELD_STRUCT_SOCKADDR_SA_LEN) */

#define clist		(ifc.ifc_ifcu.ifcu_req)
#define count		(ifc.ifc_len/sizeof(struct ifreq))


void
getwire(char **name1, char **number1)
{
  struct ifconf ifc;
  struct ifreq *ifr;
  caddr_t cp, cplim;
  int fd = -1;
  u_long address;
  addrlist *al = NULL, *tail = NULL;
  char buf[GFBUFLEN];
#if 0
  u_long net;
  u_long mask;
  u_long subnetshift;
  char buf[GFBUFLEN], *s;
#endif

#ifndef SIOCGIFFLAGS
  /* if cannot get interface flags, return nothing */
  plog(XLOG_ERROR, "getwire unable to get interface flags");
  localnets = NULL;
  return;
#endif /* not SIOCGIFFLAGS */

  /*
   * Get suitable socket
   */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    goto out;

  /*
   * Fill in ifconf details
   */
  memset(&buf[0], 0, GFBUFLEN);
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;

  /*
   * Get network interface configurations
   */
  if (ioctl(fd, SIOCGIFCONF, (caddr_t) & ifc) < 0)
    goto out;

  /*
   * Upper bound on array
   */
  cplim = buf + ifc.ifc_len;

  /*
   * This is some magic to cope with both "traditional" and the
   * new 4.4BSD-style struct sockaddrs.  The new structure has
   * variable length and a size field to support longer addresses.
   * AF_LINK is a new definition for 4.4BSD.
   */

  /*
   * Scan the list looking for a suitable interface
   */
  for (cp = buf; cp < cplim; cp += SIZE(ifr)) {
    ifr = (struct ifreq *) cp;

    if (ifr->ifr_addr.sa_family != AF_INET)
      continue;

    address = ((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr.s_addr;

    /*
     * Get interface flags
     */
    if (ioctl(fd, SIOCGIFFLAGS, (caddr_t) ifr) < 0)
      continue;

    /*
     * If the interface is a loopback, or its not running
     * then ignore it.
     */
#ifdef IFF_LOOPBACK
    if ((ifr->ifr_flags & IFF_LOOPBACK) != 0)
      continue;
#endif /* IFF_LOOPBACK */
    /*
     * Fix for 0.0.0.0 loopback on SunOS 3.X which defines IFF_ROUTE
     * instead of IFF_LOOPBACK.
     */
#ifdef IFF_ROUTE
    if (ifr->ifr_flags == (IFF_UP|IFF_RUNNING))
      continue;
#endif /* IFF_ROUTE */

    /* if the interface is not UP or not RUNNING, skip it */
    if ((ifr->ifr_flags & IFF_RUNNING) == 0 ||
	(ifr->ifr_flags & IFF_UP) == 0)
      continue;

    if ((ifr->ifr_flags & IFF_POINTOPOINT) == 0) {
      /*
       * Get the netmask of this interface
       */
      if (ioctl(fd, SIOCGIFNETMASK, (caddr_t) ifr) < 0)
	continue;

      al = getwire_lookup(address, S2IN(&ifr->ifr_addr), 0);
    } else
      al = getwire_lookup(address, 0xffffffff, 1);

    /* append to the end of the list */
    if (!localnets) {
      localnets = tail = al;
      tail->ip_next = NULL;
    } else {
      tail->ip_next = al;
      tail = al;
    }
  }

out:
  if (fd >= 0)
    close(fd);
  if (localnets) {
    *name1 = localnets->ip_net_name;
    *number1 = localnets->ip_net_num;
  } else {
    *name1 = NO_SUBNET;
    *number1 = "0.0.0.0";
  }
}
#endif /* not HAVE_GETIFADDRS */
