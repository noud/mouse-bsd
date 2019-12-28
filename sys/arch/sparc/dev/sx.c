#include <sys/systm.h>
#include <sys/device.h>
#include <machine/param.h>

#include <machine/autoconf.h>

#include "sxvar.h"

typedef struct sx_softc SOFTC;

SOFTC *sxdev = 0;

static int sx_match(struct device *parent, struct cfdata *cf, void *aux)
{
 struct mainbus_attach_args *ma;

 ma = aux;
 if (CPU_ISSUN4OR4C) return(0);
 if (strcmp(ma->ma_name,"SUNW,sx")) return(0);
 return(1);
}

static void sx_attach(struct device *parent, struct device *self, void *aux)
{
 SOFTC *sc;
 struct mainbus_attach_args *ma;

 sc = (void *)self;
 (void)parent;
 ma = aux;
 sc->uregs = ma->ma_paddr + 0x1000;
 sc->t = ma->ma_bustag;
 sc->node = ma->ma_node;
 if (bus_space_map(sc->t,ma->ma_paddr,0x1000,0,&sc->h))
  { printf(": can't map registers\n");
    return;
  }
 printf("\n");
 sxdev = sc;
}

struct cfattach sx_ca
 = { sizeof(SOFTC), &sx_match, &sx_attach };
