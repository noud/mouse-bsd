/*	$NetBSD: nfs_boot.c,v 1.52 1999/09/03 20:06:46 drochner Exp $	*/

/*-
 * Copyright (c) 1995, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
 * Support for NFS diskless booting, specifically getting information
 * about where to mount root from, what pathnames, etc.
 */

#include "opt_nfs_boot.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_ether.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#include <nfs/rpcv2.h>
#include <nfs/krpc.h>
#include <nfs/xdr_subs.h>

#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>
#include <nfs/nfsdiskless.h>

/*
 * There are two implementations of NFS diskless boot.
 * One implementation uses BOOTP (RFC951, RFC1048),
 * the other uses Sun RPC/bootparams.  See the files:
 *    nfs_bootp.c:   BOOTP (RFC951, RFC1048)
 *    nfs_bootsun.c: Sun RPC/bootparams
 */
#if defined(NFS_BOOT_BOOTP) || defined(NFS_BOOT_DHCP)
int nfs_boot_rfc951 = 1; /* BOOTP enabled (default) */
#endif
#ifdef NFS_BOOT_BOOTPARAM
int nfs_boot_bootparam = 1; /* BOOTPARAM enabled (default) */
#endif

/* mountd RPC */
static int md_mount __P((struct sockaddr_in *mdsin, char *path,
	struct nfs_args *argp));

static void nfs_boot_defrt __P((struct in_addr *));
static  int nfs_boot_getfh __P((struct nfs_dlmount *ndm));


/*
 * Called with an empty nfs_diskless struct to be filled in.
 * Find an interface, determine its ip address (etc.) and
 * save all the boot parameters in the nfs_diskless struct.
 */
int
nfs_boot_init(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{
	struct ifnet *ifp;
	int error;

	/*
	 * Find the network interface.
	 */
	ifp = ifunit(root_device->dv_xname);
	if (ifp == NULL) {
		printf("nfs_boot: '%s' not found\n",
		       root_device->dv_xname);
		return (ENXIO);
	}
	nd->nd_ifp = ifp;

	error = EADDRNOTAVAIL; /* ??? */
#if defined(NFS_BOOT_BOOTP) || defined(NFS_BOOT_DHCP)
	if (error && nfs_boot_rfc951) {
#if defined(NFS_BOOT_DHCP)
		printf("nfs_boot: trying DHCP/BOOTP\n");
#else
		printf("nfs_boot: trying BOOTP\n");
#endif
		error = nfs_bootdhcp(nd, procp);
	}
#endif
#ifdef NFS_BOOT_BOOTPARAM
	if (error && nfs_boot_bootparam) {
		printf("nfs_boot: trying RARP (and RPC/bootparam)\n");
		error = nfs_bootparam(nd, procp);
	}
#endif
	if (error)
		return (error);

	/*
	 * If the gateway address is set, add a default route.
	 * (The mountd RPCs may go across a gateway.)
	 */
	if (nd->nd_gwip.s_addr)
		nfs_boot_defrt(&nd->nd_gwip);

	/*
	 * Now fetch the NFS file handles as appropriate.
	 */
	error = nfs_boot_getfh(&nd->nd_root);

	if (error)
		nfs_boot_cleanup(nd, procp);

	return (error);
}

void
nfs_boot_cleanup(nd, procp)
	struct nfs_diskless *nd;
	struct proc *procp;
{

	nfs_boot_deladdress(nd->nd_ifp, procp, nd->nd_myip.s_addr);
	nfs_boot_ifupdown(nd->nd_ifp, procp, 0);
	nfs_boot_flushrt(nd->nd_ifp);
}

int
nfs_boot_ifupdown(ifp, procp, up)
	struct ifnet *ifp;
	struct proc *procp;
	int up;
{
	struct socket *so;
	struct ifreq ireq;
	int error;

	memset(&ireq, 0, sizeof(ireq));
	memcpy(ireq.ifr_name, ifp->if_xname, IFNAMSIZ);

	/*
	 * Get a socket to use for various things in here.
	 * After this, use "goto out" to cleanup and return.
	 */
	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("ifupdown: socreate, error=%d\n", error);
		return (error);
	}

	/*
	 * Bring up the interface. (just set the "up" flag)
	 * Get the old interface flags and or IFF_UP into them so
	 * things like media selection flags are not clobbered.
	 */
	error = ifioctl(so, SIOCGIFFLAGS, (caddr_t)&ireq, procp);
	if (error) {
		printf("ifupdown: GIFFLAGS, error=%d\n", error);
		goto out;
	}
	if (up)
		ireq.ifr_flags |= IFF_UP;
	else
		ireq.ifr_flags &= ~IFF_UP;
	error = ifioctl(so, SIOCSIFFLAGS, (caddr_t)&ireq, procp);
	if (error) {
		printf("ifupdown: SIFFLAGS, error=%d\n", error);
		goto out;
	}

	if (up)
		delay(3000000); /* give the link some time to get up */
out:
	soclose(so);
	return (error);
}

int
nfs_boot_setaddress(ifp, procp, addr, netmask, braddr)
	struct ifnet *ifp;
	struct proc *procp;
	u_int32_t addr, netmask, braddr;
{
	struct socket *so;
	struct ifaliasreq iareq;
	struct sockaddr_in *sin;
	int error;

	/*
	 * Get a socket to use for various things in here.
	 * After this, use "goto out" to cleanup and return.
	 */
	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("setaddress: socreate, error=%d\n", error);
		return (error);
	}

	memset(&iareq, 0, sizeof(iareq));
	memcpy(iareq.ifra_name, ifp->if_xname, IFNAMSIZ);

	/* Set the I/F address */
	sin = (struct sockaddr_in *)&iareq.ifra_addr;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;

	/* Set the netmask */
	if (netmask != INADDR_ANY) {
		sin = (struct sockaddr_in *)&iareq.ifra_mask;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = netmask;
	} /* else leave subnetmask unspecified (len=0) */

	/* Set the broadcast addr. */
	if (braddr != INADDR_ANY) {
		sin = (struct sockaddr_in *)&iareq.ifra_broadaddr;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = braddr;
	} /* else leave broadcast addr unspecified (len=0) */

	error = ifioctl(so, SIOCAIFADDR, (caddr_t)&iareq, procp);
	if (error) {
		printf("setaddress, error=%d\n", error);
		goto out;
	}

	delay(3000000); /* give the link some time to get up */
out:
	soclose(so);
	return (error);
}

int
nfs_boot_deladdress(ifp, procp, addr)
	struct ifnet *ifp;
	struct proc *procp;
	u_int32_t addr;
{
	struct socket *so;
	struct ifreq ireq;
	struct sockaddr_in *sin;
	int error;

	/*
	 * Get a socket to use for various things in here.
	 * After this, use "goto out" to cleanup and return.
	 */
	error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
	if (error) {
		printf("deladdress: socreate, error=%d\n", error);
		return (error);
	}

	memset(&ireq, 0, sizeof(ireq));
	memcpy(ireq.ifr_name, ifp->if_xname, IFNAMSIZ);

	sin = (struct sockaddr_in *)&ireq.ifr_addr;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;

	error = ifioctl(so, SIOCDIFADDR, (caddr_t)&ireq, procp);
	if (error) {
		printf("deladdress, error=%d\n", error);
		goto out;
	}

out:
	soclose(so);
	return (error);
}

int
nfs_boot_setrecvtimo(so)
	struct socket *so;
{
	struct mbuf *m;
	struct timeval *tv;

	m = m_get(M_WAIT, MT_SOOPTS);
	tv = mtod(m, struct timeval *);
	m->m_len = sizeof(*tv);
	tv->tv_sec = 1;
	tv->tv_usec = 0;
	return (sosetopt(so, SOL_SOCKET, SO_RCVTIMEO, m));
}

int
nfs_boot_enbroadcast(so)
	struct socket *so;
{
	struct mbuf *m;
	int32_t *on;

	m = m_get(M_WAIT, MT_SOOPTS);
	on = mtod(m, int32_t *);
	m->m_len = sizeof(*on);
	*on = 1;
	return (sosetopt(so, SOL_SOCKET, SO_BROADCAST, m));
}

int
nfs_boot_sobind_ipport(so, port)
	struct socket *so;
	u_int16_t port;
{
	struct mbuf *m;
	struct sockaddr_in *sin;
	int error;

	m = m_getclr(M_WAIT, MT_SONAME);
	sin = mtod(m, struct sockaddr_in *);
	sin->sin_len = m->m_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons(port);
	error = sobind(so, m);
	m_freem(m);
	return (error);
}

/*
 * What is the longest we will wait before re-sending a request?
 * Note this is also the frequency of "timeout" messages.
 * The re-send loop counts up linearly to this maximum, so the
 * first complaint will happen after (1+2+3+4+5)=15 seconds.
 */
#define	MAX_RESEND_DELAY 5	/* seconds */
#define TOTAL_TIMEOUT   30	/* seconds */

int
nfs_boot_sendrecv(so, nam, sndproc, snd, rcvproc, rcv, from_p, context)
	struct socket *so;
	struct mbuf *nam;
	int (*sndproc) __P((struct mbuf*, void*, int));
	struct mbuf *snd;
	int (*rcvproc) __P((struct mbuf*, void*));
	struct mbuf **rcv, **from_p;
	void *context;
{
	int error, rcvflg, timo, secs, waited;
	struct mbuf *m, *from;
	struct uio uio;

	/* Free at end if not null. */
	from = NULL;

	/*
	 * Send it, repeatedly, until a reply is received,
	 * but delay each re-send by an increasing amount.
	 * If the delay hits the maximum, start complaining.
	 */
	waited = timo = 0;
send_again:
	waited += timo;
	if (waited >= TOTAL_TIMEOUT)
		return (ETIMEDOUT);

	/* Determine new timeout. */
	if (timo < MAX_RESEND_DELAY)
		timo++;
	else
		printf("nfs_boot: timeout...\n");

	if (sndproc) {
		error = (*sndproc)(snd, context, waited);
		if (error)
			goto out;
	}

	/* Send request (or re-send). */
	m = m_copypacket(snd, M_WAIT);
	if (m == NULL) {
		error = ENOBUFS;
		goto out;
	}
	error = (*so->so_send)(so, nam, NULL, m, NULL, 0);
	if (error) {
		printf("nfs_boot: sosend: %d\n", error);
		goto out;
	}
	m = NULL;

	/*
	 * Wait for up to timo seconds for a reply.
	 * The socket receive timeout was set to 1 second.
	 */

	secs = timo;
	for (;;) {
		if (from) {
			m_freem(from);
			from = NULL;
		}
		if (m) {
			m_freem(m);
			m = NULL;
		}
		uio.uio_resid = 1 << 16; /* ??? */
		rcvflg = 0;
		error = (*so->so_receive)(so, &from, &uio, &m, NULL, &rcvflg);
		if (error == EWOULDBLOCK) {
			if (--secs <= 0)
				goto send_again;
			continue;
		}
		if (error)
			goto out;
#ifdef DIAGNOSTIC
		if (!m || !(m->m_flags & M_PKTHDR)
		    || (1 << 16) - uio.uio_resid != m->m_pkthdr.len)
			panic("nfs_boot_sendrecv: return size");
#endif

		if ((*rcvproc)(m, context))
			continue;

		if (rcv)
			*rcv = m;
		else
			m_freem(m);
		if (from_p) {
			*from_p = from;
			from = NULL;
		}
		break;
	}
out:
	if (from) m_freem(from);
	return (error);
}

/*
 * Install a default route to the passed IP address.
 */
static void
nfs_boot_defrt(gw_ip)
	struct in_addr *gw_ip;
{
	struct sockaddr dst, gw, mask;
	struct sockaddr_in *sin;
	int error;

	/* Destination: (default) */
	memset((caddr_t)&dst, 0, sizeof(dst));
	dst.sa_len = sizeof(dst);
	dst.sa_family = AF_INET;
	/* Gateway: */
	memset((caddr_t)&gw, 0, sizeof(gw));
	sin = (struct sockaddr_in *)&gw;
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = gw_ip->s_addr;
	/* Mask: (zero length) */
	/* XXX - Just pass a null pointer? */
	memset(&mask, 0, sizeof(mask));

	/* add, dest, gw, mask, flags, 0 */
	error = rtrequest(RTM_ADD, &dst, &gw, &mask,
			  (RTF_UP | RTF_GATEWAY | RTF_STATIC), NULL);
	if (error) {
		printf("nfs_boot: add route, error=%d\n", error);
		error = 0;
	}
}

static int nfs_boot_delroute __P((struct radix_node *, void *));
static int
nfs_boot_delroute(rn, w)
	struct radix_node *rn;
	void *w;
{
	struct rtentry *rt = (struct rtentry *)rn;
	int error;

	if (rt->rt_ifp != (struct ifnet *)w)
		return (0);

	error = rtrequest(RTM_DELETE, rt_key(rt), NULL, rt_mask(rt), 0, NULL);
	if (error)
		printf("nfs_boot: del route, error=%d\n", error);

	return (0);
}

void
nfs_boot_flushrt(ifp)
	struct ifnet *ifp;
{

	rn_walktree(rt_tables[AF_INET], nfs_boot_delroute, ifp);
}

/*
 * Get an initial NFS file handle using Sun RPC/mountd.
 * Separate function because we used to call it twice.
 * (once for root and once for swap)
 */
static int
nfs_boot_getfh(ndm)
	struct nfs_dlmount *ndm;	/* output */
{
	struct nfs_args *args;
	struct sockaddr_in *sin;
	char *pathname;
	int error;
	u_int16_t port;

	args = &ndm->ndm_args;

	/* Initialize mount args. */
	memset((caddr_t) args, 0, sizeof(*args));
	args->addr     = &ndm->ndm_saddr;
	args->addrlen  = args->addr->sa_len;
#ifdef NFS_BOOT_TCP
	args->sotype   = SOCK_STREAM;
#else
	args->sotype   = SOCK_DGRAM;
#endif
	args->fh       = ndm->ndm_fh;
	args->hostname = ndm->ndm_host;
	args->flags    = NFSMNT_NFSV3 | NFSMNT_NOCONN | NFSMNT_RESVPORT;

#ifdef	NFS_BOOT_OPTIONS
	args->flags    |= NFS_BOOT_OPTIONS;
#endif
#ifdef	NFS_BOOT_RWSIZE
	/*
	 * Reduce rsize,wsize for interfaces that consistently
	 * drop fragments of long UDP messages.  (i.e. wd8003).
	 * You can always change these later via remount.
	 */
	args->flags   |= NFSMNT_WSIZE | NFSMNT_RSIZE;
	args->wsize    = NFS_BOOT_RWSIZE;
	args->rsize    = NFS_BOOT_RWSIZE;
#endif

	/*
	 * Find the pathname part of the "server:pathname"
	 * string left in ndm->ndm_host by nfs_boot_init.
	 */
	pathname = strchr(ndm->ndm_host, ':');
	if (pathname == 0) {
		printf("nfs_boot: getfh - no pathname\n");
		return (EIO);
	}
	pathname++;

	/*
	 * Get file handle using RPC to mountd/mount
	 */
	sin = (struct sockaddr_in *)&ndm->ndm_saddr;
	error = md_mount(sin, pathname, args);
	if (error) {
		printf("nfs_boot: mountd `%s', error=%d\n",
		       ndm->ndm_host, error);
		return (error);
	}

	/* Set port number for NFS use. */
	/* XXX: NFS port is always 2049, right? */
#ifdef NFS_BOOT_TCP
retry:
#endif
	error = krpc_portmap(sin, NFS_PROG,
		    (args->flags & NFSMNT_NFSV3) ? NFS_VER3 : NFS_VER2,
		    (args->sotype == SOCK_STREAM) ? IPPROTO_TCP : IPPROTO_UDP,
		    &port);
	if (port == htons(0))
		error = EIO;
	if (error) {
#ifdef NFS_BOOT_TCP
		if (args->sotype == SOCK_STREAM) {
			args->sotype = SOCK_DGRAM;
			goto retry;
		}
#endif
		printf("nfs_boot: portmap NFS, error=%d\n", error);
		return (error);
	}
	sin->sin_port = port;
	return (0);
}


/*
 * RPC: mountd/mount
 * Given a server pathname, get an NFS file handle.
 * Also, sets sin->sin_port to the NFS service port.
 */
static int
md_mount(mdsin, path, argp)
	struct sockaddr_in *mdsin;		/* mountd server address */
	char *path;
	struct nfs_args *argp;
{
	/* The RPC structures */
	struct rdata {
		u_int32_t errno;
		union {
			u_int8_t  v2fh[NFSX_V2FH];
			struct {
				u_int32_t fhlen;
				u_int8_t  fh[1];
			} v3fh;
		} fh;
	} *rdata;
	struct mbuf *m;
	u_int8_t *fh;
	int minlen, error;
	int mntver;

	mntver = (argp->flags & NFSMNT_NFSV3) ? 3 : 2;
	do {
		/*
		 * Get port number for MOUNTD.
		 */
		error = krpc_portmap(mdsin, RPCPROG_MNT, mntver,
		                    IPPROTO_UDP, &mdsin->sin_port);
		if (error)
			continue;

		/* This mbuf is consumed by krpc_call. */
		m = xdr_string_encode(path, strlen(path));
		if (m == NULL)
			return ENOMEM;

		/* Do RPC to mountd. */
		error = krpc_call(mdsin, RPCPROG_MNT, mntver,
		                  RPCMNT_MOUNT, &m, NULL);
		if (error != EPROGMISMATCH)
			break;
		/* Try lower version of mountd. */
	} while (--mntver >= 1);
	if (error) {
		printf("nfs_boot: mountd error=%d\n", error);
		return error;
	}
	if (mntver != 3)
		argp->flags &= ~NFSMNT_NFSV3;

	/* The reply might have only the errno. */
	if (m->m_len < 4)
		goto bad;
	/* Have at least errno, so check that. */
	rdata = mtod(m, struct rdata *);
	error = fxdr_unsigned(u_int32_t, rdata->errno);
	if (error)
		goto out;

	/* Have errno==0, so the fh must be there. */
	if (mntver == 3) {
		argp->fhsize   = fxdr_unsigned(u_int32_t, rdata->fh.v3fh.fhlen);
		if (argp->fhsize > NFSX_V3FHMAX)
			goto bad;
		minlen = 2 * sizeof(u_int32_t) + argp->fhsize;
	} else {
		argp->fhsize   = NFSX_V2FH;
		minlen = sizeof(u_int32_t) + argp->fhsize;
	}

	if (m->m_len < minlen) {
		m = m_pullup(m, minlen);
		if (m == NULL)
			return(EBADRPC);
		rdata = mtod(m, struct rdata *);
	}

	fh = (mntver == 3) ?
		rdata->fh.v3fh.fh : rdata->fh.v2fh;
	memcpy(argp->fh, fh, argp->fhsize);

	goto out;

bad:
	error = EBADRPC;

out:
	m_freem(m);
	return error;
}
