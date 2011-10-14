/*
 * Driver for IOtech,sbiti IEEE488 interface.
 *
 * Not SMP-ready; locks against itself with spl*().
 *
 * This file is in the public domain.
 */

#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/conf.h>
#include <dev/sbus/sbusvar.h>
/*
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
*/

#include <dev/sbus/sbitiio.h>

struct sbiti_softc {
  struct device dev; /* XXX interface botch */
  struct sbusdev sbdev;
  volatile void *hw;
  unsigned int hwoff;
  unsigned int hwsize;
  } ;

static struct sbiti_softc *sbiti_sc = 0;

typedef struct sbiti_softc SOFTC;
typedef struct sbiti_hw HW;

int sbitiopen(dev_t dev, int flag, int mode, struct proc *p)
{
 dev=dev;flag=flag;mode=mode;p=p;
 return(0);
}

int sbiticlose(dev_t dev, int flag, int mode, struct proc *p)
{
 dev=dev;flag=flag;mode=mode;p=p;
 return(0);
}

int sbitiread(dev_t dev, struct uio *uio, int ioflag)
{
 dev=dev;uio=uio;ioflag=ioflag;
 return(EIO);
}

int sbitiwrite(dev_t dev, struct uio *uio, int ioflag)
{
 dev=dev;uio=uio;ioflag=ioflag;
 return(EIO);
}

int sbitiioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 volatile SOFTC *vsc;
 struct sbiti_reg *r;

 sc = sbiti_sc;
 if (sc == 0) panic("ioctl on nonexistent sbiti");
 vsc = sc;
 switch (cmd)
  { default:
       flag=flag; p=p; /* shut up "argument unused" errors */
       return(ENOTTY);
       break;
    case SBITIIOC_GREGOFF:
       *(int *)data = sc->hwoff;
       break;
    case SBITIIOC_GREGSIZE:
       *(int *)data = sc->hwsize;
       break;
    case SBITIIOC_RD_REG:
       r = (void *) data;
       if (r->off % r->size) return(EINVAL);
       switch (r->size)
	{ case 1:
	     r->val = ((volatile unsigned char *)sc->hw)[r->off];
	     break;
	  case 2:
	     r->val = ((volatile unsigned short int *)sc->hw)[r->off];
	     break;
	  case 4:
	     r->val = ((volatile unsigned int *)sc->hw)[r->off];
	     break;
	  default:
	     return(EINVAL);
	     break;
	}
       break;
    case SBITIIOC_WR_REG:
       r = (void *) data;
       if (r->off % r->size) return(EINVAL);
       switch (r->size)
	{ case 1:
	     ((volatile unsigned char *)sc->hw)[r->off] = r->val;
	     break;
	  case 2:
	     ((volatile unsigned short int *)sc->hw)[r->off] = r->val;
	     break;
	  case 4:
	     ((volatile unsigned int *)sc->hw)[r->off] = r->val;
	     break;
	  default:
	     return(EINVAL);
	     break;
	}
       break;
  }
 return(0);
}

int sbitipoll(dev_t dev, int events, struct proc *p)
{
 dev=dev;events=events;p=p;
 return(events);
}

static int sbiti_match(struct device *parent, struct cfdata *cf, void *aux)
{
 struct sbus_attach_args *sa;

 sa = aux;
 if (strcmp(sa->sa_name,"IOtech,sbiti")) return(0);
 return(1);
}

static int sbiti_hard(void *scvp)
{
 scvp=scvp;
 return(0);
}

static void sbiti_attach(struct device *parent, struct device *self, void *aux)
{
 struct sbus_attach_args *sa;
 SOFTC *sc;
 bus_space_handle_t bh;
 int err;

 sa = aux;
 sc = (void *) self;
 /* Map device registers */
 err = sbus_bus_map(sa->sa_bustag,sa->sa_slot,sa->sa_offset,sa->sa_size,
			BUS_SPACE_MAP_LINEAR,0,&bh);
 if (err)
  { printf(": cannot map registers (err %d)\n",err);
    return;
  }
 sc->hw = (void *) bh;
 sc->hwoff = sa->sa_offset;
 sc->hwsize = sa->sa_size;
 sbus_establish(&sc->sbdev,&sc->dev);
 bus_intr_establish(sa->sa_bustag,sa->sa_pri,0,&sbiti_hard,sc);
 printf(": %u@%#x\n",sc->hwsize,sc->hwoff);
}

/*
 * The autoconfig glue struct.
 */
struct cfattach sbiti_ca
 = { sizeof(struct sbiti_softc), &sbiti_match, &sbiti_attach };
