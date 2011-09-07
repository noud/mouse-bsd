#include "ptape.h"
#if NPTAPE > 0

#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <machine/stdarg.h>

#include <dev/pseudo/ptape-kern.h>

typedef unsigned long int ID;

typedef struct ptape_softc SOFTC;
typedef struct seg SEG;

struct seg {
  SEG *link;
  void *data;
  unsigned int len;
  unsigned int ptr;
  } ;

struct ptape_softc {
  unsigned int flags;
#define PSF_MOPEN     0x00000001
#define PSF_SOPEN     0x00000002
#define PSF_READING   0x00000004
#define PSF_READWAIT  0x00000008
#define PSF_WRITING   0x00000010
#define PSF_WRITEWAIT 0x00000020
#define PSF_IOCTLING  0x00000040
#define PSF_IOCTLWAIT 0x00000080
#define PSF_REQBUSY   0x00000100
#define PSF_REQWAIT   0x00000200
#define PSF_REQREPLY  0x00000400
  SEG *q_stom;
  void *retbuf;
  size_t retlen;
  struct selinfo mrsel;
  } ;

static SOFTC psc[NPTAPE];

#define PTAPE_BLKSIZ 8192

static int free_unit(void)
{
 unsigned int u;

 for (u=0;u<NPTAPE;u++)
  { if (psc[u].flags & (PSF_MOPEN|PSF_SOPEN)) continue;
    return(u);
  }
 return(-1);
}

#define BLK(addr,len) ((const void *)(addr)),((size_t)(len))
static int mreq(SOFTC *sc, char op, void *retbuf, size_t retlen, int nblk, ...)
{
 SEG segs[8];
 int sx;
 int i;
 va_list ap;
 int err;

#ifdef PTAPE_VERBOSE
 printf("ptape mreq: sc=%p op=%c retbuf=%p retlen=%lu nblk=%d\n",
	(void *)sc,op,retbuf,(unsigned long int)retlen,nblk);
#endif
 while (sc->flags & PSF_REQBUSY)
  { sc->flags |= PSF_REQWAIT;
    err = tsleep(sc,PZERO|PCATCH,"mreq",0);
    if (err) return(err);
  }
 sc->flags |= PSF_REQBUSY;
 if (! sc->flags & PSF_MOPEN)
  { err = EIO;
    goto out;
  }
 if (nblk+1 > sizeof(segs)/sizeof(segs[0])) panic("ptape mreq: too few segs");
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: nblk=%d\n",nblk);
 printf("ptape mreq: blk 0: 1@%p\n",(void *)&op);
#endif
 segs[0].data = &op;
 segs[0].len = 1;
 segs[0].ptr = 0;
 sx = 0;
 va_start(ap,nblk);
 for (i=0;i<nblk;i++)
  { segs[sx].link = &segs[sx+1];
    sx ++;
    segs[sx].data = va_arg(ap,void *);
    segs[sx].len = va_arg(ap,size_t);
#ifdef PTAPE_VERBOSE
    printf("ptape mreq: blk %d: %d@%p\n",sx,segs[sx].len,(void *)segs[sx].data);
#endif
  }
 segs[sx++].link = 0;
 va_end(ap);
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: queue (%d):\n",sx);
  { int j;
    for (j=0;j<sx;j++) printf("    %p: %u@%p link=%p\n",(void *)&segs[j],segs[j].len,(void *)segs[j].data,(void *)segs[j].link);
  }
#endif
 sc->flags &= ~PSF_REQREPLY;
 sc->retbuf = retbuf;
 sc->retlen = retlen;
 sc->q_stom = &segs[0];
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: wakeup and wait\n");
#endif
 selwakeup(&sc->mrsel);
 wakeup(sc);
 while (1)
  { if (! (sc->flags & PSF_MOPEN))
     {
#ifdef PTAPE_VERBOSE
       printf("ptape mreq: master closed\n");
#endif
       err = EIO;
       goto out;
     }
    if (sc->flags & PSF_REQREPLY) break;
    err = tsleep(sc,PZERO|PCATCH,"mreq",0);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptape mreq: interrupted\n");
#endif
       goto out;
     }
  }
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: wait done\n");
#endif
out:;
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: finishing\n");
#endif
 sc->flags &= ~PSF_REQBUSY;
 if (sc->flags & PSF_REQWAIT)
  { sc->flags &= ~PSF_REQWAIT;
    wakeup(sc);
  }
#ifdef PTAPE_VERBOSE
 printf("ptape mreq: returning %d\n",err);
#endif
 return(err);
}

void ptapeattach(int arg __attribute__((__unused__)))
{
 int i;

 for (i=0;i<NPTAPE;i++)
  { psc[i].flags = 0;
    psc[i].q_stom = 0;
  }
}

int ptapemopen(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapemopen: unit %u\n",unit);
#endif
 if (unit == 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemopen: trivial success\n");
#endif
    return(0);
  }
 if (unit > NPTAPE)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemopen: ENXIO\n");
#endif
    return(ENXIO);
  }
 sc = &psc[unit-1];
 if (sc->flags & (PSF_MOPEN|PSF_SOPEN))
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemopen: EPERM\n");
#endif
    return(EBUSY);
  }
 sc->flags |= PSF_MOPEN;
#ifdef PTAPE_VERBOSE
 printf("ptapemopen: success\n");
#endif
 return(0);
}

int ptapemclose(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapemclose: unit %u\n",unit);
#endif
 if (unit == 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemclose: trivial\n");
#endif
    return(0);
  }
 sc = &psc[unit-1];
 sc->flags &= ~PSF_MOPEN;
 wakeup(sc);
#ifdef PTAPE_VERBOSE
 printf("ptapemclose: done\n");
#endif
 return(0);
}

int ptapemread(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 int n;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapemread: unit %u\n",unit);
#endif
 if (unit == 0)
  { if (uio->uio_resid < sizeof(int))
     {
#ifdef PTAPE_VERBOSE
       printf("ptapemread: unit 0 EIO\n");
#endif
       return(EIO);
     }
#ifdef PTAPE_VERBOSE
    printf("ptapemread: calling free_unit\n");
#endif
    n = free_unit();
#ifdef PTAPE_VERBOSE
    printf("ptapemread: free_unit -> %d\n",n);
    printf("ptapemread: calling uiomove\n");
#endif
    err = uiomove(&n,sizeof(int),uio);
#ifdef PTAPE_VERBOSE
    printf("ptapemread: unit 0 err=%d\n",err);
#endif
    return(err);
  }
 sc = &psc[unit-1];
 while (!(sc->flags&PSF_REQBUSY) || !sc->q_stom)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemread: unit %u waiting\n",unit);
#endif
    err = tsleep(sc,PZERO|PCATCH,"ptapemread",0);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapemread: data wait interrupted\n");
#endif
       return(err);
     }
  }
#ifdef PTAPE_VERBOSE
 printf("ptapemread: unit %u wait done\n",unit);
#endif
 while (sc->q_stom && uio->uio_resid)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemread: stom %p\n",(void *)sc->q_stom);
#endif
    n = sc->q_stom->len - sc->q_stom->ptr;
    if (n > uio->uio_resid) n = uio->uio_resid;
#ifdef PTAPE_VERBOSE
    printf("ptapemread: moving %d\n",n);
#endif
    err = uiomove(sc->q_stom->ptr+(char *)sc->q_stom->data,n,uio);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapemread: uiomove -> err %d\n",err);
#endif
       break;
     }
    sc->q_stom->ptr += n;
    if (sc->q_stom->ptr >= sc->q_stom->len)
     { sc->q_stom = sc->q_stom->link;
#ifdef PTAPE_VERBOSE
       printf("ptapemread: advanced stom %p\n",(void *)sc->q_stom);
#endif
       if (sc->q_stom) sc->q_stom->ptr = 0;
     }
  }
#ifdef PTAPE_VERBOSE
 printf("ptapemread: loop done\n");
#endif
 if (!sc->q_stom && !sc->retlen)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemread: no reply expected, setting REQREPLY\n");
#endif
    sc->flags |= PSF_REQREPLY;
    wakeup(sc);
  }
 sc->flags &= ~PSF_READING;
 if (sc->flags & PSF_READWAIT)
  { sc->flags &= ~PSF_READWAIT;
    wakeup(sc);
  }
 return(err);
}

int ptapemwrite(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 int n;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapemwrite: unit %u\n",unit);
#endif
 if (unit == 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemwrite: unit 0 EIO\n");
#endif
    return(EIO);
  }
 sc = &psc[unit-1];
 if (!(sc->flags&PSF_REQBUSY) || sc->q_stom)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemwrite: phase wrong\n");
#endif
    return(EIO);
  }
 while (sc->retlen && uio->uio_resid)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemwrite: loop, retlen %d\n",(int)sc->retlen);
#endif
    n = sc->retlen;
    if (n > uio->uio_resid) n = uio->uio_resid;
#ifdef PTAPE_VERBOSE
    printf("ptapemwrite: uiomoving %d\n",n);
#endif
    err = uiomove((char *)sc->retbuf,n,uio);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapemwrite: uiomove -> err %d\n",err);
#endif
       break;
     }
    sc->retbuf = n + (char *)sc->retbuf;
    sc->retlen -= n;
  }
#ifdef PTAPE_VERBOSE
 printf("ptapemwrite: loop done\n");
#endif
 if (sc->retlen < 1)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapemwrite: setting REQREPLY\n");
#endif
    sc->flags |= PSF_REQREPLY;
    wakeup(sc);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapemwrite: returning %d\n",err);
#endif
 return(err);
}

int ptapemioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
#ifdef PTAPE_VERBOSE
 printf("ptapemioctl: ENOTTY\n");
#endif
 return(ENOTTY);
}

int ptapempoll(dev_t dev, int events, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int revents;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapempoll: unit %u events %#x\n",unit,events);
#endif
 if (unit == 0) return(events&(POLLIN|POLLRDNORM));
 sc = &psc[unit-1];
 revents = 0;
 if (events & (POLLIN|POLLRDNORM))
  { if ((sc->flags&PSF_REQBUSY) && sc->q_stom)
     { revents |= events & (POLLIN | POLLRDNORM);
     }
    else
     { selrecord(p,&sc->mrsel);
     }
  }
 revents |= events & (POLLOUT|POLLWRNORM);
 return(revents);
}

int ptapesopen(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 char rw;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapesopen: unit %u\n",unit);
#endif
 if (unit == 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesopen: unit 0 ENXIO\n");
#endif
    return(ENXIO);
  }
 if (unit > NPTAPE)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesopen: unit number too large ENXIO\n");
#endif
    return(ENXIO);
  }
 sc = &psc[unit-1];
 if (! (sc->flags & PSF_MOPEN))
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesopen: master not open ENXIO\n");
#endif
    return(ENXIO);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesopen: calling mreq\n");
#endif
 switch (flag & (FREAD|FWRITE))
  { case FREAD:
       rw = 'r';
       break;
    case FWRITE:
       rw = 'w';
       break;
    case FREAD|FWRITE:
       rw = 'b';
       break;
    default:
#ifdef PTAPE_VERBOSE
       printf("ptapesopen: bad rw EIO\n");
#endif
       return(EIO);
       break;
  }
 if (mreq(sc,'o',&err,sizeof(int),1,BLK(&rw,1)))
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesopen: mreq failed EIO\n");
#endif
    return(EIO);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesopen: mreq ok, error from master %d\n",err);
#endif
 if (err == 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesopen: success\n");
#endif
    sc->flags |= PSF_SOPEN;
  }
 return(err);
}

int ptapesclose(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapesclose: unit %u\n",unit);
#endif
 if (unit == 0) panic("ptape: slave close unit 0");
 sc = &psc[unit-1];
#ifdef PTAPE_VERBOSE
 printf("ptapesclose: calling mreq\n");
#endif
 mreq(sc,'c',0,0,0);
 sc->flags &= ~PSF_SOPEN;
#ifdef PTAPE_VERBOSE
 printf("ptapesclose: done\n");
#endif
 return(0);
}

int ptapesread(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int nb;
 int err;
 char *dbuf;
 int cs;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapesread: unit %u\n",unit);
#endif
 if (unit == 0) panic("ptape: slave read unit 0");
 sc = &psc[unit-1];
 while (sc->flags & PSF_READING)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesread: unit %u busy, sleeping\n",unit);
#endif
    sc->flags |= PSF_READWAIT;
    err = tsleep(sc,PZERO|PCATCH,"ptaperead",0);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapesread: unit %u sleep interrupted\n",unit);
#endif
       return(err);
     }
  }
 sc->flags |= PSF_READING;
#ifdef PTAPE_VERBOSE
 printf("ptapesread: unit %u reading: calling mreq\n",unit);
#endif
 if (mreq(sc,'r',&nb,sizeof(nb),1,BLK("o",1)))
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesread: mreq failed EIO\n");
#endif
    err = EIO;
    goto out;
  }
 if (nb < 0)
  { err = - nb;
#ifdef PTAPE_VERBOSE
    printf("ptapesread: mreq ok, error from master %d\n",err);
#endif
    goto out;
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesread: mreq ok nb %d\n",nb);
#endif
 err = 0;
 if (nb > 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesread: calling malloc\n");
#endif
    dbuf = malloc(PTAPE_BLKSIZ,M_DEVBUF,M_WAITOK);
#ifdef PTAPE_VERBOSE
    printf("ptapesread: malloc done\n");
#endif
    while (1)
     { cs = nb;
       if (cs > PTAPE_BLKSIZ) cs = PTAPE_BLKSIZ;
       if (cs > uio->uio_resid) cs = uio->uio_resid;
#ifdef PTAPE_VERBOSE
       printf("ptapesread: loop, cs = %d\n",cs);
#endif
       if (cs == 0)
	{ mreq(sc,'r',0,0,1,BLK("c",1));
	  break;
	}
       if (mreq(sc,'r',dbuf,cs,2,BLK("r",1),BLK(&cs,sizeof(cs))))
	{
#ifdef PTAPE_VERBOSE
	  printf("ptapesread: loop, mreq failed EIO\n");
#endif
	  err = EIO;
	  goto outfree;
	}
#ifdef PTAPE_VERBOSE
       printf("ptapesread: loop, mreq done, calling uiomove\n");
#endif
       err = uiomove(dbuf,cs,uio);
       if (err)
	{
#ifdef PTAPE_VERBOSE
	  printf("ptapesread: loop, uiomove -> err %d\n",err);
#endif
	  mreq(sc,'r',0,0,1,BLK("c",1));
	  break;
	}
#ifdef PTAPE_VERBOSE
       printf("ptapesread: loop, uiomove ok\n");
#endif
       nb -= cs;
     }
#ifdef PTAPE_VERBOSE
    printf("ptapesread: loop done\n");
#endif
outfree:;
#ifdef PTAPE_VERBOSE
    printf("ptapesread: calling free\n");
#endif
    free(dbuf,M_DEVBUF);
  }
out:;
#ifdef PTAPE_VERBOSE
 printf("ptapesread: done\n");
#endif
 sc->flags &= ~PSF_READING;
 if (sc->flags & PSF_READWAIT)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesread: calling wakeup\n");
#endif
    sc->flags &= ~PSF_READWAIT;
    wakeup(sc);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesread: returning %d\n",err);
#endif
 return(err);
}

int ptapeswrite(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int nb;
 int err;
 char *dbuf;
 int cs;

 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapeswrite: unit %u\n",unit);
#endif
 if (unit == 0) panic("ptape: slave write unit 0");
 sc = &psc[unit-1];
 while (sc->flags & PSF_WRITING)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: unit %u busy, sleeping\n",unit);
#endif
    sc->flags |= PSF_WRITEWAIT;
    err = tsleep(sc,PZERO|PCATCH,"ptapewrite",0);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapeswrite: unit %u sleep interrupted\n",unit);
#endif
       return(err);
     }
  }
 sc->flags |= PSF_WRITING;
#ifdef PTAPE_VERBOSE
 printf("ptapeswrite: unit %u writing: calling mreq\n",unit);
#endif
 nb = uio->uio_resid;
 if (mreq(sc,'w',&nb,sizeof(nb),2,BLK("o",1),BLK(&nb,sizeof(nb))))
  {
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: mreq failed EIO\n");
#endif
    err = EIO;
    goto out;
  }
 if (nb < 0)
  { err = - nb;
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: mreq ok, error from master %d\n",err);
#endif
    goto out;
  }
#ifdef PTAPE_VERBOSE
 printf("ptapeswrite: mreq ok nb %d\n",nb);
#endif
 err = 0;
 if (nb > 0)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: calling malloc\n");
#endif
    dbuf = malloc(PTAPE_BLKSIZ,M_DEVBUF,M_WAITOK);
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: malloc done\n");
#endif
    while (1)
     { cs = nb;
       if (cs > PTAPE_BLKSIZ) cs = PTAPE_BLKSIZ;
       if (cs > uio->uio_resid) cs = uio->uio_resid;
#ifdef PTAPE_VERBOSE
       printf("ptapeswrite: loop, cs = %d\n",cs);
#endif
       if (cs == 0)
	{ mreq(sc,'w',0,0,1,BLK("c",1));
	  break;
	}
       err = uiomove(dbuf,cs,uio);
       if (err)
	{
#ifdef PTAPE_VERBOSE
	  printf("ptapeswrite: loop, uiomove -> err %d\n",err);
#endif
	  mreq(sc,'w',0,0,1,BLK("c",1));
	  break;
	}
#ifdef PTAPE_VERBOSE
       printf("ptapeswrite: uiomove ok, calling mreq\n");
#endif
       if (mreq(sc,'w',0,0,3,BLK("w",1),BLK(&cs,sizeof(cs)),BLK(dbuf,cs)))
	{
#ifdef PTAPE_VERBOSE
	  printf("ptapeswrite: loop, mreq failed EIO\n");
#endif
	  err = EIO;
	  goto outfree;
	}
       nb -= cs;
     }
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: loop done\n");
#endif
outfree:;
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: calling free\n");
#endif
    free(dbuf,M_DEVBUF);
  }
out:;
#ifdef PTAPE_VERBOSE
 printf("ptapeswrite: done\n");
#endif
 sc->flags &= ~PSF_WRITING;
 if (sc->flags & PSF_WRITEWAIT)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapeswrite: calling wakeup\n");
#endif
    sc->flags &= ~PSF_WRITEWAIT;
    wakeup(sc);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapeswrite: returning %d\n",err);
#endif
 return(err);
}

int ptapesioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 int nb;

 if (IOCPARM_LEN(cmd) > IOCPARM_MAX) return(ENOTTY);
 unit = minor(dev);
#ifdef PTAPE_VERBOSE
 printf("ptapesioctl: unit %u cmd %lx data %p pid %d\n",unit,(unsigned long int)cmd,(void *)data,(int)p->p_pid);
#endif
 if (unit == 0) panic("ptape: slave ioctl unit 0");
 sc = &psc[unit-1];
 while (sc->flags & PSF_IOCTLING)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesioctl: unit %u busy, sleeping\n",unit);
#endif
    sc->flags |= PSF_IOCTLWAIT;
    err = tsleep(sc,PZERO|PCATCH,"ptapeioctl",0);
    if (err)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapesioctl: unit %u sleep interrupted\n",unit);
#endif
       return(err);
     }
  }
 sc->flags |= PSF_IOCTLING;
 if (cmd & IOC_IN)
  { nb = IOCPARM_LEN(cmd);
#ifdef PTAPE_VERBOSE
    printf("ptapesioctl: unit %u ioctling: calling mreq (IN, nb %d)\n",unit,nb);
#endif
    if (mreq(sc,'i',&err,sizeof(err),3,BLK("o",1),BLK(&cmd,sizeof(cmd)),BLK(data,nb)))
     {
#ifdef PTAPE_VERBOSE
       printf("ptapesioctl: mreq failed EIO\n");
#endif
       err = EIO;
       goto out;
     }
  }
 else
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesioctl: unit %u ioctling: calling mreq (not IN)\n",unit);
#endif
    if (mreq(sc,'i',&err,sizeof(err),2,BLK("o",1),BLK(&cmd,sizeof(cmd))))
     {
#ifdef PTAPE_VERBOSE
       printf("ptapesioctl: mreq failed EIO\n");
#endif
       err = EIO;
       goto out;
     }
  }
 if (err)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesioctl: mreq ok, error from master %d\n",err);
#endif
    goto out;
  }
 if (cmd & IOC_OUT)
  { nb = IOCPARM_LEN(cmd);
    if (nb > 0)
     {
#ifdef PTAPE_VERBOSE
       printf("ptapesioctl: OUT, calling mreq (nb %d)\n",nb);
#endif
       if (mreq(sc,'i',data,nb,2,BLK("d",1),BLK(&nb,sizeof(nb))))
	{
#ifdef PTAPE_VERBOSE
	  printf("ptapesioctl: mreq failed EIO\n");
#endif
	  err = EIO;
	  goto out;
	}
     }
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesioctl: successful\n");
#endif
out:;
#ifdef PTAPE_VERBOSE
 printf("ptapesioctl: done\n");
#endif
 sc->flags &= ~PSF_IOCTLING;
 if (sc->flags & PSF_IOCTLWAIT)
  {
#ifdef PTAPE_VERBOSE
    printf("ptapesioctl: calling wakeup\n");
#endif
    sc->flags &= ~PSF_IOCTLWAIT;
    wakeup(sc);
  }
#ifdef PTAPE_VERBOSE
 printf("ptapesioctl: returning %d\n",err);
#endif
 return(err);
}

int ptapespoll(dev_t dev, int events, struct proc *p)
{
#ifdef PTAPE_VERBOSE
 printf("ptapempoll: unit %u events %#x\n",(unsigned int)minor(dev),events);
#endif
 return(events&(POLLIN|POLLOUT|POLLRDNORM|POLLWRNORM));
}

#endif
