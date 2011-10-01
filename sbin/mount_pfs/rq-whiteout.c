#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_whiteout(RQARGS)
{
 struct pfs_whiteout_req q;
 struct pfs_whiteout_rep p;
 int namelen;
 const char *name;

 if (datalen <= sizeof(q)) protofail("WHITEOUT size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: whiteout: ",__progname);
 switch (q.op)
  { case PFS_WHITE_TEST:   fprintf(stderr,"TEST");   break;
    case PFS_WHITE_CREATE: fprintf(stderr,"CREATE"); break;
    case PFS_WHITE_DELETE: fprintf(stderr,"DELETE"); break;
    default:
       fprintf(stderr,"(op%d)\n",q.op);
       exit(1);
       break;
  }
 fprintf(stderr,", name %.*s: ",namelen,name);
 maybefail();
 /* we do not support whiteout */
 p.err = EOPNOTSUPP;
 fprintf(stderr,"EOPNOTSUPP\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
