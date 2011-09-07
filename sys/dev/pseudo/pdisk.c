#include "pdisk.h"
#if NPDISK > 0

/* This file interlocks against itself in non-MP-safe ways.  Look for
   anything ivolving "lock" or "LOCK" to find the places. */

#define PDISK_VERBOSE

#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/disklabel.h>
#include <machine/stdarg.h>

#include <dev/pseudo/pdisk-kern.h>

typedef unsigned long int ID;

typedef struct pdisk_softc SOFTC;
typedef struct seg SEG;
typedef struct ioq IOQ;

struct ioq {
  IOQ *link;
  unsigned int blkno;
  unsigned int nblks;
  char *data;
  } ;

struct seg {
  SEG *link;
  void *data;
  unsigned int len;
  unsigned int ptr;
  } ;

struct pdisk_softc {
  unsigned int unit;
  volatile unsigned int flags;
#define PSF_MOPEN     0x00000001
#define PSF_SOPEN     0x00000002
#define PSF_REQBUSY   0x00000004
#define PSF_REQWAIT   0x00000008
#define PSF_REQREPLY  0x00000010
#define PSF_SOLOCK    0x00000020
#define PSF_SOWANT    0x00000040
#define PSF_WLABEL    0x00000080
#define PSF_LABELLING 0x00000100
#define PSF_LABLOCK   0x00000200
#define PSF_LABWANT   0x00000400
#define PSF_HAVESIZE  0x00000800
#define PSF_ASYNCLOCK 0x00001000
#define PSF_ASYNCWANT 0x00002000
#define PSF_HAVELABEL 0x00004000
#define PSF_KEEPLABEL 0x00008000
#define PSF_READREQ   0x00010000
  unsigned int size;
  volatile unsigned int copenmask;
  volatile unsigned int bopenmask;
  volatile unsigned int openmask;
  volatile size_t len_stom;
  volatile size_t len_mtos;
  volatile int iobp;
#define IOBUFSIZE (1+sizeof(int)+DEV_BSIZE)
  void *iobuf;
  struct selinfo mrsel;
  struct disk dkdev;
  char xname[32];
  char *wait;
  IOQ *async;
  IOQ **tasync;
  } ;

static SOFTC *psc;

#ifdef PDISK_VERBOSE
static int verbose;
#define VERBOSE(n) (verbose & PDV_##n)
#define PDV_MREQ  0x00000001
#define PDV_DATA  0x00000002
#define PDV_READ  0x00000004
#define PDV_WRITE 0x00000008
#define PDV_PUSH  0x00000010
#define PDV_OPEN  0x00000020
#define PDV_STRAT 0x00000040
#define PDV_FLUSH 0x00000080
#else
#define VERBOSE(n) 0
#endif

#define PDISK_BLKSIZ 8192

static int free_unit(void)
{
 unsigned int u;

 for (u=0;u<NPDISK;u++)
  { if (psc[u].flags & (PSF_MOPEN|PSF_SOPEN)) continue;
    return(u);
  }
 return(-1);
}

static int pdisklock(SOFTC *sc, unsigned int lockbit, unsigned int wantbit, const char *wchan)
{
 int err;

 while (sc->flags & lockbit)
  { sc->flags |= wantbit;
    err = tsleep(sc->wait,PRIBIO|PCATCH,wchan,0);
    if (err) return(err);
  }
 sc->flags |= lockbit;
 return(0);
}

static void pdiskunlock(SOFTC *sc, unsigned int lockbit, unsigned int wantbit)
{
 sc->flags &= ~lockbit;
 if (sc->flags & wantbit)
  { sc->flags &= ~wantbit;
    wakeup(sc->wait);
  }
}

#define BLK(addr,len) ((const void *)(addr)),((size_t)(len))
static int mreq(SOFTC *sc, char op, size_t retlen, int nblk, ...)
{
 char *bp;
 int bf;
 int i;
 va_list ap;
 int err;

 if (VERBOSE(MREQ)) printf("pdisk mreq: sc=%p op=%c retlen=%lu nblk=%d\n",
		(void *)sc,op,(unsigned long int)retlen,nblk);
 if (retlen > IOBUFSIZE) panic("pdisk mreq: retlen > %d",(int)IOBUFSIZE);
 while (sc->flags & PSF_REQBUSY)
  { sc->flags |= PSF_REQWAIT;
    err = tsleep(sc->wait,PZERO|PCATCH,"mreq",0);
    if (err) return(err);
  }
 sc->flags |= PSF_REQBUSY;
 if (! sc->flags & PSF_MOPEN)
  { err = EIO;
    goto out;
  }
 if (VERBOSE(MREQ))
  { printf("pdisk mreq: nblk=%d\n",nblk);
    printf("pdisk mreq: 1@%p\n",(void *)&op);
  }
 bp = sc->iobuf;
 bf = 0;
 *bp++ = op;
 bf ++;
 va_start(ap,nblk);
 for (i=0;i<nblk;i++)
  { void *data;
    size_t len;
    data = va_arg(ap,void *);
    len = va_arg(ap,size_t);
    if (bf+len > IOBUFSIZE) panic("pdisk mreq: iobuf overflow: %d+%d > %d",bf,(int)len,(int)IOBUFSIZE);
    bcopy(data,bp,len);
    bp += len;
    bf += len;
    if (VERBOSE(MREQ)) printf("pdisk mreq: %d@%p\n",(int)len,data);
  }
 va_end(ap);
 if (VERBOSE(DATA))
  { int i;
    int l;
    printf("pdisk stom in:");
    l = (bf < 16) ? bf : 16;
    for (i=0;i<l;i++) printf(" %02x",((unsigned char *)sc->iobuf)[i]);
    if (l != bf) printf(" ...");
    printf("\n");
  }
 sc->flags &= ~PSF_REQREPLY;
 sc->len_stom = bf;
 sc->len_mtos = retlen;
 sc->iobp = 0;
 sc->flags |= PSF_READREQ;
 if (VERBOSE(MREQ)) printf("pdisk mreq: wakeup and wait\n");
 selwakeup(&sc->mrsel);
 wakeup(sc->wait);
 while (1)
  { if (! (sc->flags & PSF_MOPEN))
     { if (VERBOSE(MREQ)) printf("pdisk mreq: master closed\n");
       err = EIO;
       goto out;
     }
    if (sc->flags & PSF_REQREPLY) break;
    err = tsleep(sc->wait,PZERO|PCATCH,"mreq",0);
    if (err)
     { if (VERBOSE(MREQ)) printf("pdisk mreq: interrupted\n");
       goto out;
     }
  }
 if (VERBOSE(MREQ)) printf("pdisk mreq: wait done\n");
 if (VERBOSE(DATA))
  { int i;
    int l;
    printf("pdisk mtos out:");
    l = sc->len_mtos;
    if (l > 16) l = 16;
    for (i=0;i<l;i++) printf(" %02x",((unsigned char *)sc->iobuf)[i]);
    if (l != sc->len_mtos) printf(" ...");
    printf("\n");
  }
out:;
 if (VERBOSE(MREQ)) printf("pdisk mreq: finishing\n");
 sc->flags &= ~PSF_REQBUSY;
 if (sc->flags & PSF_REQWAIT)
  { sc->flags &= ~PSF_REQWAIT;
    wakeup(sc->wait);
  }
 if (VERBOSE(MREQ)) printf("pdisk mreq: returning %d\n",err);
 return(err);
}

void pdiskattach(int arg __attribute__((__unused__)))
{
 void *mem;
 int i;
 SOFTC *sc;
 char *waits;

 mem = malloc(NPDISK*(sizeof(SOFTC)+1),M_DEVBUF,M_NOWAIT);
 if (mem == 0)
  { printf("pdisk: attach malloc failed\n");
    return;
  }
 psc = mem;
 waits = (NPDISK * sizeof(SOFTC)) + (char *)mem;
 for (i=0;i<NPDISK;i++)
  { sc = &psc[i];
    sc->unit = i;
    sc->flags = 0;
    sc->bopenmask = 0;
    sc->copenmask = 0;
    sc->openmask = 0;
    sc->async = 0;
    sc->tasync = &sc->async;
    sc->wait = waits++;
  }
}

static int readablock(SOFTC *sc, unsigned int blkno, void *data)
{
 int err;

 if (VERBOSE(READ)) printf("pdisk read: unit %u reading: calling mreq\n",sc->unit);
 if (mreq(sc,'r',sizeof(int)+DEV_BSIZE,1,BLK(&blkno,sizeof(blkno))))
  { if (VERBOSE(READ)) printf("pdisk read: mreq failed EIO\n");
    return(EIO);
  }
 err = *(int *)sc->iobuf;
 if (err)
  { if (VERBOSE(READ)) printf("pdisk read: mreq ok, error from master %d\n",err);
    return(err);
  }
 if (VERBOSE(READ)) printf("pdisk read: mreq ok, copying to %p\n",data);
 bcopy(sizeof(int)+(char *)sc->iobuf,data,DEV_BSIZE);
 if (VERBOSE(READ)) printf("pdisk read: done\n");
 if (VERBOSE(DATA))
  { int i;
    printf("pdisk read: data");
    for (i=0;i<16;i++) printf(" %02x",((unsigned char *)data)[i]);
    printf(" ...\n");
  }
 return(0);
}

static int writeablock(SOFTC *sc, unsigned int blkno, const void *data)
{
 int err;

 if (VERBOSE(WRITE)) printf("pdisk write: unit %u writing: calling mreq\n",sc->unit);
 if (mreq(sc,'w',sizeof(err),2,BLK(&blkno,sizeof(blkno)),BLK(data,DEV_BSIZE)))
  { if (VERBOSE(WRITE)) printf("pdisk write: mreq failed EIO\n");
    return(EIO);
  }
 err = *(int *)sc->iobuf;
 if (err)
  { if (VERBOSE(WRITE)) printf("pdisk write: mreq ok, error from master %d\n",err);
    return(err);
  }
 if (VERBOSE(WRITE)) printf("pdisk write: done\n");
 return(0);
}

static int reallydoio(SOFTC *sc, IOQ *q, int forread)
{
 int err;

 while (q->nblks > 0)
  { if (forread) err = readablock(sc,q->blkno,q->data);
    else         err = writeablock(sc,q->blkno,q->data);
    if (err) return(err);
    q->blkno ++;
    q->data += DEV_BSIZE;
    q->nblks --;
  }
 return(0);
}

static void pusher(void *arg)
{
 SOFTC *sc;
 volatile SOFTC *vsc;
 IOQ *q;

 sc = arg;
 vsc = sc;
 while (1)
  { if (! (vsc->flags & PSF_MOPEN)) kthread_exit(0);
    pdisklock(sc,PSF_ASYNCLOCK,PSF_ASYNCWANT,"pdpalck");
    if (vsc->async == 0)
     { /* Depend on nobody else being able to frob the async list between
	  pdiskunlock and giving up the CPU in tsleep - XXX not MP-safe */
       pdiskunlock(sc,PSF_ASYNCLOCK,PSF_ASYNCWANT);
       wakeup(&sc->async);
       tsleep(sc->wait,PRIBIO,"psfpush",0);
       continue;
     }
    q = vsc->async;
    if (VERBOSE(PUSH)) printf("pdisk%u: pushing %p\n",sc->unit,(void *)q);
    if (! (vsc->async=q->link)) vsc->tasync = &sc->async;
    pdiskunlock(sc,PSF_ASYNCLOCK,PSF_ASYNCWANT);
    /* Again, depend on nobody else being able to try to do I/O after the
       previous line and before reallydoio locks in mreq(). XXX */
    reallydoio(sc,q,0);
    free(q,M_DEVBUF);
  }
}

int pdiskmopen(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int xunit;

 xunit = minor(dev);
 if (VERBOSE(OPEN)) printf("pdiskmopen: unit %u\n",xunit);
 if (xunit == 0)
  { if (VERBOSE(OPEN)) printf("pdiskmopen: trivial success\n");
    return(0);
  }
 if (xunit > NPDISK)
  { if (VERBOSE(OPEN)) printf("pdiskmopen: ENXIO\n");
    return(ENXIO);
  }
 sc = &psc[xunit-1];
 if (sc->flags & (PSF_MOPEN|PSF_SOPEN))
  { if (VERBOSE(OPEN)) printf("pdiskmopen: EBUSY\n");
    return(EBUSY);
  }
 sc->flags |= PSF_MOPEN;
 sprintf(&sc->xname[0],"pdisk%u",sc->unit);
 sc->dkdev.dk_name = &sc->xname[0];
 disk_attach(&sc->dkdev);
 sc->iobuf = malloc(sizeof(int)+DEV_BSIZE,M_DEVBUF,M_WAITOK);
 if (kthread_create1(pusher,sc,0,"pdisk%upush",sc->unit)) panic("can't fork pdisk %u pusher",sc->unit);
 if (VERBOSE(OPEN)) printf("pdiskmopen: success\n");
 return(0);
}

static void maybe_detach(SOFTC *sc)
{
 if ((sc->flags & (PSF_MOPEN|PSF_SOPEN)) == 0) disk_detach(&sc->dkdev);
}

int pdiskmclose(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;

 unit = minor(dev);
 if (VERBOSE(OPEN)) printf("pdiskmclose: unit %u\n",unit);
 if (unit == 0)
  { if (VERBOSE(OPEN)) printf("pdiskmclose: trivial\n");
    return(0);
  }
 sc = &psc[unit-1];
 sc->flags &= ~PSF_MOPEN;
 wakeup(sc->wait);
 maybe_detach(sc);
 if (VERBOSE(OPEN)) printf("pdiskmclose: done\n");
 return(0);
}

int pdiskmread(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 int n;

 unit = minor(dev);
 if (VERBOSE(READ)) printf("pdiskmread: unit %u\n",unit);
 if (unit == 0)
  { if (uio->uio_resid < sizeof(int))
     { if (VERBOSE(READ)) printf("pdiskmread: unit 0 EIO\n");
       return(EIO);
     }
    if (VERBOSE(READ)) printf("pdiskmread: calling free_unit\n");
    n = free_unit();
    if (VERBOSE(READ))
     { printf("pdiskmread: free_unit -> %d\n",n);
       printf("pdiskmread: calling uiomove\n");
     }
    err = uiomove(&n,sizeof(int),uio);
    if (VERBOSE(READ)) printf("pdiskmread: unit 0 err=%d\n",err);
    return(err);
  }
 sc = &psc[unit-1];
 while (! (sc->flags & PSF_REQBUSY))
  { if (VERBOSE(READ)) printf("pdiskmread: unit %u waiting\n",unit);
    err = tsleep(sc->wait,PZERO|PCATCH,"pdmread",0);
    if (err)
     { if (VERBOSE(READ)) printf("pdiskmread: data wait interrupted\n");
       return(err);
     }
  }
 if (VERBOSE(READ)) printf("pdiskmread: unit %u wait done\n",unit);
 if (! (sc->flags & PSF_READREQ))
  { if (VERBOSE(READ)) printf("pdiskmread: unit %u no more request\n",unit);
    return(0);
  }
 if (uio->uio_resid < 1)
  { if (VERBOSE(READ)) printf("pdiskmread: unit %d zero read\n",unit);
    return(0);
  }
 while (1)
  { n = sc->len_stom - sc->iobp;
    if (n > uio->uio_resid) n = uio->uio_resid;
    if (n < 1) return(0);
    if (VERBOSE(READ)) printf("pdiskmread: moving %d\n",n);
    err = uiomove(sc->iobp+(char *)sc->iobuf,n,uio);
    if (err)
     { if (VERBOSE(READ)) printf("pdiskmread: uiomove -> err %d\n",err);
       break;
     }
    if (VERBOSE(DATA))
     { int i;
       int l;
       printf("pdisk stom out @%d:",sc->iobp);
       l = (n < 16) ? n : 16;
       for (i=0;i<l;i++) printf(" %02x",((unsigned char *)sc->iobuf)[sc->iobp+i]);
       if (l != n) printf(" ...");
       printf("\n");
     }
    if ((sc->iobp += n) >= sc->len_stom)
     { sc->flags &= ~PSF_READREQ;
       sc->iobp = 0;
       break;
     }
  }
 if (VERBOSE(READ)) printf("pdiskmread: loop done\n");
 if (!(sc->flags & PSF_READREQ) && !sc->len_mtos)
  { if (VERBOSE(READ)) printf("pdiskmread: no reply expected, setting REQREPLY\n");
    sc->flags |= PSF_REQREPLY;
    wakeup(sc->wait);
  }
 return(err);
}

int pdiskmwrite(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 int n;

 unit = minor(dev);
 if (VERBOSE(WRITE)) printf("pdiskmwrite: unit %u\n",unit);
 if (unit == 0)
  {
#ifdef PDISK_VERBOSE
    unsigned char buf[2];
    if (uio->uio_resid < 1)
     { if (VERBOSE(WRITE)) printf("pdiskmwrite: zero-length\n");
       return(0);
     }
    if (uio->uio_resid < 2)
     { if (VERBOSE(WRITE)) printf("pdiskmwrite: too small\n");
       return(EMSGSIZE);
     }
    err = uiomove(&buf[0],2,uio);
    if (err)
     { if (VERBOSE(WRITE)) printf("pdiskmwrite: uiomove failed %d\n",err);
       return(err);
     }
    if (buf[0]) verbose |= 1 << buf[1]; else verbose &= ~(1 << buf[1]);
    return(0);
#else
    return(EIO);
#endif
  }
 sc = &psc[unit-1];
 if ((sc->flags&(PSF_REQBUSY|PSF_READREQ)) != PSF_REQBUSY)
  { if (VERBOSE(WRITE)) printf("pdiskmwrite: phase wrong\n");
    return(EIO);
  }
 while (((n = sc->len_mtos - sc->iobp) > 0) && uio->uio_resid)
  { if (VERBOSE(WRITE)) printf("pdiskmwrite: loop, n %d\n",n);
    if (n > uio->uio_resid) n = uio->uio_resid;
    if (VERBOSE(WRITE)) printf("pdiskmwrite: uiomoving %d\n",n);
    err = uiomove(sc->iobp+(char *)sc->iobuf,n,uio);
    if (VERBOSE(DATA))
     { int i;
       int l;
       printf("pdisk mtos in @%d:",sc->iobp);
       l = (n < 16) ? n : 16;
       for (i=0;i<l;i++) printf(" %02x",((unsigned char *)sc->iobuf)[sc->iobp+i]);
       if (l != n) printf(" ...");
       printf("\n");
     }
    if (err)
     { if (VERBOSE(WRITE)) printf("pdiskmwrite: uiomove -> err %d\n",err);
       break;
     }
    sc->iobp += n;
  }
 if (VERBOSE(WRITE)) printf("pdiskmwrite: loop done\n");
 if (sc->iobp >= sc->len_mtos)
  { if (VERBOSE(WRITE)) printf("pdiskmwrite: setting REQREPLY\n");
    sc->flags |= PSF_REQREPLY;
    wakeup(sc->wait);
  }
 if (VERBOSE(WRITE)) printf("pdiskmwrite: returning %d\n",err);
 return(err);
}

int pdiskmioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 if (VERBOSE(OPEN)) printf("pdiskmioctl: ENOTTY\n");
 return(ENOTTY);
}

int pdiskmpoll(dev_t dev, int events, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int revents;

 unit = minor(dev);
 if (VERBOSE(OPEN)) printf("pdiskmpoll: unit %u events %#x\n",unit,events);
 if (unit == 0) return(events&(POLLIN|POLLRDNORM));
 sc = &psc[unit-1];
 revents = 0;
 if (events & (POLLIN|POLLRDNORM))
  { if ((sc->flags&(PSF_REQBUSY|PSF_READREQ)) == (PSF_REQBUSY|PSF_READREQ))
     { revents |= events & (POLLIN | POLLRDNORM);
     }
    else
     { selrecord(p,&sc->mrsel);
     }
  }
 revents |= events & (POLLOUT|POLLWRNORM);
 return(revents);
}

static int getsize(SOFTC *sc)
{
 int v;

 if (! (sc->flags & PSF_HAVESIZE))
  { if (mreq(sc,'z',sizeof(int),0)) return(-EIO);
    v = *(int *)sc->iobuf;
    if (VERBOSE(OPEN)) printf("pdisk%u getsize -> %d\n",sc->unit,v);
    if (v < 0) return(v);
    sc->size = v;
    sc->flags |= PSF_HAVESIZE;
  }
 return(sc->size);
}

static void default_label(SOFTC *sc, struct disklabel *label)
{
 int size;

 size = getsize(sc);
 if (size < 0) size = 0;
 bzero(label,sizeof(*label));
 label->d_secperunit = size;
 label->d_secsize = DEV_BSIZE;
 label->d_nsectors = 32;
 label->d_ntracks = 64;
 label->d_ncylinders = howmany(size,32*64);
 label->d_secpercyl = 2048;
 strncpy(&label->d_typename[0],"pdisk",sizeof(label->d_typename));
 label->d_type = 0; /* XXX there's no DTYPE_OTHER! */
 strncpy(&label->d_packname[0],"fictitious",sizeof(label->d_packname));
 label->d_rpm = 3600;
 label->d_interleave = 1;
 label->d_flags = 0;
 label->d_partitions[RAW_PART].p_offset = 0;
 label->d_partitions[RAW_PART].p_size = label->d_secperunit;
 label->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
 label->d_npartitions = RAW_PART + 1;
 label->d_magic = DISKMAGIC;
 label->d_magic2 = DISKMAGIC;
 label->d_checksum = dkcksum(label);
}

static void flush_cache(void)
{
 static void *buf = 0;
 static unsigned char c = 0;
 int i;

 if (buf == 0) buf = malloc(1048576,M_DEVBUF,M_WAITOK);
 c ++;
 for (i=0;i<1048576;i+=16) ((unsigned char *)buf)[i] = c;
}

void pdisksstrategy(struct buf *bp)
{
 unsigned int unit;
 unsigned int part;
 SOFTC *sc;
 struct disklabel *label;
 unsigned int blkno;
 int err;
 IOQ q;

 unit = DISKUNIT(bp->b_dev);
 if (VERBOSE(STRAT)) printf("pdisksstrategy: bp=%p unit %u\n",(void *)bp,unit);
 sc = &psc[unit];
 if (! (sc->flags & PSF_MOPEN))
  { if (VERBOSE(STRAT)) printf("pdisksstrategy: master not open\n");
    bp->b_error = ENXIO;
    bp->b_flags |= B_ERROR;
    biodone(bp);
    return;
  }
 if (bp->b_bcount == 0)
  { if (VERBOSE(STRAT)) printf("pdisksstrategy: zero length\n");
    biodone(bp);
    return;
  }
 label = sc->dkdev.dk_label;
 if ( (label->d_secsize % DEV_BSIZE) ||
      (bp->b_bcount % label->d_secsize) )
  { if (VERBOSE(STRAT)) printf("pdisksstrategy: not sector-aligned\n");
    bp->b_error = EINVAL;
    bp->b_flags |= B_ERROR;
    biodone(bp);
    return;
  }
 part = DISKPART(bp->b_dev);
 if ( (part != RAW_PART) &&
      (bounds_check_with_label(bp,label,sc->flags&(PSF_WLABEL|PSF_LABELLING)) <= 0) )
  { if (VERBOSE(STRAT)) printf("pdisksstrategy: label rejected\n");
    biodone(bp);
    return;
  }
 if (bp->b_bcount % DEV_BSIZE) panic("pdisksstrategy unaligned size");
 blkno = bp->b_blkno / (label->d_secsize / DEV_BSIZE);
 if (part != RAW_PART) blkno += label->d_partitions[part].p_offset;
 q.blkno = blkno * (label->d_secsize / DEV_BSIZE);
 if (VERBOSE(STRAT)) printf("pdisksstrategy: part=%d blkno=%u data=%p\n",part,q.blkno,(void *)bp->b_data);
 q.data = bp->b_data;
 q.nblks = bp->b_bcount / DEV_BSIZE;
 bp->b_resid = 0;
 if (bp->b_flags & B_READ)
  { volatile unsigned int *p;
    int i;
    p = (void *)bp->b_data;
    for (i=bp->b_bcount/sizeof(unsigned int);i>0;i--) *p++ = 0xfeedface;
    if (VERBOSE(FLUSH)) flush_cache();
  }
 if ((bp->b_flags & B_ASYNC) && !(bp->b_flags & B_READ))
  { IOQ *qp;
    qp = malloc(sizeof(IOQ)+bp->b_bcount,M_DEVBUF,M_WAITOK);
    qp->link = 0;
    qp->blkno = q.blkno;
    qp->data = (void *) (qp+1);
    qp->nblks = q.nblks;
    bcopy(q.data,qp->data,bp->b_bcount);
    err = pdisklock(sc,PSF_ASYNCLOCK,PSF_ASYNCWANT,"pdalck");
    if (err)
     { bp->b_error = err;
       bp->b_flags |= B_ERROR;
       biodone(bp);
       free(qp,M_DEVBUF);
       return;
     }
    *((volatile SOFTC *)sc)->tasync = qp;
    ((volatile SOFTC *)sc)->tasync = &qp->link;
    if (VERBOSE(STRAT)) printf("pdisk%u: queued %p\n",sc->unit,(void *)qp);
    pdiskunlock(sc,PSF_ASYNCLOCK,PSF_ASYNCWANT);
    wakeup(sc->wait);
  }
 else
  { while (((volatile SOFTC *)sc)->async)
     { err = tsleep(&sc->async,PRIBIO|PCATCH,"pdpush",hz);
       if (err && (err != EWOULDBLOCK))
	{ bp->b_error = err;
	  bp->b_flags |= B_ERROR;
	  biodone(bp);
	  return;
	}
     }
    err = reallydoio(sc,&q,bp->b_flags&B_READ);
    if (err)
     { bp->b_error = err;
       bp->b_flags |= B_ERROR;
     }
  }
 if (VERBOSE(FLUSH)) flush_cache();
 if (VERBOSE(DATA) && (bp->b_flags&B_READ))
  { int i;
    printf("pdisk strat read:");
    for (i=0;i<16;i++) printf(" %02x",((unsigned char *)sc->iobuf)[i]);
    printf(" ... err=%d\n",err);
  }
 biodone(bp);
}

static void load_label(dev_t dev, SOFTC *sc)
{
 const char *why;

 if (sc->flags & PSF_HAVELABEL) return;
 if (VERBOSE(OPEN)) printf("load_label dk_label=%p dk_cpulabel=%p\n",(void *)sc->dkdev.dk_label,(void *)sc->dkdev.dk_cpulabel);
 sc->flags |= PSF_HAVELABEL;
 /* XXX readdisklabel assumes the label is already partially set up! */
 default_label(sc,sc->dkdev.dk_label);
 why = readdisklabel(MAKEDISKDEV(major(dev),sc->unit,RAW_PART),pdisksstrategy,sc->dkdev.dk_label,sc->dkdev.dk_cpulabel);
 if (why)
  { printf("pdisk%d: %s\n",sc->unit,why);
    default_label(sc,sc->dkdev.dk_label);
  }
}

int pdisksopen(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int err;
 char rw;
 unsigned char part;

 unit = DISKUNIT(dev);
 if (VERBOSE(OPEN)) printf("pdisksopen: unit %u\n",unit);
 if (unit >= NPDISK)
  { if (VERBOSE(OPEN)) printf("pdisksopen: unit number too large ENXIO\n");
    return(ENXIO);
  }
 sc = &psc[unit];
 if (! (sc->flags & PSF_MOPEN))
  { if (VERBOSE(OPEN)) printf("pdisksopen: master not open ENXIO\n");
    return(ENXIO);
  }
 err = pdisklock(sc,PSF_SOLOCK,PSF_SOWANT,"pdsopen");
 if (err) return(err);
 part = DISKPART(dev);
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
       if (VERBOSE(OPEN)) printf("pdisksopen: bad rw EIO\n");
       pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
       return(EIO);
       break;
  }
 if (VERBOSE(OPEN)) printf("pdisksopen: calling mreq\n");
 if (mreq(sc,'o',sizeof(int),2,BLK(&rw,1),BLK(&part,1)))
  { if (VERBOSE(OPEN)) printf("pdisksopen: mreq failed EIO\n");
    pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
    return(EIO);
  }
 err = *(int *)sc->iobuf;
 if (VERBOSE(OPEN)) printf("pdisksopen: mreq ok, error from master %d\n",err);
 if (err)
  { pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
    return(err);
  }
 if (VERBOSE(OPEN)) printf("pdisksopen: conditional success\n");
 sc->flags |= PSF_SOPEN;
 if (sc->openmask == 0) load_label(dev,sc);
 if (VERBOSE(OPEN)) printf("pdisksopen: label loaded\n");
 if ( (part != RAW_PART) &&
      ( (part >= sc->dkdev.dk_label->d_npartitions) ||
	(sc->dkdev.dk_label->d_partitions[part].p_fstype == FS_UNUSED) ) )
  { if (VERBOSE(OPEN)) printf("pdisksopen: invalid partition\n");
    if (sc->openmask == 0)
     { mreq(sc,'c',0,0);
       sc->flags &= ~PSF_SOPEN;
     }
    pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
    return(ENXIO);
  }
 switch (mode)
  { case S_IFCHR:
       sc->copenmask |= 1 << part;
       break;
    case S_IFBLK:
       sc->bopenmask |= 1 << part;
       break;
    default:
       printf("pdisksopen opening unknown mode %d\n",mode);
       panic("pdisksopen");
       break;
  }
 sc->openmask = sc->copenmask | sc->bopenmask;
 pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
 if (VERBOSE(OPEN)) printf("pdisksopen: returning\n");
 return(0);
}

int pdisksclose(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 unsigned int part;
 int err;

 unit = DISKUNIT(dev);
 if (VERBOSE(OPEN)) printf("pdisksclose: unit %u\n",unit);
 sc = &psc[unit];
 part = DISKPART(dev);
 err = pdisklock(sc,PSF_SOLOCK,PSF_SOWANT,"pdsclose");
 if (err) return(err);
 switch (mode)
  { case S_IFCHR:
       sc->copenmask &= ~(1 << part);
       break;
    case S_IFBLK:
       sc->bopenmask &= ~(1 << part);
       break;
    default:
       printf("pdisksclose closing unknown mode %d\n",mode);
       panic("pdisksclose");
       break;
  }
 sc->openmask = sc->copenmask | sc->bopenmask;
 if (sc->openmask == 0)
  { if (VERBOSE(OPEN)) printf("pdisksclose: calling mreq\n");
    mreq(sc,'c',0,0);
    sc->flags &= ~PSF_SOPEN;
    if (! (sc->flags & PSF_KEEPLABEL)) sc->flags &= ~PSF_HAVELABEL;
    maybe_detach(sc);
  }
 pdiskunlock(sc,PSF_SOLOCK,PSF_SOWANT);
 if (VERBOSE(OPEN)) printf("pdisksclose: done\n");
 return(0);
}

int pdisksread(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 SOFTC *sc;
 int err;
 void *uaddr;
 int ulen;

 unit = DISKUNIT(dev);
 if (VERBOSE(READ)) printf("pdisksread: unit %u\n",unit);
 sc = &psc[unit];
 if (! (sc->flags & PSF_MOPEN)) return(EIO);
 uaddr = uio->uio_iov[0].iov_base;
 ulen = uio->uio_iov[0].iov_len;
 err = physio(pdisksstrategy,0,dev,B_READ,minphys,uio);
 if (err)
  { if (VERBOSE(READ)) printf("pdisksread: physio returned %d\n",err);
    return(err);
  }
 if (VERBOSE(DATA))
  { int i;
    int l;
    unsigned char c;
    printf("pdisksread:");
    l = 16;
    if (l < ulen) l = ulen;
    do <"datadone">
     { for (i=0;i<l;i++)
	{ switch (uio->uio_segflg)
	   { case UIO_USERSPACE:
		err = copyin(((char *)uaddr)+i,&c,1);
		if (err)
		 { printf(" <err%d>\n",err);
		   break <"datadone">;
		 }
		break;
	     case UIO_SYSSPACE:
		err = ((char *)uaddr)[i];
		break;
	     default:
		panic("pdisksread: bad segflg");
		break;
	   }
	  printf(" %02x",c);
	}
       printf(" ...\n");
     } while (0);
  }
 return(0);
}

int pdiskswrite(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 SOFTC *sc;

 unit = DISKUNIT(dev);
 if (VERBOSE(WRITE)) printf("pdiskswrite: unit %u\n",unit);
 sc = &psc[unit];
 if (! (sc->flags & PSF_MOPEN)) return(EIO);
 return(physio(pdisksstrategy,0,dev,B_WRITE,minphys,uio));
}

int pdisksioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 int err;

 unit = DISKUNIT(dev);
 if (unit >= NPDISK) return(ENXIO);
 sc = &psc[unit];
 switch (cmd)
  { case DIOCSDINFO:
    case DIOCWDINFO:
    case DIOCWLABEL:
       if (! (flag & FWRITE)) return(EBADF);
       break;
  }
 switch (cmd)
  { case DIOCGDINFO:
       *(struct disklabel *)data = *sc->dkdev.dk_label;
       break;
    case DIOCGPART:
       ((struct partinfo *)data)->disklab = sc->dkdev.dk_label;
       ((struct partinfo *)data)->part = &sc->dkdev.dk_label->d_partitions[DISKPART(dev)];
       break;
    case DIOCSDINFO:
    case DIOCWDINFO:
       err = pdisklock(sc,PSF_LABLOCK,PSF_LABWANT,"pdsioctl");
       if (err) return(err);
       sc->flags |= PSF_LABELLING;
       err = setdisklabel(sc->dkdev.dk_label,(struct disklabel *)data,0,sc->dkdev.dk_cpulabel);
       if (!err && (cmd == DIOCWDINFO)) err = writedisklabel(MAKEDISKDEV(major(dev),unit,RAW_PART),pdisksstrategy,sc->dkdev.dk_label,sc->dkdev.dk_cpulabel);
       sc->flags &= ~PSF_LABELLING;
       pdiskunlock(sc,PSF_LABLOCK,PSF_LABWANT);
       if (err) return(err);
       break;
    case DIOCWLABEL:
       if (*(int *)data) sc->flags |= PSF_WLABEL; else sc->flags &= ~PSF_WLABEL;
       break;
    case DIOCKLABEL:
       if (*(int *)data) sc->flags |= PSF_KEEPLABEL; else sc->flags &= ~PSF_KEEPLABEL;
       break;
    case DIOCGDEFLABEL:
       default_label(sc,(struct disklabel *)data);
       break;
    default:
       return(ENOTTY);
       break;
  }
 return(0);
}

int pdisksdump(dev_t dev __attribute__((__unused__)), daddr_t blkno __attribute__((__unused__)), caddr_t va __attribute__((__unused__)), size_t size __attribute__((__unused__)))
{
 return(ENXIO);
}

int pdiskssize(dev_t dev __attribute__((__unused__)))
{
 /* Used only for FS_SWAP, so... */
 return(-1);
}

#endif
