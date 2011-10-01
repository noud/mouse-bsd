#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_link(RQARGS)
{
 struct pfs_link_req q;
 struct pfs_link_rep p;
 int namelen;
 const char *name;

 if (datalen <= sizeof(q)) protofail("LINK size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: link: %d into %d as %.*s: ",__progname,q.old,q.dir,namelen,name);
 maybefail();
 /* we do not permit link anywhere */
 p.err = EACCES;
 fprintf(stderr,"EACCES\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
