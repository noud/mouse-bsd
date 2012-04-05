#include <sys/systm.h>
#include <sys/device.h>
#include <dev/pseudo/pseudo-kern.h>

static int pseudo_match(struct device *parent __attribute__((__unused__)), struct cfdata *match __attribute__((__unused__)), void *aux __attribute__((__unused__)))
{
 return(0);
}

static int pseudo_print(void *aux __attribute__((__unused__)), const char *pnp __attribute__((__unused__)))
{
 printf("\n");
 return(UNCONF);
}

static int pseudo_search(struct device *ps, struct cfdata *cf, void *aux __attribute__((__unused__)))
{
 config_attach(ps,cf,0,&pseudo_print);
 return(0);
}

static void pseudo_attach(struct device *parent, struct device *self, void *aux)
{
 printf("\n");
 config_search(&pseudo_search,self,0);
}

struct cfattach pseudo_ca = { sizeof(struct device), &pseudo_match, &pseudo_attach };

int pseudo_submatch(struct device *dev __attribute__((__unused__)), struct cfdata *cf, void *aux __attribute__((__unused__)))
{
 return(!strcmp(cf->cf_driver->cd_name,"pseudo"));
}
