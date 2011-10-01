#include <stdio.h>
#include <errno.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_symlink(RQARGS)
{
 struct pfs_symlink_req q;
 struct pfs_symlink_rep p;
 int namelen;
 const char *name;
 int ltlen;
 const char *lt;
 int i;

 if (datalen <= sizeof(q)) protofail("SYMLINK size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = q.name_len;
 i = datalen - sizeof(q);
 if ((namelen < 0) || (namelen > i))
  { fprintf(stderr,"%s: protocol error, name_len %d but space %d\n",__progname,namelen,i);
    exit(1);
  }
 ltlen = i - namelen;
 name = sizeof(q) + (const char *)data;
 lt = name + namelen;
 fprintf(stderr,"%s: symlink: %.*s in %d -> %.*s: ",__progname,namelen,name,q.dir,ltlen,lt);
 maybefail();
 /* we do not permit symlink anywhere */
 p.err = EOPNOTSUPP;
 fprintf(stderr,"EOPNOTSUPP\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
