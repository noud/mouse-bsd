/* ptape, glue to rest of kernel */

#include <sys/types.h>

extern void ptapeattach(int);
extern int ptapemopen(dev_t, int, int, struct proc *);
extern int ptapemclose(dev_t, int, int, struct proc *);
extern int ptapemread(dev_t, struct uio *, int);
extern int ptapemwrite(dev_t, struct uio *, int);
extern int ptapemioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int ptapempoll(dev_t, int, struct proc *);
extern int ptapesopen(dev_t, int, int, struct proc *);
extern int ptapesclose(dev_t, int, int, struct proc *);
extern int ptapesread(dev_t, struct uio *, int);
extern int ptapeswrite(dev_t, struct uio *, int);
extern int ptapesioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int ptapespoll(dev_t, int, struct proc *);
