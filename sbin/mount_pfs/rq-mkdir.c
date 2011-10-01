#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_mkdir(RQARGS)
{
 struct pfs_mkdir_req q;
 struct pfs_mkdir_rep p;
 int namelen;
 const char *name;

 if (datalen <= sizeof(q)) protofail("MKDIR size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: mkdir: id %d, name %.*s: ",__progname,q.id,namelen,name);
 maybefail();
 /* we do not permit mkdir anywhere */
 p.err = EACCES;
 fprintf(stderr,"EACCES\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
