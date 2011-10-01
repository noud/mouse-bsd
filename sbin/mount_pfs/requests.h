#ifndef _REQUESTS_H_346c9513_
#define _REQUESTS_H_346c9513_

#include <miscfs/pfs/pfs.h>

struct reqtbl {
  unsigned int op;
  void (*handler)(struct pfs_req *, int, const void *, int, int);
  } ;

extern struct reqtbl reqtbl[];
extern int reqtbl_n;

#define RQARGS	struct pfs_req *req __attribute__((__unused__)),	\
		int fd __attribute__((__unused__)),			\
		const void *data __attribute__((__unused__)),		\
		int datalen __attribute__((__unused__)),		\
		int auxsock __attribute__((__unused__))

#define SIZECHECK(nwant,rqname) do\
	{ if (datalen != (nwant))					\
	   { protofail("%s size wrong (wanted %d, got %d)",		\
					(rqname),(nwant),datalen);	\
	   }								\
	} while (0)

extern void protofail(const char *, ...)
	__attribute__((__format__(__printf__,1,2),__noreturn__));
extern void sendreply(int, unsigned int, void *, int);
extern void maybefail(void);

#include "request-list.h"

#endif
