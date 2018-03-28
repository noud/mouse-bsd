#include "ed.h"
#if NED > 0

#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/conf.h>
#include <sys/dkio.h>
#include <sys/file.h>
#include <dev/cons.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/sha1.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/vnode_if.h>
#include <sys/filedesc.h>
#include <sys/blowfish.h>
#include <sys/disklabel.h>
#include <miscfs/specfs/specdev.h>

#include <dev/pseudo/ed-intf.h>
#include <dev/pseudo/ed-kern.h>

#include "diskwatch.h"
#if NDISKWATCH > 0
#include <dev/pseudo/diskwatch-kern.h>
#endif

#define MAXKEYLEN 1024

typedef struct ed_softc SOFTC;

struct ed_softc {
  struct device d;
  unsigned int bopenmask;
  unsigned int copenmask;
  unsigned int openmask;
  unsigned int flags;
#define ESF_HAVEKEY   0x00000001
#define ESF_HAVEDEV   0x00000002
#define ESF_PROMPT    0x00000004
#define ESF_CANREAD   0x00000008
#define ESF_CANWRITE  0x00000010
#define ESF_WLABEL    0x00000020
#define ESF_KLABEL    0x00000040
#define ESF_HAVELABEL 0x00000080
#define ESF_AUTOCONF  0x00000100
  int labelling;
  char *wait;
  struct file *dev;
  BLOWFISH_KEY keydata;
  struct disk dkdev;
#if NDISKWATCH > 0
  int watchunit[MAXPARTITIONS];
#endif
  } ;

static int ned = 0;
static SOFTC **scv = 0;

static int edmatch(struct device *parent __attribute__((__unused__)), struct cfdata *cf __attribute__((__unused__)), void *aux __attribute__((__unused__)))
{
 printf("!!edmatch!!\n");
 return(1);
}

static void edattach(struct device *parent __attribute__((__unused__)), struct device *self, void *aux __attribute__((__unused__)))
{
 SOFTC *sc;

 sc = (void *) self;
 if (sc->d.dv_unit >= ned)
  { SOFTC **newv;
    int i;
    newv = malloc((sc->d.dv_unit+1)*sizeof(SOFTC *),M_DEVBUF,M_NOWAIT);
    if (! newv) panic("edattach: no memory (unit %d)",sc->d.dv_unit);
    for (i=0;i<ned;i++) newv[i] = scv[i];
    for (;i<=sc->d.dv_unit;i++) newv[i] = 0;
    if (scv) free(scv,M_DEVBUF);
    scv = newv;
    ned = sc->d.dv_unit + 1;
  }
 if (scv[sc->d.dv_unit]) panic("edattach: duplicate unit %d",sc->d.dv_unit);
 scv[sc->d.dv_unit] = sc;
 sc->bopenmask = 0;
 sc->copenmask = 0;
 sc->openmask = 0;
 sc->flags = 0;
 sc->labelling = 0;
 sc->dkdev.dk_name = &sc->d.dv_xname[0];
 disk_attach(&sc->dkdev);
#ifdef ED0_ROOT
 if (sc->d.dv_unit == 0) sc->flags |= ESF_AUTOCONF;
#endif
#if NDISKWATCH > 0
  { int i;
    for (i=0;i<MAXPARTITIONS;i++) sc->watchunit[i] = -1;
  }
#endif
}

struct cfattach ed_ca
 = { sizeof(SOFTC), &edmatch, &edattach, 0, 0 };

/* These are from kern_physio.c, which declares them non-static -
   but doesn't export them via a .h file! */
extern struct buf *getphysbuf(void);
extern void putphysbuf(struct buf *);

static void set_key(SOFTC *sc, char *kbuf, int klen)
{
 char altkbuf[56];

 if (klen < 1)
  { altkbuf[0] = 0;
    kbuf = &altkbuf[0];
    klen = 1;
  }
 if (klen > 56)
  { SHA1_CTX h;
    bcopy(kbuf,&altkbuf[0],36);
    SHA1Init(&h);
    SHA1Update(&h,kbuf,klen);
    SHA1Final(&altkbuf[36],&h);
    kbuf = &altkbuf[0];
    klen = 56;
  }
 blowfish_setkey(&sc->keydata,kbuf,klen);
 sc->flags = (sc->flags & ~ESF_PROMPT) | ESF_HAVEKEY;
}

static void xor8(const void *in1, const void *in2, void *out)
{
 int i;

 for (i=0;i<8;i++) ((unsigned char *)out)[i] =
	((const unsigned char *)in1)[i] ^ ((const unsigned char *)in2)[i];
}

static void decrypt_data(const void *from, void *to, int len, void *kd, off_t bno)
{
 int b;
 int i;
 const unsigned char *fp;
 unsigned char *tp;
 unsigned char iv[8];

 if (len % 512) panic("ed decrypt_data: bad length");
 fp = from;
 tp = to;
 for (b=len>>9;b>0;b--,bno++)
  { for (i=7;i>=0;i--) iv[i] = ((unsigned long long int)bno) >> (i*8);
    blowfish_encrypt(kd,&iv[0],&iv[0]);
    for (i=512;i>0;i-=8)
     { blowfish_decrypt(kd,fp,tp);
       xor8(&iv[0],tp,tp);
       bcopy(fp,&iv[0],8);
       fp += 8;
       tp += 8;
     }
  }
}

static void encrypt_data(const void *from, void *to, int len, void *kd, off_t bno)
{
 int b;
 int i;
 const unsigned char *fp;
 unsigned char *tp;
 unsigned char iv[8];

 if (len % 512) panic("ed encrypt_data: bad length");
 fp = from;
 tp = to;
 for (b=len>>9;b>0;b--,bno++)
  { for (i=7;i>=0;i--) iv[i] = ((unsigned long long int)bno) >> (i*8);
    blowfish_encrypt(kd,&iv[0],&iv[0]);
    for (i=512;i>0;i-=8)
     { xor8(&iv[0],fp,&iv[0]);
       blowfish_encrypt(kd,&iv[0],&iv[0]);
       bcopy(&iv[0],tp,8);
       fp += 8;
       tp += 8;
     }
  }
}

static void bs_sp_bs(void)
{
 cnputc('\b');
 cnputc(' ');
 cnputc('\b');
}

static int getstr_echo(char *cp, int size)
{
 char *lp;
 int c;
 int len;
 int echoing;

 echoing = 1;
 cnpollc(1);
 lp = cp;
 len = 0;
 while (1)
  { c = cngetc();
    switch (c)
     { case '\n':
       case '\r':
	  printf("\n");
	  *lp++ = '\0';
	  cnpollc(0);
	  return(len);
	  break;
       case '\b':
       case '\177':
	  if (len)
	   { len --;
	     lp --;
	     if (echoing) bs_sp_bs();
	   }
	  break;
       case 0x05: /* ^E */
	  if (echoing)
	   { for (c=len;c>0;c--) bs_sp_bs();
	     echoing = 0;
	   }
	  else
	   { for (c=0;c<len;c++) cnputc(cp[c]);
	     echoing = 1;
	   }
	  break;
       case 0x15: /* ^U */
       case 0x18: /* ^X */
	  if (echoing) for (c=len;c>0;c--) bs_sp_bs();
	  len = 0;
	  lp = cp;
	  break;
       default:
	  if ((len+1 >= size) || (c < 32))
	   { cnputc('\007');
	   }
	  else
	   { len ++;
	     *lp++ = c;
	     if (echoing) cnputc(c);
	   }
	  break;
     }
  }
}

#ifdef ED0_ROOT
static void maybe_autoconf_ed0(void)
{
 SOFTC *sc;
 struct disk *d;
 int maj;
 int unit;
 int part;
 int i;
 dev_t dv;
 struct file *fp;
 struct vnode *vp;
 char tmp[sizeof(ED0_ROOT)];
 extern struct pool file_pool;

 if ((ned > 0) && scv[0] && (scv[0]->flags & ESF_AUTOCONF))
  { sc = scv[0];
    i = sizeof(ED0_ROOT);
    if (i < 2) panic("ED0_ROOT (%s) too short",ED0_ROOT);
    strncpy(&tmp[0],ED0_ROOT,i-2);
    tmp[i-2] = '\0';
    d = disk_find(&tmp[0]);
    if (! d) panic("can't find ED0_ROOT disk %s",&tmp[0]);
    part = (int)(unsigned char)ED0_ROOT[i-2] - 'a';
    if ((part < 0) || (part >= MAXPARTITIONS)) panic("bad ED0_ROOT partition %c",ED0_ROOT[i-2]);
    /* This is annoying.  We want to construct a dev_t for this device -
       but there's no simple way to go from a struct disk * to the block
       major number for it.  So we search bdevsw for the strategy routine;
       this assumes no two distinct devices use the same strategy routine,
       but that's probably safe. */
    for (maj=nblkdev-1;maj>=0;maj--)
     { if (bdevsw[maj].d_strategy == d->dk_driver->d_strategy) break;
     }
    if (maj < 0) panic("can't find major for %s [%p]",ED0_ROOT,(void *)d->dk_driver->d_strategy);
    for (i-=3;(i>=0)&&(tmp[i]>='0')&&(tmp[i]<='9');i--) ;
    unit = 0;
    for (i++;tmp[i];i++) unit = (unit * 10) + tmp[i] - '0';
    dv = MAKEDISKDEV(maj,unit,part);
    i = bdevvp(dv,&vp);
    if (i) panic("error %d from bdevvp for ED0_ROOT",i);
    nfiles ++;
    fp = pool_get(&file_pool,PR_NOWAIT);
    if (fp == 0) panic("can't allocate open file struct for ED0_ROOT");
    bzero(fp,sizeof(*fp)); /* XXX */
    fp->f_count = 1;
    fp->f_cred = NOCRED;
    fp->f_usecount = 1;
    fp->f_flag = FREAD | FWRITE;
    fp->f_type = DTYPE_VNODE;
    fp->f_ops = &vnops;
    fp->f_data = vp;
    i = VOP_OPEN(vp,FREAD|FWRITE,NOCRED,0);
    if (i) panic("error %d opening ED0_ROOT (%s)",i,ED0_ROOT);
    vp->v_writecount ++;
    VOP_UNLOCK(vp,0);
    sc->flags &= ~ESF_AUTOCONF;
    sc->flags |= ESF_CANREAD | ESF_CANWRITE | ESF_HAVEDEV | ESF_PROMPT;
    sc->dev = fp;
  }
}
#else
#define maybe_autoconf_ed0() /*nothing*/
#endif

int edctlopen(dev_t dev __attribute__((__unused__)), int flag __attribute__((__unused__)), int mode __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 maybe_autoconf_ed0();
 return(0);
}

int edctlclose(dev_t dev __attribute__((__unused__)), int flag __attribute__((__unused__)), int mode __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 return(0);
}

int edctlread(dev_t dev __attribute__((__unused__)), struct uio *uio __attribute__((__unused__)), int ioflag __attribute__((__unused__)))
{
 return(0);
}

int edctlwrite(dev_t dev, struct uio *uio, int ioflag)
{
 SOFTC *sc;
 unsigned char unit;
 unsigned char cmd;
 int err;

 if (uio->uio_resid < 1) return(0);
 err = uiomove(&cmd,1,uio);
 if (err) return(err);
 switch (cmd)
  { case ED_CMD_SETKEY:
	{ int klen;
	  char kbuf[MAXKEYLEN];
	  if (uio->uio_resid < 1) return(EINVAL);
	  err = uiomove(&unit,1,uio);
	  if (err) return(err);
	  if (unit >= ned) return(ENXIO);
	  sc = scv[unit];
	  if (! sc) return(ENXIO);
	  if (uio->uio_resid > MAXKEYLEN) return(EMSGSIZE);
	  klen = uio->uio_resid;
	  err = uiomove(&kbuf[0],klen,uio);
	  if (err) return(err);
	  set_key(sc,&kbuf[0],klen);
	}
       break;
    case ED_CMD_SETPROMPT:
       if (uio->uio_resid != 1) return(EINVAL);
       err = uiomove(&unit,1,uio);
       if (err) return(err);
       if (unit >= ned) return(ENXIO);
       sc = scv[unit];
       if (! sc) return(ENXIO);
       sc->flags = (sc->flags & ~ESF_HAVEKEY) | ESF_PROMPT;
       break;
    case ED_CMD_SETDEV:
	{ int fd;
	  struct proc *p;
	  struct filedesc *fdp;
	  struct file *fp;
	  if (uio->uio_resid != 1+sizeof(int)) return(EINVAL);
	  err = uiomove(&unit,1,uio);
	  if (err) return(err);
	  if (unit >= ned) return(ENXIO);
	  sc = scv[unit];
	  if (! sc) return(ENXIO);
	  err = uiomove(&fd,sizeof(int),uio);
	  if (err) return(err);
	  p = uio->uio_procp;
	  fdp = p->p_fd;
	  if ((fd < 0) || (fd >= fdp->fd_nfiles)) return(EBADF);
	  fp = fdp->fd_ofiles[fd];
	  if (!fp || (fp->f_iflags & FIF_WANTCLOSE)) return(EBADF);
	  if (fp->f_type != DTYPE_VNODE) return(EINVAL);
	  switch (((struct vnode *)fp->f_data)->v_type)
	   { case VBLK:
		break;
	     default:
		return(ENOTBLK);
		break;
	   }
	  FILE_USE(fp);
	  sc->flags &= ~(ESF_CANREAD|ESF_CANWRITE);
	  if (fp->f_flag & FREAD) sc->flags |= ESF_CANREAD;
	  if (fp->f_flag & FWRITE) sc->flags |= ESF_CANWRITE;
	  if (sc->flags & ESF_HAVEDEV) closef(sc->dev,0);
	  fp->f_count ++;
	  sc->dev = fp;
	  fdrelease(p,fd);
	  sc->flags |= ESF_HAVEDEV;
	}
       break;
    case ED_CMD_RESET:
       if (uio->uio_resid != 1) return(EINVAL);
       err = uiomove(&unit,1,uio);
       if (err) return(err);
       if (unit >= ned) return(ENXIO);
       sc = scv[unit];
       if (! sc) return(ENXIO);
       if (sc->flags & ESF_HAVEDEV) closef(sc->dev,0);
       sc->flags = 0;
       break;
    default:
       return(EINVAL);
       break;
  }
 return(0);
}

int edctlioctl(dev_t dev __attribute__((__unused__)), u_long cmd __attribute__((__unused__)), caddr_t data __attribute__((__unused__)), int flag __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 return(ENOTTY);
}

int edctlpoll(dev_t dev __attribute__((__unused__)), int events, struct proc *p __attribute__((__unused__)))
{
 return(events);
}

static int getsize(SOFTC *sc)
{
 int err;
 struct partinfo pi;

 if ((sc->flags & (ESF_HAVEKEY|ESF_HAVEDEV)) != (ESF_HAVEKEY|ESF_HAVEDEV)) panic("ed: impossible getsize");
 err = VOP_IOCTL((struct vnode *)sc->dev->f_data,DIOCGPART,(void *)&pi,sc->dev->f_flag,NOCRED,curproc);
 if (err) return(-1);
 return(pi.part->p_size);
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
 strncpy(&label->d_typename[0],"ed",sizeof(label->d_typename));
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

void edstrategy(struct buf *bp)
{
 __label__ ret;

 unsigned int unit;
 unsigned int part;
 SOFTC *sc;
 struct disklabel *label;
 struct buf *subb;
 char *data;
 off_t o;
 int err;

 /* This sucks.  Nested functions generate calls to abort(), which doesn't
    exist in the kernel, so we can't use a nested function here.  So instead
    we make it a macro. :-þ */
#define error(e) do { bp->b_error = (e); bp->b_flags |= B_ERROR; \
    biodone(bp); goto ret; } while (0)

 unit = DISKUNIT(bp->b_dev);
 if (unit >= ned) error(ENXIO);
 sc = scv[unit];
 if (! sc) error(ENXIO);
 data = 0;
 subb = 0;
 if ((sc->flags & (ESF_HAVEKEY|ESF_HAVEDEV)) != (ESF_HAVEKEY|ESF_HAVEDEV)) error(EIO);
 switch (bp->b_flags & (B_READ|B_WRITE))
  { case B_READ:
       if (! (sc->flags & ESF_CANREAD)) error(EIO);
       break;
    case B_WRITE:
       if (! (sc->flags & ESF_CANWRITE)) error(EIO);
       break;
    default:
       panic("edstrategy READ|WRITE 1");
       break;
  }
 if (bp->b_bcount == 0)
  { biodone(bp);
    return;
  }
 label = sc->dkdev.dk_label;
 if ( (label->d_secsize % DEV_BSIZE) ||
      (bp->b_bcount % label->d_secsize) ) error(EINVAL);
 part = DISKPART(bp->b_dev);
 if ( (part != RAW_PART) &&
      (bounds_check_with_label(bp,label,(sc->flags&ESF_WLABEL)|sc->labelling) <= 0) )
  { biodone(bp);
    return;
  }
 data = malloc(bp->b_bcount,M_DEVBUF,M_NOWAIT);
 if (data == 0) error(ENOBUFS);
 if (bp->b_bcount % DEV_BSIZE) panic("edstrategy unaligned size");
 o = bp->b_blkno / (label->d_secsize / DEV_BSIZE);
 o += label->d_partitions[part].p_offset;
 o *= label->d_secsize;
#if NDISKWATCH > 0
 if (sc->watchunit[part] >= 0) diskwatch_watch(sc->watchunit[part],bp);
#endif
 switch (bp->b_flags & (B_READ|B_WRITE))
  { case B_READ:
       subb = getphysbuf();
       subb->b_flags = B_BUSY | B_PHYS | B_RAW | B_READ;
       switch (((struct vnode *)sc->dev->f_data)->v_type)
	{ case VBLK:
	     break;
	  default:
	     panic("edstrategy: impossible backing v_type");
	     break;
	}
       subb->b_dev = ((struct vnode *)sc->dev->f_data)->v_rdev;
       subb->b_error = 0;
       subb->b_proc = bp->b_proc;
       subb->b_blkno = btodb(o);
       subb->b_bcount = bp->b_bcount;
       subb->b_data = data;
       (*bdevsw[major(subb->b_dev)].d_strategy)(subb);
       err = biowait(subb);
       if (err) error(err);
       decrypt_data(data,bp->b_data,bp->b_bcount,&sc->keydata,o>>9);
       bp->b_resid = subb->b_resid;
       break;
    case B_WRITE:
       encrypt_data(bp->b_data,data,bp->b_bcount,&sc->keydata,o>>9);
       subb = getphysbuf();
       subb->b_flags = B_BUSY | B_PHYS | B_RAW | B_WRITE;
       switch (((struct vnode *)sc->dev->f_data)->v_type)
	{ case VBLK:
	     break;
	  default:
	     panic("edstrategy: impossible backing v_type");
	     break;
	}
       subb->b_dev = ((struct vnode *)sc->dev->f_data)->v_rdev;
       subb->b_error = 0;
       subb->b_proc = bp->b_proc;
       subb->b_blkno = btodb(o);
       subb->b_bcount = bp->b_bcount;
       subb->b_data = data;
       (*bdevsw[major(subb->b_dev)].d_strategy)(subb);
       err = biowait(subb);
       if (err) error(err);
       bp->b_resid = subb->b_resid;
       break;
    default:
       panic("edstrategy READ|WRITE 2");
       break;
  }
 biodone(bp);
ret:;
 if (subb) putphysbuf(subb);
 if (data) free(data,M_DEVBUF);
#undef error
}

static void load_label(dev_t dev, SOFTC *sc)
{
 const char *why;

 if (sc->flags & ESF_HAVELABEL) return;
 sc->flags |= ESF_HAVELABEL;
 /* XXX readdisklabel assumes the label is already partially set up! */
 default_label(sc,sc->dkdev.dk_label);
 why = readdisklabel(MAKEDISKDEV(major(dev),sc->d.dv_unit,RAW_PART),edstrategy,sc->dkdev.dk_label,sc->dkdev.dk_cpulabel);
 if (why)
  { printf("ed%d: %s\n",sc->d.dv_unit,why);
    default_label(sc,sc->dkdev.dk_label);
  }
}

static int maybe_prompt(SOFTC *sc)
{
 switch (sc->flags & (ESF_HAVEKEY|ESF_PROMPT))
  { case ESF_HAVEKEY:
       break;
    case ESF_PROMPT:
	{ char keybuf[MAXKEYLEN];
	  int kl;
	  printf("Enter key for ed%d: ",sc->d.dv_unit);
	  kl = getstr_echo(&keybuf[0],MAXKEYLEN);
	  if (kl < 1) return(EIO);
	  set_key(sc,&keybuf[0],kl);
	}
       break;
    default:
       return(EIO);
       break;
  }
 return(0);
}

int edopen(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 unsigned char part;
 int err;

 maybe_autoconf_ed0();
 unit = DISKUNIT(dev);
 if (unit >= ned) return(ENXIO);
 sc = scv[unit];
 if (! sc) return(ENXIO);
 if (! (sc->flags & ESF_HAVEDEV)) return(EIO);
 if ((flag & FREAD) && !(sc->flags & ESF_CANREAD)) return(EACCES);
 if ((flag & FWRITE) && !(sc->flags & ESF_CANWRITE)) return(EACCES);
 err = maybe_prompt(sc);
 if (err) return(err);
 part = DISKPART(dev);
 if (! (sc->flags & ESF_HAVELABEL)) load_label(dev,sc);
 if ( (part != RAW_PART) &&
      ( (part >= sc->dkdev.dk_label->d_npartitions) ||
	(sc->dkdev.dk_label->d_partitions[part].p_fstype == FS_UNUSED) ) )
  { return(ENXIO);
  }
 switch (mode)
  { case S_IFCHR:
       sc->copenmask |= 1 << part;
       break;
    case S_IFBLK:
       sc->bopenmask |= 1 << part;
       break;
    default:
       printf("edopen opening unknown mode %d\n",mode);
       panic("edopen");
       break;
  }
 sc->openmask = sc->copenmask | sc->bopenmask;
 return(0);
}

int edclose(dev_t dev, int flag, int mode, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 unsigned int part;

 unit = DISKUNIT(dev);
 if (unit >= ned) return(ENXIO);
 sc = scv[unit];
 if (! sc) return(ENXIO);
 part = DISKPART(dev);
 switch (mode)
  { case S_IFCHR:
       sc->copenmask &= ~(1 << part);
       break;
    case S_IFBLK:
       sc->bopenmask &= ~(1 << part);
       break;
    default:
       printf("edclose closing unknown mode %d\n",mode);
       panic("edclose");
       break;
  }
 sc->openmask = sc->copenmask | sc->bopenmask;
 if ((sc->openmask == 0) && !(sc->flags & ESF_KLABEL)) sc->flags &= ~ESF_HAVELABEL;
 return(0);
}

int edread(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 SOFTC *sc;

 unit = DISKUNIT(dev);
 if (unit >= ned) return(ENXIO);
 sc = scv[unit];
 if (! sc) return(ENXIO);
 if ( (sc->flags & (ESF_HAVEKEY|ESF_HAVEDEV|ESF_CANREAD)) !=
      (ESF_HAVEKEY|ESF_HAVEDEV|ESF_CANREAD) ) return(EIO);
 return(physio(edstrategy,0,dev,B_READ,&minphys,uio));
}

int edwrite(dev_t dev, struct uio *uio, int ioflag)
{
 unsigned int unit;
 SOFTC *sc;

 unit = DISKUNIT(dev);
 if (unit >= ned) return(ENXIO);
 sc = scv[unit];
 if (! sc) return(ENXIO);
 if ( (sc->flags & (ESF_HAVEKEY|ESF_HAVEDEV|ESF_CANWRITE)) !=
      (ESF_HAVEKEY|ESF_HAVEDEV|ESF_CANWRITE) ) return(EIO);
 return(physio(edstrategy,0,dev,B_WRITE,&minphys,uio));
}

int edioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
 SOFTC *sc;
 unsigned int unit;
 unsigned int part;
 int err;

 unit = DISKUNIT(dev);
 part = DISKPART(dev);
 if (unit >= ned) return(ENXIO);
 sc = scv[unit];
 if (! sc) return(ENXIO);
 switch (cmd)
  { case DIOCSDINFO:
    case DIOCWDINFO:
    case DIOCWLABEL:
       if (! (flag & FWRITE)) return(EBADF);
       break;
#if NDISKWATCH > 0
    case DWIOCSET:
       if (p) return(EINVAL);
       if (sc->watchunit[part] >= 0) return(EBUSY);
       sc->watchunit[part] = *(int *)data;
       return(0);
    case DWIOCCLR:
       if (p) return(EINVAL);
       if (sc->watchunit[part] != *(int *)data) return(EBUSY);
       sc->watchunit[part] = -1;
       return(0);
#endif
  }
 switch (cmd)
  { case DIOCGDINFO:
       *(struct disklabel *)data = *sc->dkdev.dk_label;
       break;
    case DIOCGPART:
       ((struct partinfo *)data)->disklab = sc->dkdev.dk_label;
       ((struct partinfo *)data)->part = &sc->dkdev.dk_label->d_partitions[part];
       break;
    case DIOCSDINFO:
    case DIOCWDINFO:
       sc->labelling ++;
       err = setdisklabel(sc->dkdev.dk_label,(struct disklabel *)data,0,sc->dkdev.dk_cpulabel);
       if (!err && (cmd == DIOCWDINFO)) err = writedisklabel(MAKEDISKDEV(major(dev),unit,RAW_PART),&edstrategy,sc->dkdev.dk_label,sc->dkdev.dk_cpulabel);
       sc->labelling --;
       if (err) return(err);
       break;
    case DIOCWLABEL:
       if (*(int *)data) sc->flags |= ESF_WLABEL; else sc->flags &= ~ESF_WLABEL;
       break;
    case DIOCKLABEL:
       if (*(int *)data) sc->flags |= ESF_KLABEL; else sc->flags &= ~ESF_KLABEL;
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

int eddump(dev_t dev __attribute__((__unused__)), daddr_t blkno __attribute__((__unused__)), caddr_t va __attribute__((__unused__)), size_t size __attribute__((__unused__)))
{
 return(ENXIO);
}

int edsize(dev_t dev)
{
 SOFTC *sc;
 unsigned int unit;
 int part;
 int already;
 int rv;
 struct disklabel *label;

 unit = DISKUNIT(dev);
 if (unit >= ned) return(-1);
 sc = scv[unit];
 if (! sc) return(-1);
 part = DISKPART(dev);
 already = (sc->openmask >> part) & 1;
 if (!already && edopen(dev,0,S_IFBLK,0)) return(-1);
 label = sc->dkdev.dk_label;
 rv = (label->d_partitions[part].p_fstype == FS_SWAP)
      ? ( label->d_partitions[part].p_size *
	  (label->d_secsize / DEV_BSIZE) )
      : -1;
 if (!already && edclose(dev,0,S_IFBLK,0)) return(-1);
 return(rv);
}

#endif
