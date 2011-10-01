#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_seek(RQARGS)
{
 struct pfs_seek_req q;
 struct pfs_seek_rep p;

 SIZECHECK(sizeof(q),"SEEK");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: seek: id %d, from %qd to %qd: ",__progname,q.id,(long long int)q.oldoff,(long long int)q.newoff);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0 ... NWBASE+NWFILES-1:
       if (q.newoff < 0)
	{ fprintf(stderr,"EINVAL\n");
	  p.err = EINVAL;
	}
       else
	{ fprintf(stderr,"OK\n");
	  p.err = 0;
	}
       break;
    default:
       fprintf(stderr,"ESTALE\n");
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
