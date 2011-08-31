#include "vlan.h"

#if NVLAN > 0

#include "opt_inet.h"

#include <net/if_dl.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/sockio.h>
#include <net/if_types.h>

#include <dev/pseudo/if_vlan.h>
#include <dev/pseudo/dot1qint.h>

#ifdef INET
#include <netinet/if_inarp.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

typedef struct vlan_softc SOFTC;
typedef struct vlan_config CONF;

static SOFTC *softc;
static int nvlan;

static const char null_ea[6] = { 0, 0, 0, 0, 0, 0 };
#define NULL_EA ((const void *)&null_ea[0])

static void vlan_start(struct ifnet *ifp)
{
 SOFTC *sc;
 struct mbuf *m;
 unsigned char *mp;
 struct ifnet *if2;

 sc = ifp->if_softc;
 if2 = sc->conf.u.dstifp;
 while (1)
  { IF_DEQUEUE(&ifp->if_snd,m);
    if (m == 0) break;
#if NBPFILTER > 0
    if (ifp->if_bpf) bpf_mtap(ifp->if_bpf,m);
#endif
    if (! (m->m_flags & M_PKTHDR))
     { panic("vlan_start no pkthdr");
     }
    if (! if2)
     { m_freem(m);
       continue;
     }
    ifp->if_opackets ++;
    if (m->m_pkthdr.len < ETHER_MIN_LEN+4-ETHER_CRC_LEN)
     { struct mbuf *ml;
       int nb;
       nb = ETHER_MIN_LEN+4-ETHER_CRC_LEN - m->m_pkthdr.len;
       for (ml=m;ml->m_next;ml=ml->m_next) ;
       if (M_TRAILINGSPACE(ml) >= nb)
	{ bzero(mtod(ml,char *)+ml->m_len,nb);
	  ml->m_len += nb;
	}
       else
	{ struct mbuf *mpad;
	  if (nb > MLEN)
	   { panic("vlan_start: padding >MLEN");
	   }
	  else
	   { MGET(mpad,M_DONTWAIT,m->m_type);
	     mpad->m_len = nb;
	     bzero(mtod(mpad,char *),nb);
	   }
	  ml->m_next = mpad;
	}
       m->m_pkthdr.len += nb;
     }
    switch (sc->conf.tag)
     { case VLAN_NONE:
	  break;
       case VLAN_OTHER:
	  m_freem(m);
	  continue;
	  break;
       default:
	  M_PREPEND(m,4,M_DONTWAIT);
	  if (m == 0) break;
	  if (m->m_len < 18) m = m_pullup(m,18);
	  if (m == 0) break;
	  mp = mtod(m,char *);
	   { char t[12];
	     bcopy(mp+4,&t[0],12);
	     bcopy(&t[0],mp,12);
	   }
	  mp[12] = ETHERTYPE_VLAN >> 8;
	  mp[13] = ETHERTYPE_VLAN & 0xff;
	  mp[14] = sc->conf.tag >> 8;
	  mp[15] = sc->conf.tag & 0xff;
     }
    if (IF_QFULL(&if2->if_snd))
     { IF_DROP(&if2->if_snd);
       m_freem(m);
     }
    else
     { if2->if_obytes += m->m_pkthdr.len;
       IF_ENQUEUE(&if2->if_snd,m);
       if (m->m_flags & M_MCAST) if2->if_omcasts++;
       if (! (if2->if_flags & IFF_OACTIVE))
	{ (*if2->if_start)(if2);
	}
     }
  }
}

static int vlan_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
 SOFTC *sc;
 int s;
 int error;
 struct ifnet *if2;
 struct ifreq *ifr;

 error = 0;
 sc = ifp->if_softc;
 s = splnet();
 if2 = sc->conf.u.dstifp;
 switch (cmd)
  { case SIOCSIFADDR:
	{ struct ifaddr *ifa;
	  ifa = (void *) data;
	  ifp->if_flags |= IFF_UP;
	  switch (ifa->ifa_addr->sa_family)
	   {
#ifdef INET
	     case AF_INET:
		arp_ifinit(ifp,ifa);
		break;
#endif
	   }
	}
       break;
    case SIOCSIFFLAGS:
       if (if2)
	{ if ((ifp->if_flags & IFF_PROMISC) && !sc->sub_promisc)
	   { error = ifpromisc(if2,1);
	     if (error)
	      { ifp->if_flags &= ~IFF_PROMISC;
	      }
	     else
	      { sc->sub_promisc = 1;
	      }
	   }
	  else if (sc->sub_promisc && !(ifp->if_flags & IFF_PROMISC))
	   { error = ifpromisc(if2,0);
	     if (error)
	      { ifp->if_flags |= IFF_PROMISC;
	      }
	     else
	      { sc->sub_promisc = 0;
	      }
	   }
	}
       break;
    case SIOCADDMULTI:
       error = ether_addmulti((struct ifreq *)data,&sc->ec);
       if (error == ENETRESET)
	{ error = if2 ? (*if2->if_ioctl)(if2,cmd,data) : 0;
	}
       break;
    case SIOCDELMULTI:
       error = ether_delmulti((struct ifreq *)data,&sc->ec);
       if (error == ENETRESET)
	{ error = if2 ? (*if2->if_ioctl)(if2,cmd,data) : 0;
	}
       break;
    case SIOCSIFMEDIA:
    case SIOCGIFMEDIA:
       if (! if2)
	{ error = EOPNOTSUPP;
	  break;
	}
       error = (*if2->if_ioctl)(if2,cmd,data);
       break;
    case SIOCSIFMTU:
       ifr = (void *) data;
       if ((ifr->ifr_mtu > ETHERMTU) || (ifr->ifr_mtu < ETHERMIN))
	{ error = EINVAL;
	}
       else
	{ ifp->if_mtu = ifr->ifr_mtu;
	}
       break;
    default:
       error = EINVAL;
       break;
  }
 splx(s);
 return(error);
}

static int lookup_ea(struct ifnet *ifp, void *ea)
{
 struct sockaddr_dl *sdl;

 sdl = ifp->if_sadl;
 if ( sdl &&
      (sdl->sdl_family == AF_LINK) &&
      (sdl->sdl_type == IFT_ETHER) &&
      (sdl->sdl_alen == 6) )
  { bcopy(LLADDR(sdl),ea,6);
    return(0);
  }
 return(1);
}

static void vlan_setea(SOFTC *sc, const void *ea)
{
 struct sockaddr_dl *sdl;

 sdl = sc->ec.ec_if.if_sadl;
 if (sdl && (sdl->sdl_family == AF_LINK))
  { sdl->sdl_type = IFT_ETHER;
    sdl->sdl_alen = 6;
    bcopy(ea,LLADDR(sdl),6);
  }
}

static void vlan_ifdisc(SOFTC *sc)
{
 struct ifnet *ifp;
 int i;

 ifp = sc->conf.u.dstifp;
 if (ifp == 0) return;
 sc->conf.u.dstifp = 0;
 for (i=0;i<nvlan;i++) if (softc[i].conf.u.dstifp == ifp) break;
 if (i >= nvlan) ifp->if_flags &= ~IFF_DOT1Q;
 if (sc->sub_promisc)
  { ifpromisc(ifp,0);
    sc->sub_promisc = 0;
  }
}

static void vlan_ifconn(SOFTC *sc, struct ifnet *ifp)
{
 sc->conf.u.dstifp = ifp;
 ifp->if_flags |= IFF_DOT1Q;
 sc->ec.ec_if.if_flags |= IFF_RUNNING;
 if (sc->ec.ec_if.if_flags & IFF_PROMISC)
  { ifpromisc(ifp,1);
    sc->sub_promisc = 1;
  }
}

static void vlan_down(SOFTC *sc)
{
 vlan_ifdisc(sc);
 vlan_setea(sc,NULL_EA);
 sc->ec.ec_if.if_flags &= ~(IFF_UP|IFF_RUNNING);
 sc->conf.tag = VLAN_OTHER;
}

void vlanattach(int count)
{
 SOFTC *sc;
 struct ifnet *ifp;
 int i;

 nvlan = count;
 softc = malloc(count*sizeof(SOFTC),M_DEVBUF,M_WAITOK);
 memset(softc,0,count*sizeof(SOFTC)); /* XXX */
 for (i=0;i<count;i++)
  { sc = softc + i;
    ifp = &sc->ec.ec_if;
    sprintf(ifp->if_xname,"vlan%d",i);
    ifp->if_softc = sc;
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = vlan_ioctl;
    ifp->if_start = vlan_start;
    if_attach(ifp);
    ether_ifattach(ifp,NULL_EA);
    ifp->if_hdrlen = 18;
    sc->conf.tag = VLAN_OTHER;
    sc->conf.u.dstifp = 0;
#if NBPFILTER > 0
    bpfattach(&ifp->if_bpf,ifp,DLT_EN10MB,sizeof(struct ether_header));
#endif
    sc->sub_promisc = 0;
  }
}

int vlanopen(dev_t dev, int flag, int mode, struct proc *p)
{
 if (minor(dev) >= nvlan) return(ENXIO);
 return(0);
}

int vlanclose(dev_t dev, int flag, int mode, struct proc *p)
{
 return(0);
}

int vlanioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 CONF *cf;
 struct ifnet *ifp;
 char nbuf[IFNAMSIZ];
 char ea[6];

 sc = softc + minor(dev);
 switch (cmd)
  { case VLANIOC_SCONF:
       if (! (flag & FWRITE)) return(EBADF);
       cf = (CONF *) data;
       if (! cf->u.dstifn[0])
	{ vlan_down(sc);
	}
       else
	{ strncpy(&nbuf[0],&cf->u.dstifn[0],IFNAMSIZ);
	  nbuf[IFNAMSIZ-1] = '\0';
	  ifp = ifunit(&nbuf[0]);
	  if (ifp == 0) return(ENXIO); /* needs translation */
	  if (ifp->if_type != IFT_ETHER) return(EINVAL); /* needs translation */
	  if (lookup_ea(ifp,&ea[0])) return(ENOENT); /* needs translation */
	  if (ifp != sc->conf.u.dstifp)
	   { vlan_ifdisc(sc);
	     vlan_ifconn(sc,ifp);
	     vlan_setea(sc,&ea[0]);
	   }
	  sc->conf.tag = cf->tag;
	}
       return(0);
       break;
    case VLANIOC_GCONF:
       if (! (flag & FREAD)) return(EBADF);
       cf = (CONF *) data;
       cf->tag = sc->conf.tag;
       if (sc->conf.u.dstifp)
	{ strncpy(&cf->u.dstifn[0],&sc->conf.u.dstifp->if_xname[0],IFNAMSIZ);
	}
       else
	{ memset(&cf->u.dstifn[0],0,IFNAMSIZ);
	}
       return(0);
       break;
  }
 return(ENOTTY);
}

/* this routine assumes responsibility for the mbuf chain */
void dot1q_input(struct ifnet *ifp, struct mbuf *m)
{
 int i;
 SOFTC *sc;
 SOFTC *sco;
 unsigned char *mp;
 int vlan;

 if (! (m->m_flags & M_PKTHDR)) panic("dot1q_input: no pkthdr");
 m = m_pullup(m,18);
 if (m == 0) return;
 mp = mtod(m,unsigned char *);
 if ( (mp[12] == (ETHERTYPE_VLAN >> 8)) &&
      (mp[13] == (ETHERTYPE_VLAN & 0xff)) )
  { vlan = (mp[14] << 8) | mp[15];
     { char t[12];
       bcopy(mp,&t[0],12);
       bcopy(&t[0],mp+4,12);
     }
    m_adj(m,4);
  }
 else
  { vlan = VLAN_NONE;
  }
 sco = 0;
 for (i=nvlan-1;i>=0;i--)
  { sc = softc + i;
    if (sc->conf.u.dstifp != ifp) continue;
    if (sc->conf.tag == vlan) break;
    if (sc->conf.tag == VLAN_OTHER) sco = sc;
  }
 if (i < 0) sc = sco;
 if (sc)
  { m->m_pkthdr.rcvif = &sc->ec.ec_if;
#if NBPFILTER > 0
    if (sc->ec.ec_if.if_bpf) bpf_mtap(sc->ec.ec_if.if_bpf,m);
#endif
    sc->ec.ec_if.if_ipackets ++;
    (*sc->ec.ec_if.if_input)(&sc->ec.ec_if,m);
  }
 else
  { m_freem(m);
  }
}

#endif /* NVLAN > 0 */
