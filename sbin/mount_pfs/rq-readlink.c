#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_readlink(RQARGS)
{
 struct pfs_readlink_req q;
 char dbuf[PFS_REP_MAX]
	__attribute__((__aligned__(__alignof__(struct pfs_readlink_rep))));
#define p (*(struct pfs_readlink_rep *)&dbuf[0])
 int dlen;

 SIZECHECK(sizeof(q),"READLINK");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: readlink: id %d: ",__progname,q.id);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0 ... PFS_MIN_ID+2:
    case PFS_MIN_ID+4 ... NWBASE+NWFILES-1:
       p.err = EINVAL;
       fprintf(stderr,"EINVAL\n");
       break;
    case PFS_MIN_ID+3:
       p.err = 0;
       strcpy(&dbuf[sizeof(struct pfs_readlink_rep)],"dir/file2");
       dlen = 9;
       fprintf(stderr,"OK\n");
       break;
    default:
       p.err = ESTALE;
       fprintf(stderr,"ESTALE\n");
       break;
  }
 if (p.err) dlen = 0;
 sendreply(fd,req->id,&dbuf[0],sizeof(p)+dlen);
}
