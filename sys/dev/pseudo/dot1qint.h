#include <net/if.h>
#include <sys/mbuf.h>

extern void dot1q_input(struct ifnet *, struct mbuf *);
