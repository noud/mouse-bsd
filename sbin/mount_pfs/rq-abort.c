#include <stdio.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_abort(RQARGS)
{
 struct pfs_abort_req q;
 int namelen;
 const char *name;

 if (datalen <= sizeof(q)) protofail("ABORT size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: abort: id %d, name %.*s\n",__progname,q.dir,namelen,name);
 /* we ignore ABORTs, since we don't precompute stuff during lookup */
}
