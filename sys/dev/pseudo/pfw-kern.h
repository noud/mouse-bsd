/* pfw, glue to rest of kernel */

#include <sys/types.h>

extern void pfwattach(int);
extern int pfwopen(dev_t, int, int, struct proc *);
extern int pfwclose(dev_t, int, int, struct proc *);
extern int pfwread(dev_t, struct uio *, int);
extern int pfwwrite(dev_t, struct uio *, int);
extern int pfwioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int pfwpoll(dev_t, int, struct proc *);
