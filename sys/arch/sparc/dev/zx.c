/*
 * SUNW,leo driver.
 */

#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <machine/fbio.h>

#include <machine/zxintf.h>
#include <sparc/dev/zxvar.h>

#define ZX_OFF_BOOTPROM   0x00000000
#define ZX_OFF_LC_SS0_KRN 0x00200000
#define ZX_OFF_LC_SS0_USR 0x00201000
#define ZX_OFF_LD_SS0     0x00400000
#define ZX_OFF_LD_GBL     0x00401000
#define ZX_OFF_LX_CROSS   0x00600000
#define ZX_OFF_LX_CURSOR  0x00601000
#define ZX_OFF_LX_VCTR    0x00602000
#define ZX_OFF_SS0        0x00800000
#define ZX_OFF_LC_SS1_KRN 0x01200000
#define ZX_OFF_LC_SS1_USR 0x01201000
#define ZX_OFF_LD_SS1     0x01400000
#define ZX_OFF_SS1        0x01800000

cdev_decl(zx);
static void zx_unblank(struct device *);

extern struct cfdriver zx_cd;

static struct fbdriver zx_fbdriver
 = { zx_unblank, zxopen, zxclose, zxioctl, zxpoll, zxmmap };

static void print_chipcode(unsigned int ccv)
{
 if (((ccv >> ZX_CC_MBO_SHIFT) & ZX_CC_MBO_MASK) != ZX_CC_MBO_MASK)
  { printf("?chipcode %08x",ccv);
  }
 else
  { printf("ver %d device ID %04x mfr %03x",
	(ccv >> ZX_CC_VERS_SHIFT) & ZX_CC_VERS_MASK,
	(ccv >> ZX_CC_DEVID_SHIFT) & ZX_CC_DEVID_MASK,
	(ccv >> ZX_CC_JEDMFGR_SHIFT) & ZX_CC_JEDMFGR_MASK );
  }
}

void zxattach(struct zx_softc *sc, const char *name, int isconsole, int isfb)
{
 struct fbdevice *fb;
 int i;

 fb = &sc->fbdev;
 fb->fb_driver = &zx_fbdriver;
 printf(": %s, %d x %d",name,fb->fb_type.fb_width,fb->fb_type.fb_height);
 /* init cmap */
 /* enable video */
 if (isconsole)
  { printf(" (console)");
#ifdef RASTERCONSOLE
    /* init rasterconsole
    fbrcons_init(&sc->fbdev);
    sc->fbdev.fb_rinfo.ri_hw = sc;
    sc->fbdev.fb_rinfo.ri_ops.copyrows = cg6_ras_copyrows;
    sc->fbdev.fb_rinfo.ri_ops.copycols = cg6_ras_copycols;
    sc->fbdev.fb_rinfo.ri_ops.erasecols = cg6_ras_erasecols;
    sc->fbdev.fb_rinfo.ri_ops.eraserows = cg6_ras_eraserows;
    sc->fbdev.fb_rinfo.ri_do_cursor = cg6_ras_do_cursor;
    */
#endif
  }
 printf("\n");
 printf("%s: LeoCommand: ",&sc->dev.dv_xname[0]);
 print_chipcode(sc->cmd0->chipcode);
 printf("\n");
 for (i=0;i<5;i++)
  { printf("%s: LeoDraw %d: ",&sc->dev.dv_xname[0],i);
    print_chipcode(sc->draw0[i]->chipcode);
    printf("\n");
  }
 printf("%s: LeoCross: ",&sc->dev.dv_xname[0]);
 sc->cross->lxaddr = ZXX_XADDR_CHIPCODE;
 print_chipcode(sc->cross->lxctl);
 printf("\n");
 if (isfb) fb_attach(&sc->fbdev,isconsole);
}

int zxopen(dev_t dev, int flags, int mode, struct proc *p)
{
 int unit;

 unit = minor(dev);
 if ((unit >= zx_cd.cd_ndevs) || (zx_cd.cd_devs[unit] == 0)) return(ENXIO);
 return(0);
}

int zxclose(dev_t dev, int flags, int mode, struct proc *p)
{
/*
 struct cgsix_softc *sc = cgsix_cd.cd_devs[minor(dev)];
	cg6_reset(sc);
*/
 return(0);
}

int zxioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct proc *p)
{
 struct zx_softc *sc;

 sc = zx_cd.cd_devs[minor(dev)];
 switch (cmd)
  { case FBIOGTYPE:
       *(struct fbtype *)data = sc->fbdev.fb_type;
       break;
    case FBIOGATTR:
#define fba ((struct fbgattr *)data)
       fba->real_type = sc->fbdev.fb_type.fb_type;
       fba->owner = 0;		/* XXX ??? */
       fba->fbtype = sc->fbdev.fb_type;
       fba->sattr.flags = 0;
       fba->sattr.emu_type = sc->fbdev.fb_type.fb_type;
       fba->sattr.dev_specific[0] = -1;
       fba->emu_types[0] = sc->fbdev.fb_type.fb_type;
       fba->emu_types[1] = -1;
#undef fba
       break;
    case FBIOGETCMAP:
       break;
    case FBIOPUTCMAP:
       break;
    case FBIOGVIDEO:
       *(int *)data = 0/*sc->sc_blanked*/;
       break;
    case FBIOSVIDEO:
       /*
       if (*(int *)data)
			cg6_unblank(&sc->sc_dev);
		else if (!sc->sc_blanked) {
			sc->sc_blanked = 1;
			sc->sc_thc->thc_misc &= ~THC_MISC_VIDEN;
		}
       */
       break;
/* these are for both FBIOSCURSOR and FBIOGCURSOR */
#define p ((struct fbcursor *)data)
#define cc (&sc->sc_cursor)
    case FBIOGCURSOR:
       break;
    case FBIOSCURSOR:
       break;
#undef p
#undef cc
    case FBIOGCURPOS:
       break;
    case FBIOSCURPOS:
       break;
    case FBIOGCURMAX:
       break;
    default:
#ifdef DEBUG
       log(LOG_NOTICE,"zxioctl(0x%lx) (%s[%d])\n",cmd,p->p_comm,p->p_pid);
#endif
       return(ENOTTY);
  }
 return(0);
}

int zxpoll(dev_t dev, int events, struct proc *p)
{
 return(seltrue(dev,events,p));
}

static void zx_unblank(struct device *dev)
{
/*
 struct zx_softc *sc;

 sc = (void *) dev;
*/
}

struct mmo {
  unsigned int mo_uaddr;	/* user (virtual) address */
  unsigned int mo_size;		/* size, or 0 for video ram size */
  unsigned int mo_physoff;	/* offset from sc_physadr */
  } ;

int zxmmap(dev_t dev, int off, int prot)
{
 struct zx_softc *sc;
 int mox;
 struct mmo *mo;
 unsigned int u;
 static struct mmo mmo[]
  = { { ZX_FB0_VOFF,        0x00800000, ZX_OFF_SS0        },
      { ZX_LC0_VOFF,        0x00001000, ZX_OFF_LC_SS0_USR },
      { ZX_LD0_VOFF,        0x00001000, ZX_OFF_LD_SS0     },
      { ZX_LX0_CURSOR_VOFF, 0x00001000, ZX_OFF_LX_CURSOR  },
      { ZX_FB1_VOFF,        0x00800000, ZX_OFF_SS1        },
      { ZX_LC1_VOFF,        0x00001000, ZX_OFF_LC_SS1_USR },
      { ZX_LD1_VOFF,        0x00001000, ZX_OFF_LD_SS1     },
      { ZX_LX_KRN_VOFF,     0x00001000, ZX_OFF_LX_CROSS   },
      { ZX_LC0_KRN_VOFF,    0x00001000, ZX_OFF_LC_SS0_KRN },
      { ZX_LC1_KRN_VOFF,    0x00001000, ZX_OFF_LC_SS1_KRN },
      { ZX_LD_GBL_VOFF,     0x00001000, ZX_OFF_LD_GBL     },
      { ZX_CMD0_VOFF,       0x00002000, ZX_OFF_LC_SS0_KRN },
      { ZX_CMD1_VOFF,       0x00002000, ZX_OFF_LC_SS1_KRN },
      { ZX_DRAW0_VOFF,      0x00002000, ZX_OFF_LD_SS0     },
      { ZX_DRAW1_VOFF,      0x00001000, ZX_OFF_LD_SS1     },
      { ZX_CROSS_VOFF,      0x00003000, ZX_OFF_LX_CROSS   } };
#define NMMO (sizeof mmo / sizeof *mmo)

 sc = zx_cd.cd_devs[minor(dev)];
 if (off & PGOFSET) panic("zxmmap");
 for (mox=0;mox<NMMO;mox++)
  { mo = &mmo[mox];
    if ((unsigned int)off < mo->mo_uaddr) continue;
    u = off - mo->mo_uaddr;
    if (u < mo->mo_size)
     { bus_space_handle_t bh;
       if (bus_space_mmap( sc->bustag, sc->bustype,
			  sc->paddr+u+mo->mo_physoff,
			  BUS_SPACE_MAP_LINEAR, &bh)) return(-1);
       return((int)bh);
     }
  }
#ifdef DEBUG
  { struct proc *p = curproc;
    log(LOG_NOTICE,"zxmmap(0x%x) (%s[%d])\n",off,p->p_comm,p->p_pid);
  }
#endif
 return(-1);
}
