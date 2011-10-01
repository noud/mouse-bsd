#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_close(RQARGS)
{
 struct pfs_close_req q;
 struct pfs_close_rep p;

 SIZECHECK(sizeof(q),"CLOSE");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: close: id %d, flags %#x: ",__progname,q.id,q.flags);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0 ... PFS_MIN_ID+7:
       p.err = 0;
       break;
    case NWBASE ... NWBASE+NWFILES-1:
       p.err = (filestate[q.id-NWBASE].flags & FSF_PRESENT) ? 0 : ESTALE;
       break;
    default:
       p.err = ESTALE;
       break;
  }
 switch (p.err)
  { case 0:
       fprintf(stderr,"OK\n");
       break;
    case ESTALE:
       fprintf(stderr,"ESTALE\n");
       break;
    default:
       fprintf(stderr,"impossible error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
