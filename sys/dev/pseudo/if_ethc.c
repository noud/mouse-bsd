#include "ethc.h"

#if NETHC > 0

#include <net/if.h>
#include <sys/mbuf.h>
#include <net/if_dl.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/sockio.h>
#include <net/if_types.h>

#include <dev/pseudo/if_ethc.h>

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#endif

typedef struct ethc_softc SOFTC;
typedef struct ethc_config CONF;

static SOFTC *softc;
static int nethc;

static const char null_ea[6] = { 0, 0, 0, 0, 0, 0 };
#define NULL_EA ((const void *)&null_ea[0])

static void ethc_start(struct ifnet *ifp)
{
 SOFTC *sc;
 struct mbuf *m;
 int i;
 int mx;
 struct ifnet *if2;

 sc = ifp->if_softc;
 while (1)
  { IF_DEQUEUE(&ifp->if_snd,m);
    if (m == 0) break;
#if NBPFILTER > 0
    if (ifp->if_bpf) bpf_mtap(ifp->if_bpf,m);
#endif
    if2 = sc->ifs[0];
    mx = if2->if_snd.ifq_len;
    for (i=1;i<sc->nifs;i++)
     { if (sc->ifs[i]->if_snd.ifq_len < mx)
	{ if2 = sc->ifs[i];
	  mx = if2->if_snd.ifq_len;
	}
     }
    if (IF_QFULL(&if2->if_snd))
     { IF_DROP(&if2->if_snd);
     }
    else
     { if2->if_obytes += m->m_pkthdr.len;
       IF_ENQUEUE(&if2->if_snd,m);
       if (m->m_flags & M_MCAST) if2->if_omcasts++;
       if (! (if2->if_flags & IFF_OACTIVE)) (*if2->if_start)(if2);
     }
  }
}

static int ethc_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
 SOFTC *sc;
 int s;
 int error;

 error = 0;
 sc = ifp->if_softc;
 s = splnet();
 switch (cmd)
  { case SIOCSIFADDR:
       if (ifp->if_flags & IFF_RUNNING) ifp->if_flags |= IFF_UP;
       break;
    case SIOCSIFMTU:
    case SIOCADDMULTI:
    case SIOCDELMULTI:
    case SIOCSIFMEDIA:
    case SIOCGIFMEDIA:
       error = EOPNOTSUPP;
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

static void ethc_setea(SOFTC *sc, const void *ea)
{
 struct sockaddr_dl *sdl;

 sdl = sc->ec.ec_if.if_sadl;
 if (sdl && (sdl->sdl_family == AF_LINK))
  { sdl->sdl_type = IFT_ETHER;
    sdl->sdl_alen = 6;
    bcopy(ea,LLADDR(sdl),6);
  }
}

void ethcattach(int count)
{
 SOFTC *sc;
 struct ifnet *ifp;
 int i;

 nethc = count;
 softc = malloc(count*sizeof(SOFTC),M_DEVBUF,M_WAITOK);
 memset(softc,0,count*sizeof(SOFTC)); /* XXX */
 for (i=0;i<count;i++)
  { sc = softc + i;
    ifp = &sc->ec.ec_if;
    sprintf(ifp->if_xname,"ethc%d",i);
    ifp->if_softc = sc;
    ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
    ifp->if_ioctl = ethc_ioctl;
    ifp->if_start = ethc_start;
    if_attach(ifp);
    ether_ifattach(ifp,NULL_EA);
    sc->nifs = 0;
    sc->ifs = 0;
#if NBPFILTER > 0
    bpfattach(&ifp->if_bpf,ifp,DLT_EN10MB,sizeof(struct ether_header));
#endif
  }
}

int ethcopen(dev_t dev, int flag, int mode, struct proc *p)
{
 if (minor(dev) >= nethc) return(ENXIO);
 return(0);
}

int ethcclose(dev_t dev, int flag, int mode, struct proc *p)
{
 return(0);
}

int ethcioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 CONF *cf;
 struct ifnet *ifp;
 struct ifnet **ifv;
 struct ifnet **oldifv;
 int i;
 int n;
 int s;
 int err;
 char nbuf[IFNAMSIZ];
 char ea[6];

 sc = softc + minor(dev);
 switch (cmd)
  { case ETHCIOC_SCONF:
       if (! (flag & FWRITE)) return(EBADF);
       cf = (CONF *) data;
       if (cf->nifs < 1)
	{ s = splnet();
	  sc->ec.ec_if.if_flags &= ~(IFF_RUNNING|IFF_UP);
	  oldifv = sc->ifs;
	  sc->nifs = 0;
	  sc->ifs = 0;
	  ethc_setea(sc,NULL_EA);
	  splx(s);
	}
       else
	{ ifv = malloc(cf->nifs*sizeof(struct ifnet *),M_DEVBUF,M_WAITOK);
	  if (ifv == 0) return(ENOBUFS);
	  for (i=0;i<cf->nifs;i++)
	   { strncpy(&nbuf[0],&cf->ifnames[i][0],IFNAMSIZ);
	     nbuf[IFNAMSIZ-1] = '\0';
	     ifp = ifunit(&nbuf[0]);
	     if (ifp == 0) return(ENXIO); /* needs translation */
	     if (ifp->if_type != IFT_ETHER) return(EINVAL); /* needs translation */
	     ifv[i] = ifp;
	   }
	  if (lookup_ea(ifv[0],&ea[0])) return(ENOENT); /* needs translation */
	  s = splnet();
	  oldifv = sc->ifs;
	  sc->nifs = cf->nifs;
	  sc->ifs = ifv;
	  ethc_setea(sc,&ea[0]);
	  sc->ec.ec_if.if_mtu = ifv[0]->if_mtu;
	  sc->ec.ec_if.if_flags |= IFF_RUNNING;
	  splx(s);
	}
       if (oldifv) free(oldifv,M_DEVBUF);
       return(0);
       break;
    case ETHCIOC_GCONF:
       if (! (flag & FREAD)) return(EBADF);
       cf = (CONF *) data;
       n = (cf->nifs < sc->nifs) ? cf->nifs : sc->nifs;
       for (i=0;i<n;i++)
	{ strncpy(&nbuf[0],&sc->ifs[i]->if_xname[0],IFNAMSIZ);
	  nbuf[IFNAMSIZ-1] = '\0';
	  err = copyout(&nbuf[0],&cf->ifnames[i][0],IFNAMSIZ);
	  if (err) return(err);
	}
       cf->nifs = sc->nifs;
       return(0);
       break;
  }
 return(ENOTTY);
}

#endif /* NETHC > 0 */
