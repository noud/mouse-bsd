/*	$NetBSD: if_tun.c,v 1.38 1999/07/01 08:12:48 itojun Exp $	*/

/*
 * Copyright (c) 1988, Julian Onions <jpo@cs.nott.ac.uk>
 * Nottingham University 1987.
 *
 * This source may be freely distributed, however I would be interested
 * in any changes that are made.
 *
 * This driver takes packets off the IP i/f and hands them up to a
 * user process to have its wicked way with. This driver has its
 * roots in a similar driver written by Phil Cockcroft (formerly) at
 * UCL. This driver is based much more on read/write/poll mode of
 * operation though.
 */

#include "tun.h"
#if NTUN > 0

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/signalvar.h>
#include <sys/conf.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>

#if defined(INET) || defined(INET6)
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#ifdef INET6
#include <netinet/ip6.h>
#endif
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <sys/time.h>
#include <net/bpf.h>
#endif

#include <net/if_tun.h>

#define TUNDEBUG	if (tundebug) printf
int	tundebug = 0;

struct tun_softc tunctl[NTUN];
extern int ifqmaxlen;
void	tunattach __P((int));

int	tun_ioctl __P((struct ifnet *, u_long, caddr_t));
int	tun_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
		       struct rtentry *rt));

void
tunattach(unused)
	int unused;
{
	register int i;
	struct ifnet *ifp;

	for (i = 0; i < NTUN; i++) {
		tunctl[i].tun_flags = TUN_INITED;

		ifp = &tunctl[i].tun_if;
		sprintf(ifp->if_xname, "tun%d", i);
		ifp->if_softc = &tunctl[i];
		ifp->if_mtu = TUNMTU;
		ifp->if_ioctl = tun_ioctl;
		ifp->if_output = tun_output;
		ifp->if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
		ifp->if_type = IFT_TUN;
		ifp->if_snd.ifq_maxlen = ifqmaxlen;
		ifp->if_collisions = 0;
		ifp->if_ierrors = 0;
		ifp->if_oerrors = 0;
		ifp->if_ipackets = 0;
		ifp->if_opackets = 0;
		if_attach(ifp);
#if NBPFILTER > 0
		bpfattach(&tunctl[i].tun_bpf, ifp, DLT_NULL, sizeof(u_int32_t));
#endif
	}
}

/*
 * tunnel open - must be superuser & the device must be
 * configured in
 */
int
tunopen(dev, flag, mode, p)
	dev_t	dev;
	int	flag, mode;
	struct proc *p;
{
	struct ifnet	*ifp;
	struct tun_softc *tp;
	register int	unit, error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	if ((unit = minor(dev)) >= NTUN)
		return (ENXIO);
	tp = &tunctl[unit];
	if (tp->tun_flags & TUN_OPEN)
		return(EBUSY);
	ifp = &tp->tun_if;
	tp->tun_flags |= TUN_OPEN;
	TUNDEBUG("%s: open\n", ifp->if_xname);
	return (0);
}

/*
 * tunclose - close the device
 */
int
tunclose(dev, flag, mode, p)
	dev_t	dev;
	int	flag;
	int	mode;
	struct proc *p;
{
	register int	unit = minor(dev), s;
	struct tun_softc *tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;
	struct mbuf	*m;

	tp->tun_flags &= ~TUN_OPEN;

	/*
	 * junk all pending output
	 */
	do {
		s = splimp();
		IF_DEQUEUE(&ifp->if_snd, m);
		splx(s);
		if (m)
			m_freem(m);
	} while (m);

	tp->tun_pgrp = 0;
	selwakeup(&tp->tun_rsel);

	TUNDEBUG ("%s: closed\n", ifp->if_xname);
	return (0);
}

/*
 * Process an ioctl request.
 */
int
tun_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t	data;
{
 int error;
 int s;

 error = 0;
 s = splimp();
 switch (cmd)
  { case SIOCSIFADDR:
#define ifa ((struct ifaddr *)data)
       switch (ifa->ifa_addr->sa_family)
	{
#ifdef INET
	  case AF_INET:
	     ifp->if_flags |= IFF_UP;
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     ifp->if_flags |= IFF_UP;
	     if (ifp->if_mtu < IPV6_MMTU) ifp->if_mtu = IPV6_MMTU;
	     break;
#endif
	  default:
	     error = EAFNOSUPPORT;
	     break;
	}
#undef ifa
       break;
    case SIOCSIFDSTADDR:
    case SIOCSIFBRDADDR:
#define ifa ((struct ifaddr *)data)
       switch (ifa->ifa_addr->sa_family)
	{
#ifdef INET
	  case AF_INET:
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     break;
#endif
	  default:
	     error = EAFNOSUPPORT;
	     break;
	}
#undef ifa
       break;
    case SIOCSIFMTU:
#define ifr ((struct ifreq *)data)
       if (ifr->ifr_mtu < 576)
	{ error = EINVAL;
	  break;
	}
#ifdef INET6
       if (ifr->ifr_mtu < IPV6_MMTU)
	{ struct ifaddr *ifa;
	  for ( ifa = ifp->if_addrlist.tqh_first;
		ifa;
		ifa = ifa->ifa_list.tqe_next )
	   { switch (((struct sockaddr *)ifa->ifa_addr)->sa_family)
	      { case AF_INET6:
		   error = EINVAL;
		   goto done; /* break switch, for, switch */
	      }
	   }
	}
#endif
       ifp->if_mtu = ifr->ifr_mtu;
#undef ifr
       break;
    case SIOCGIFMTU:
#define ifr ((struct ifreq *)data)
       ifr->ifr_mtu = ifp->if_mtu;
#undef ifr
       break;
    case SIOCADDMULTI:
    case SIOCDELMULTI:
#define ifr ((struct ifreq *)data)
       if (ifr == 0)
	{ error = EAFNOSUPPORT;				/* XXX */
	  break;
	}
       switch (ifr->ifr_addr.sa_family)
	{
#ifdef INET
	  case AF_INET:
	     break;
#endif
#ifdef INET6
	  case AF_INET6:
	     break;
#endif
	  default:
	     error = EAFNOSUPPORT;
	     break;
	}
       break;
    case SIOCSIFFLAGS:
       break;
    default:
       error = EINVAL;
       break;
  }
#ifdef INET6 /* used only inside INET6 above */
done:;
#endif
 splx(s);
 return(error);
}

/*
 * tun_output - queue packets from higher level ready to put out.
 */
int
tun_output(ifp, m0, dst, rt)
	struct ifnet   *ifp;
	struct mbuf    *m0;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	struct tun_softc *tp = ifp->if_softc;
	struct proc	*p;
#ifdef INET
	int		s;
#endif

	TUNDEBUG ("%s: tun_output\n", ifp->if_xname);

	switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif
#ifdef INET6
		case AF_INET6:
			break;
#endif
		default:
			printf("%s: af%d not supported\n",ifp->if_xname,dst->sa_family);
			m_freem(m0);
			return(EAFNOSUPPORT);
			break;
	}
	if (! (tp->tun_flags & TUN_OPEN)) {
		return(ENETDOWN);
	}

#if NBPFILTER > 0
	if (tp->tun_bpf) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer to it).
		 */
		struct mbuf m;
		u_int32_t af = dst->sa_family;

		m.m_next = m0;
		m.m_len = sizeof(af);
		m.m_data = (char *)&af;

		bpf_mtap(tp->tun_bpf, &m);
	}
#endif

	switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
#endif
#ifdef INET6
		case AF_INET6:
#endif
#if defined(INET) || defined(INET6)
			if (tp->tun_flags & TUN_PREPADDR) {
				/* Simple link-layer header */
				M_PREPEND(m0, dst->sa_len, M_DONTWAIT);
				if (m0 == NULL) {
					IF_DROP(&ifp->if_snd);
					return (ENOBUFS);
				}
				bcopy(dst, mtod(m0, char *), dst->sa_len);
			}
			/* FALLTHROUGH */
#endif
		case AF_UNSPEC:
			s = splimp();
			if (IF_QFULL(&ifp->if_snd)) {
				IF_DROP(&ifp->if_snd);
				m_freem(m0);
				splx(s);
				ifp->if_collisions++;
				return (ENOBUFS);
			}
			IF_ENQUEUE(&ifp->if_snd, m0);
			splx(s);
			ifp->if_opackets++;
			break;
		default:
			m_freem(m0);
			return (EAFNOSUPPORT);
	}

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgrp) {
		if (tp->tun_pgrp > 0)
			gsignal(tp->tun_pgrp, SIGIO);
		else if ((p = pfind(-tp->tun_pgrp)) != NULL)
			psignal(p, SIGIO);
	}
	selwakeup(&tp->tun_rsel);
	return (0);
}

/*
 * the cdevsw interface is now pretty minimal.
 */
int
tunioctl(dev, cmd, data, flag, p)
	dev_t		dev;
	u_long		cmd;
	caddr_t		data;
	int		flag;
	struct proc	*p;
{
	int		unit = minor(dev), s;
	struct tun_softc *tp = &tunctl[unit];

	switch (cmd) {
	case TUNSDEBUG:
		tundebug = *(int *)data;
		break;

	case TUNGDEBUG:
		*(int *)data = tundebug;
		break;

	case TUNSIFMODE:
		switch (*(int *)data) {
		case IFF_POINTOPOINT:
		case IFF_BROADCAST:
			s = splimp();
			if (tp->tun_if.if_flags & IFF_UP) {
				splx(s);
				return (EBUSY);
			}
			tp->tun_if.if_flags &=
				~(IFF_BROADCAST|IFF_POINTOPOINT);
			tp->tun_if.if_flags |= *(int *)data;
			splx(s);
			break;
		default:
			return (EINVAL);
			break;
		}
		break;

	case TUNSLMODE:
		if (*(int *)data)
			tp->tun_flags |= TUN_PREPADDR;
		else
			tp->tun_flags &= ~TUN_PREPADDR;
		break;

	case FIONBIO:
		if (*(int *)data)
			tp->tun_flags |= TUN_NBIO;
		else
			tp->tun_flags &= ~TUN_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			tp->tun_flags |= TUN_ASYNC;
		else
			tp->tun_flags &= ~TUN_ASYNC;
		break;

	case FIONREAD:
		s = splimp();
		if (tp->tun_if.if_snd.ifq_head)
			*(int *)data = tp->tun_if.if_snd.ifq_head->m_pkthdr.len;
		else
			*(int *)data = 0;
		splx(s);
		break;

	case TIOCSPGRP:
		tp->tun_pgrp = *(int *)data;
		break;

	case TIOCGPGRP:
		*(int *)data = tp->tun_pgrp;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * The cdevsw read interface - reads a packet at a time, or at
 * least as much of a packet as can be read.
 */
int
tunread(dev, uio, ioflag)
	dev_t		dev;
	struct uio	*uio;
	int		ioflag;
{
	int		unit = minor(dev);
	struct tun_softc *tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;
	struct mbuf	*m, *m0;
	int		error=0, len, s;

	TUNDEBUG ("%s: read\n", ifp->if_xname);
	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname, tp->tun_flags);
		return EHOSTDOWN;
	}

	tp->tun_flags &= ~TUN_RWAIT;

	s = splimp();
	do {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0) {
			if (tp->tun_flags & TUN_NBIO) {
				splx(s);
				return (EWOULDBLOCK);
			}
			tp->tun_flags |= TUN_RWAIT;
			if (tsleep((caddr_t)tp, PZERO|PCATCH, "tunread", 0)) {
				splx(s);
				return (EINTR);
			}
		}
	} while (m0 == 0);
	splx(s);

	while (m0 && uio->uio_resid > 0 && error == 0) {
		len = min(uio->uio_resid, m0->m_len);
		if (len == 0)
			break;
		error = uiomove(mtod(m0, caddr_t), len, uio);
		MFREE(m0, m);
		m0 = m;
	}

	if (m0) {
		TUNDEBUG("Dropping mbuf\n");
		m_freem(m0);
	}
	if (error)
		ifp->if_ierrors++;
	return (error);
}

/*
 * the cdevsw write interface - an atomic write is a packet - or else!
 */
int
tunwrite(dev, uio, ioflag)
	dev_t		dev;
	struct uio	*uio;
	int		ioflag;
{
	int		unit = minor (dev);
	struct tun_softc *tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;
	struct mbuf	*top, **mp, *m;
	struct ifqueue	*ifq;
	int		isr, error=0, s, tlen, mlen;
 struct sockaddr_storage dst;

	TUNDEBUG("%s: tunwrite\n", ifp->if_xname);

 if (tp->tun_flags & TUN_PREPADDR)
  { /* XXX this code depends on ss_len being first! */
    if (uio->uio_resid < sizeof(dst.ss_len)) return(EIO);
    error = uiomove(&dst.ss_len,sizeof(dst.ss_len),uio);
    if (error) return(error);
    if (dst.ss_len <= sizeof(dst.ss_len)) return(EINVAL);
    error = uiomove(sizeof(dst.ss_len)+(char *)&dst,dst.ss_len-sizeof(dst.ss_len),uio);
    if (error) return(error);
  }
 else
  { dst.ss_len = sizeof(struct sockaddr_in);
    dst.ss_family = AF_INET;
  }

	if (uio->uio_resid < 0 || uio->uio_resid > TUNMTU) {
		TUNDEBUG("%s: len=%lu!\n", ifp->if_xname,
		    (unsigned long)uio->uio_resid);
		return (EIO);
	}

	switch (dst.ss_family) {
#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		isr = NETISR_IP;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ifq = &ip6intrq;
		isr = NETISR_IPV6;
		break;
#endif
	default:
		ifp->if_ierrors ++;
		return(EAFNOSUPPORT);
	}

	tlen = uio->uio_resid;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);
	mlen = MHLEN;

	top = 0;
	mp = &top;
	while (error == 0 && uio->uio_resid > 0) {
		m->m_len = min(mlen, uio->uio_resid);
		error = uiomove(mtod (m, caddr_t), m->m_len, uio);
		*mp = m;
		mp = &m->m_next;
		if (uio->uio_resid > 0) {
			MGET (m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				error = ENOBUFS;
				break;
			}
			mlen = MLEN;
		}
	}
	if (error) {
		if (top)
			m_freem (top);
		ifp->if_ierrors++;
		return (error);
	}

	top->m_pkthdr.len = tlen;
	top->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
	if (tp->tun_bpf) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer to it).
		 */
		struct mbuf m;
		u_int32_t af = dst.ss_family;

		m.m_next = top;
		m.m_len = sizeof(af);
		m.m_data = (char *)&af;

		bpf_mtap(tp->tun_bpf, &m);
	}
#endif

	s = splimp();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		splx(s);
		ifp->if_collisions++;
		m_freem(top);
		return (ENOBUFS);
	}
	IF_ENQUEUE(ifq, top);
	splx(s);
	ifp->if_ipackets++;
	schednetisr(isr);
	return (error);
}

/*
 * tunpoll - the poll interface, this is only useful on reads
 * really. The write detect always returns true, write never blocks
 * anyway, it either accepts the packet or drops it.
 */
int
tunpoll(dev, events, p)
	dev_t		dev;
	int		events;
	struct proc	*p;
{
	int		unit = minor(dev), s;
	struct tun_softc *tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;
	int		revents = 0;

	s = splimp();
	TUNDEBUG("%s: tunpoll\n", ifp->if_xname);

	if (events & (POLLIN | POLLRDNORM)) {
		if (ifp->if_snd.ifq_len > 0) {
			TUNDEBUG("%s: tunpoll q=%d\n", ifp->if_xname,
			    ifp->if_snd.ifq_len);
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			TUNDEBUG("%s: tunpoll waiting\n", ifp->if_xname);
			selrecord(p, &tp->tun_rsel);
		}
	}

	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);

	splx(s);
	return (revents);
}

#endif  /* NTUN */
