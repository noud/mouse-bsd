/*
 * Driver for the SUNW,rtvc video-capture card.
 */

#include "rtvc.h"
#if NRTVC > 0

#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <vm/vm_prot.h>
#include <machine/bus.h>
#include <machine/param.h>
#include <dev/sbus/sbusvar.h>

typedef struct rtvc_softc RTVC;

struct rtvc_softc {
  struct device dev;		/* XXX interface botch */
  struct sbusdev sbdev;		/* XXX interface botch */
  RTVC *link;
  int romx;
  bus_space_tag_t bustag;
  struct sbus_reg romreg;
  } ;

extern dev_decl(rtvc,open);
extern dev_decl(rtvc,close);
extern dev_decl(rtvc,read);
extern dev_decl(rtvc,write);
extern dev_decl(rtvc,ioctl);
extern dev_decl(rtvc,mmap);
extern dev_decl(rtvc,poll);

static RTVC *all_rtvcs = 0;

static int rtvc_match(struct device *parent, struct cfdata *cf, void *aux)
{
 return(!strcmp(((struct sbus_attach_args *)aux)->sa_name,"SUNW,rtvc"));
}

static void rtvc_attach(struct device *parent, struct device *self, void *aux)
{
 RTVC *sc;
 struct sbus_attach_args *sa;
 int i;

 sc = (void *) self; /* XXX interface botch */
 sa = aux;
 printf(": ");
 printf("sa_nintr %d, sa_nreg %d\n",sa->sa_nintr,sa->sa_nreg);
 sc->romx = -1;
 sc->bustag = sa->sa_bustag;
 for (i=0;i<sa->sa_nintr;i++)
  { printf("%s: sa_intr[%d] = pri %#x, vec %#x\n",
	&sc->dev.dv_xname[0], i,
	sa->sa_intr[i].sbi_pri,
	sa->sa_intr[i].sbi_vec );
  }
 for (i=0;i<sa->sa_nreg;i++)
  { printf("%s: sa_reg[%d] = slot %u, offset %#x, size %#x\n",
	&sc->dev.dv_xname[0], i,
	sa->sa_reg[i].sbr_slot,
	sa->sa_reg[i].sbr_offset,
	sa->sa_reg[i].sbr_size );
    if (sa->sa_reg[i].sbr_offset == 0)
     { sc->romx = i;
       sc->romreg = sa->sa_reg[i];
     }
  }
 printf("%s: saving as unit %d\n",&sc->dev.dv_xname[0],sc->dev.dv_unit);
 sc->link = all_rtvcs;
 all_rtvcs = sc;
}

int rtvcopen(dev_t dev, int flags, int mode, struct proc *p)
{
 int unit;
 RTVC *sc;

 unit = minor(dev);
 printf("rtvcopen: unit %d\n",unit);
 for (sc=all_rtvcs;sc;sc=sc->link)
  { printf("rtvcopen: checking unit %d\n",sc->dev.dv_unit);
    if (sc->dev.dv_unit == unit)
     { printf("rtvcopen: found\n");
       if (flags & FWRITE) return(EPERM);
       return(0);
     }
  }
 printf("rtvcopen: not found\n");
 return(ENXIO);
}

int rtvcclose(dev_t dev, int flags, int mode, struct proc *p)
{
 return(0);
}

int rtvcread(dev_t dev, struct uio *uio, int ioflag)
{
 return(0);
}

int rtvcwrite(dev_t dev, struct uio *uio, int ioflag)
{
 return(EIO);
}

int rtvcioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 return(ENOTTY);
}

int rtvcpoll(dev_t dev, int events, struct proc *p)
{
 return(events);
}

int rtvcmmap(dev_t dev, int off, int prot)
{
 int unit;
 RTVC *sc;
 bus_space_handle_t bh;

 if (off & PGOFSET) panic("rtvcmmap - bad offset");
 if (prot & VM_PROT_WRITE) return(-1);
 unit = minor(dev);
 do <"found">
  { for (sc=all_rtvcs;sc;sc=sc->link)
     { if (sc->dev.dv_unit == unit) break <"found">;
     }
    panic("rtvcmmap - not found");
  } while (0);
 if (sc->romx < 0) return(-1);
 if (off >= sc->romreg.sbr_size) return(-1);
 if (bus_space_mmap( sc->bustag,
		     sc->romreg.sbr_slot,
		     sc->romreg.sbr_offset + off,
		     BUS_SPACE_MAP_LINEAR,
		     &bh )) return(-1);
 return((int)bh);
}

/* Autoconfig glue */
struct cfattach rtvc_ca
 = { sizeof(RTVC), &rtvc_match, &rtvc_attach };

#endif /* NRTVC > 0 */
