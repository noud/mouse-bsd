#include <net/if.h>

struct vlan_config {
  int tag;			/* vlan tag number */
#define VLAN_NONE (-1)		/* `tag number' for untagged packets */
#define VLAN_OTHER (-2)		/* `tag number' for unclaimed packets */
  union {
    struct ifnet *dstifp;	/* target interface (kernel) */
    char dstifn[IFNAMSIZ];	/* target interface name (userland) */
    } u;
  } ;

/* Sets a vlan's configuration */
#define VLANIOC_SCONF _IOW('e',0,struct vlan_config)

/* Gets a vlan's configuration */
#define VLANIOC_GCONF _IOR('e',1,struct vlan_config)

#ifdef _KERNEL

#include <sys/proc.h>
#include <sys/types.h>
#include <net/if_ether.h>

struct vlan_softc {
  struct ethercom ec;
  struct vlan_config conf;
  unsigned int sub_promisc : 1;
  } ;

extern void vlanattach(int);
extern int vlanopen(dev_t, int, int, struct proc *);
extern int vlanclose(dev_t, int, int, struct proc *);
extern int vlanioctl(dev_t, u_long, caddr_t, int, struct proc *);

#endif
