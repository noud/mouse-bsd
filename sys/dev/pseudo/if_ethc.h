#include <net/if.h>

struct ethc_config {
  int nifs;			/* number of underlying interfaces */
  char (*ifnames)[IFNAMSIZ];	/* underlying interface names */
  } ;

/* Sets an ethc's configuration */
#define ETHCIOC_SCONF _IOW('e',0,struct ethc_config)

/* Gets an ethc's configuration */
#define ETHCIOC_GCONF _IOWR('e',1,struct ethc_config)

#ifdef _KERNEL

#include <sys/proc.h>
#include <sys/types.h>
#include <net/if_ether.h>

struct ethc_softc {
  struct ethercom ec;
  int nifs;
  struct ifnet **ifs;
  } ;

extern void ethcattach(int);
extern int ethcopen(dev_t, int, int, struct proc *);
extern int ethcclose(dev_t, int, int, struct proc *);
extern int ethcioctl(dev_t, u_long, caddr_t, int, struct proc *);

#endif
