#include <netinet/in.h>

struct encap_rt {
  unsigned int inx;		/* index in rts array */
  struct in_addr srcmatch;	/* src addr match value */
  struct in_addr srcmask;	/* src addr mask value */
  struct in_addr src;		/* src addr for encapsulated packets */
  struct in_addr dst;		/* dst addr for encapsulated packets */
  struct in_addr curdst;	/* current effective dst value */
  unsigned char id;		/* tunnel id */
  unsigned char ttl;		/* ttl to send with (set 0 -> default) */
  char *secret;			/* secret: the bytes... */
  unsigned int secretlen;	/* ...and the length */
  } ;

struct encap_listentry {
  unsigned int net;
  unsigned int mask;
  } ;

struct encap_listio {
  int oldn;
  struct encap_listentry *oldlist;
  int newn;
  struct encap_listentry *newlist;
  } ;

/* Gets the number of slots in the rts array */
#define ENCAP_GETNRT _IOR('e',0,unsigned int)

/* Gets an rt entry, given the slot number in the inx field */
#define ENCAP_GETRT  _IOWR('e',1,struct encap_rt)

/* Sets an rt entry; inx must be either in the array or one past it */
#define ENCAP_SETRT  _IOW('e',2,struct encap_rt)

/* Delete an rt entry by index; shifts the rest down */
#define ENCAP_DELRT  _IOW('e',3,unsigned int)

/* Send an empty packet down a route (used to provoke address learning) */
#define ENCAP_SENDTO _IOW('e',4,unsigned int)

/* Send an magic "reboot" packet down a route */
#define ENCAP_REBOOT _IOW('e',5,unsigned int)

/* Set flag bits */
#define ENCAP_SFLAGS _IOW('e',6,unsigned int)

/* Get flag bits */
#define ENCAP_GFLAGS _IOR('e',7,unsigned int)

/* Atomically replace flag bits */
#define ENCAP_SGFLAGS _IOWR('e',8,unsigned int)

/* Set/get blocking list */
#define ENCAP_BLOCK _IOWR('e',9,struct encap_listio)

/* Do debugging tasks (not documented here - see the source) */
#define ENCAP_DEBUG _IOW('e',10,void *)

/* Set/get poison list */
#define ENCAP_POISON _IOWR('e',11,struct encap_listio)

/* Get poisoned list */
#define ENCAP_POISONED _IOWR('e',12,struct encap_listio)

/* Flag bits for ENCAP_*FLAGS */
#define ESF_D_SIG 0x00000001 /* debug signature generation/checking */
#define ESF_D_PKT 0x00000002 /* debug packet reception/routing */
/* Some flags are global; some are per-unit. */
#define ESF_GLOBAL (ESF_D_SIG|ESF_D_PKT)

#ifdef _KERNEL

#include <net/if.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/types.h>

struct encap_softc;

struct encap_list {
  struct encap_softc *sc;
  int len;
  struct encap_listentry *list;
  } ;

struct encap_dlistentry {
  unsigned int net;
  unsigned int mask;
  unsigned int stamp;
  void *poison;
  } ;

struct encap_dlist {
  struct encap_softc *sc;
  int len;
  int alloc;
  struct encap_dlistentry *list;
  int flags;
#define EDLF_LOCK 0x00000001 /* locked for write (can still read) */
  } ;

struct encap_softc {
  struct ifnet ifnet;
  unsigned int nrt;
  struct encap_rt **rts;
  unsigned int flags; /* see ESF_D_* defines above */
#ifdef STONE_ENCAP_HACKS
  struct encap_list block;
  struct encap_list poison;
  struct encap_dlist poisoned;
#endif
  } ;

struct encap_poison {
  struct encap_softc *sc;
  unsigned long int addr;
  } ;

#define ESF_UCHG (ESF_D_SIG)

extern void encap_input(struct mbuf *, ...);
extern void encapattach(int);
extern int encapopen(dev_t, int, int, struct proc *);
extern int encapclose(dev_t, int, int, struct proc *);
extern int encapioctl(dev_t, u_long, caddr_t, int, struct proc *);

#endif
