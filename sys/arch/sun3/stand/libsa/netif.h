/*	$NetBSD: netif.h,v 1.4 1998/01/05 07:03:26 perry Exp $	*/


#include "iodesc.h"

struct netif {
	void *nif_devdata;
};

ssize_t		netif_get __P((struct iodesc *, void *, size_t, time_t));
ssize_t		netif_put __P((struct iodesc *, void *, size_t));

int		netif_open __P((void *));
int		netif_close __P((int));

struct iodesc	*socktodesc __P((int));
