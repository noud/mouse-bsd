#include "rwkm.h"

#if NRWKM > 0

#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wseventvar.h>

#include <dev/wscons/rwkmint.h>

typedef struct softc SOFTC;
#define EVQ_SZ

struct softc {
  int open;
  struct wseventvar evq;
  } ;

static SOFTC softcs[NRWKM][2];

cdev_decl(rwkm);
extern void rwkmattach(int); /* XXX */

void rwkmattach(int n)
{
 if (n != NRWKM) panic("rwkm: count wrong");
 for (n--;n>=0;n--)
  { softcs[n][0].open = 0;
    wsevent_init(&softcs[n][0].evq);
    softcs[n][1].open = 0;
    wsevent_init(&softcs[n][1].evq);
  }
}

int rwkmopen(dev_t dev, int flags, int mode, struct proc *p)
{
 int mousep;
 unsigned int u;
 int err;

 u = minor(dev);
 mousep = u & 1;
 u >>= 1;
 if ((u < 0) || (u >= NRWKM)) return(ENXIO);
 if (softcs[u][mousep].open) return(EBUSY);
 err = mousep
       ? wsmouse_rwkm_open(u,&softcs[u][1].evq)
       : wskbd_rwkm_open(u,&softcs[u][0].evq);
 if (err) return(err);
 softcs[u][mousep].open = 1;
 return(0);
}

int rwkmclose(dev_t dev, int flags, int mode, struct proc *p)
{
 int mousep;
 unsigned int u;

 u = minor(dev);
 mousep = u & 1;
 u >>= 1;
 mousep ? wsmouse_rwkm_close(u) : wskbd_rwkm_close(u);
 softcs[u][mousep].open = 0;
 return(0);
}

int rwkmread(dev_t dev, struct uio *uio, int flags)
{
 unsigned int u;

 u = minor(dev);
 return(wsevent_read(&softcs[u>>1][u&1].evq,uio,flags));
}

int rwkmwrite(dev_t dev, struct uio *uio, int flags)
{
 return(EIO);
}

int rwkmioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 return(ENOTTY);
}

int rwkmpoll(dev_t dev, int events, struct proc *p)
{
 unsigned int u;

 u = minor(dev);
 return(wsevent_poll(&softcs[u>>1][u&1].evq,events,p));
}

#endif
