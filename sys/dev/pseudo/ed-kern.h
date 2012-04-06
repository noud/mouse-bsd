#ifndef _ED_KERN_H_07b3ca65_
#define _ED_KERN_H_07b3ca65_

/* ed, glue to rest of kernel */

#include <sys/types.h>
#include <sys/device.h>

extern struct cfattach ed_ca;
extern int edctlopen(dev_t, int, int, struct proc *);
extern int edctlclose(dev_t, int, int, struct proc *);
extern int edctlread(dev_t, struct uio *, int);
extern int edctlwrite(dev_t, struct uio *, int);
extern int edctlioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int edctlpoll(dev_t, int, struct proc *);
extern void edstrategy(struct buf *);
extern int edopen(dev_t, int, int, struct proc *);
extern int edclose(dev_t, int, int, struct proc *);
extern int edread(dev_t, struct uio *, int);
extern int edwrite(dev_t, struct uio *, int);
extern int edioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int eddump(dev_t, daddr_t, caddr_t, size_t);
extern int edsize(dev_t);

#endif
