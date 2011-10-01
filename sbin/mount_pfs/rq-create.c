#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_create(RQARGS)
{
 struct pfs_create_req q;
 struct pfs_create_rep p;
 int namelen;
 const char *name;
 int i;
 FSTATE *f;

 if (datalen <= sizeof(q)) protofail("CREATE size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: create: id %d, name %.*s: ",__progname,q.id,namelen,name);
 maybefail();
 switch <"idswitch"> (q.id)
  { case PFS_MIN_ID+4:
       do <"fsearch">
	{ for (i=0;i<NWFILES;i++)
	   { f = &filestate[i];
	     if (f->flags & FSF_PRESENT) continue;
	     break <"fsearch">;
	   }
	  p.err = ENOSPC;
	  fprintf(stderr,"ENOSPC\n");
	  break <"idswitch">;
	} while (0);
       f->name = malloc(namelen);
       f->namelen = namelen;
       bcopy(name,f->name,namelen);
       f->contents = 0;
       f->cursize = 0;
       f->flags = FSF_PRESENT;
       f->mode = q.attr.va_mode;
       p.err = 0;
       p.id = NWBASE + i;
       fprintf(stderr,"success, id %d\n",p.id);
       break;
    default:
       p.err = EACCES;
       fprintf(stderr,"EACCES\n");
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
