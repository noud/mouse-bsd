#include <net/if.h>
#include <netinet/in.h>

struct srt_rt {
  unsigned int inx;		/* index in rts array */
  struct in_addr srcmatch;	/* src addr match value */
  struct in_addr srcmask;	/* src addr mask value */
  union {
    struct ifnet *dstifp;	/* target interface (kernel) */
    char dstifn[IFNAMSIZ];	/* target interface name (userland) */
    } u;
  union {			/* dst addr on new if */
    struct sockaddr_in sin;
    struct sockaddr sa;
    } dst;
  } ;

/* Gets the number of slots in the rts array */
#define SRT_GETNRT _IOR('e',0,unsigned int)

/* Gets an rt entry, given the slot number in the inx field */
#define SRT_GETRT  _IOWR('e',1,struct srt_rt)

/* Sets an rt entry; inx must be either in the array or one past it */
#define SRT_SETRT  _IOW('e',2,struct srt_rt)

/* Delete an rt entry by index; shifts the rest down */
#define SRT_DELRT  _IOW('e',3,unsigned int)

/* Set flag bits */
#define SRT_SFLAGS _IOW('e',4,unsigned int)

/* Get flag bits */
#define SRT_GFLAGS _IOR('e',5,unsigned int)

/* Atomically replace flag bits */
#define SRT_SGFLAGS _IOWR('e',6,unsigned int)

/* Do debugging tasks (not documented here - see the source) */
#define SRT_DEBUG _IOW('e',7,void *)

/* Flag bits for SRT_*FLAGS */
#define SSF_MTULOCK 0x00000001 /* don't auto-update MTU */
/* Some flags are global; some are per-unit. */
#define SSF_GLOBAL (0)

#ifdef _KERNEL

#include <net/if.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/types.h>

struct srt_softc {
  struct ifnet ifnet;
  unsigned int nrt;
  struct srt_rt **rts;
  unsigned int flags;
  } ;

#define SSF_UCHG (SSF_MTULOCK)

extern void srtattach(int);
extern int srtopen(dev_t, int, int, struct proc *);
extern int srtclose(dev_t, int, int, struct proc *);
extern int srtioctl(dev_t, u_long, caddr_t, int, struct proc *);

#endif
