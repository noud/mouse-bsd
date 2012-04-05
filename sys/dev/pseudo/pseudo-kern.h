#ifndef _PSEUDO_KERN_H_e0621f41_
#define _PSEUDO_KERN_H_e0621f41_

#include <sys/device.h>

extern struct cfattach pseudo_ca;
extern int pseudo_submatch(struct device *, struct cfdata *, void *);

#endif
