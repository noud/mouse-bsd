/*
 * SUNW,leo driver, sbus attachment
 */

#include <sys/systm.h>
#include <sys/device.h>
#include <machine/fbvar.h>
#include <machine/promlib.h>

#include <sparc/dev/zxvar.h>

extern int fbnode;

static int zx_sbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
 return(!strcmp(((struct sbus_attach_args *)aux)->sa_name,"SUNW,leo"));
}

static void zx_sbus_attach(struct device *parent, struct device *self, void *aux)
{
 struct zx_softc *sc;
 struct sbus_attach_args *sa;
 struct fbdevice *fb;
 int node;
 int isconsole;
 const char *name;
 bus_space_handle_t bh;
 int i;
 extern struct tty *fbconstty;

 sc = (struct zx_softc *) self;
 sa = aux;
 fb = &sc->fbdev;
 sc->bustag = sa->sa_bustag;
 sc->bustype = (bus_type_t)sa->sa_slot;
 sc->paddr = (bus_addr_t)sa->sa_offset;
 node = sa->sa_node;
 fb->fb_type.fb_type = FBTYPE_SUNLEO;
 fb_setsize_obp(fb,32,1280,1024,node);
 fb->fb_type.fb_depth = 32;
 fb->fb_type.fb_cmsize = 256;
 fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
 fb->fb_pixels = 0;
 fb->fb_device = &sc->dev;
 fb->fb_flags = sc->dev.dv_cfdata->cf_flags & FB_USERMASK;
 fb->fb_pfour = 0;
 name = getpropstring(node,"model");
 if (! name) name = "???";
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x00200000,
		   0x00002000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map LeoCommand 0\n",self->dv_xname);
    return;
  }
 sc->cmd0 = (void *) bh;
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x01200000,
		   0x00002000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map LeoCommand 1\n",self->dv_xname);
    return;
  }
 sc->cmd1 = (void *) bh;
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x00400000,
		   0x00002000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map LeoDraw 0\n",self->dv_xname);
    return;
  }
 for (i=0;i<5;i++) sc->draw0[i] = (void *)(((char *)bh) + (i*0x200));
 sc->draw0bc = (void *)(((char *)bh) + 0xe00);
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x01400000,
		   0x00001000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map LeoDraw 1\n",self->dv_xname);
    return;
  }
 for (i=0;i<5;i++) sc->draw1[i] = (void *)(((char *)bh) + (i*0x200));
 sc->draw1bc = (void *)(((char *)bh) + 0xe00);
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x00600000,
		   0x00003000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map LeoCross\n",self->dv_xname);
    return;
  }
 sc->cross = (void *) bh;
 if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		   sa->sa_offset + 0x00800000,
		   0x00800000,
		   BUS_SPACE_MAP_LINEAR,
		   0, &bh) != 0)
  { printf("%s: cannot map pixels\n",self->dv_xname);
    return;
  }
 sc->pixels = (void *) bh;
 sbus_establish(&sc->sbdev,&sc->dev);
 isconsole = (node == fbnode) && fbconstty;
 if (isconsole)
  { /*
    int ramsize;
    ramsize = fb->fb_type.fb_height * fb->fb_linebytes;
    if (sbus_bus_map( sa->sa_bustag, sa->sa_slot,
		      sa->sa_offset + ZX_RAM_OFFSET,
		      ramsize,
		      BUS_SPACE_MAP_LINEAR,
		      0, &bh) != 0)
     { printf("%s: cannot map pixels\n",self->dv_xname);
       return;
     }
    sc->sc_fb.fb_pixels = (caddr_t)bh;
    */
  }
 zxattach(sc,name,isconsole,node==fbnode);
}

struct cfattach zx_sbus_ca
 = { sizeof(struct zx_softc), zx_sbus_match, zx_sbus_attach };
