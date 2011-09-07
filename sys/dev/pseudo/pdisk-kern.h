/* pdisk, glue to rest of kernel */

#include <sys/types.h>

extern void pdiskattach(int);
extern int pdiskmopen(dev_t, int, int, struct proc *);
extern int pdiskmclose(dev_t, int, int, struct proc *);
extern int pdiskmread(dev_t, struct uio *, int);
extern int pdiskmwrite(dev_t, struct uio *, int);
extern int pdiskmioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int pdiskmpoll(dev_t, int, struct proc *);
extern int pdisksopen(dev_t, int, int, struct proc *);
extern int pdisksclose(dev_t, int, int, struct proc *);
extern int pdisksread(dev_t, struct uio *, int);
extern int pdiskswrite(dev_t, struct uio *, int);
extern void pdisksstrategy(struct buf *);
extern int pdisksioctl(dev_t, u_long, caddr_t, int, struct proc *);
extern int pdisksdump(dev_t, daddr_t, caddr_t, size_t);
extern int pdiskssize(dev_t);
