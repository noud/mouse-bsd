#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_rmdir(RQARGS)
{
 struct pfs_rmdir_req q;
 struct pfs_rmdir_rep p;
 int namelen;
 const char *name;

 if (datalen <= sizeof(q)) protofail("RMDIR size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: rmdir: dir %d, obj %d, name %.*s: ",__progname,q.dir,q.obj,namelen,name);
 maybefail();
 /* we do not permit rmdir anywhere */
 p.err = EACCES;
 fprintf(stderr,"EACCES\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
