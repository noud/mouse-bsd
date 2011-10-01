#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_remove(RQARGS)
{
 struct pfs_remove_req q;
 struct pfs_remove_rep p;
 int namelen;
 const char *name;
 int i;
 FSTATE *f;

 if (datalen <= sizeof(q)) protofail("REMOVE size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: remove: id %d, name %.*s, from %d: ",__progname,q.obj,namelen,name,q.dir);
 maybefail();
 switch (q.dir)
  { case PFS_MIN_ID+4:
       i = q.obj - NWBASE;
       if ((i < 0) || (i >= NWFILES))
	{ p.err = ESTALE;
	  break;
	}
       f = &filestate[i];
       if (! (f->flags & FSF_PRESENT))
	{ p.err = ESTALE;
	  break;
	}
       if ((f->namelen == namelen) && !bcmp(f->name,name,namelen))
	{ p.err = 0;
	  free(f->name);
	  free(f->contents);
	  f->flags &= ~FSF_PRESENT;
	}
       else
	{ p.err = ESTALE;
	  fprintf(stderr,"(%d exists but has name %.*s) ",q.obj,f->namelen,f->name);
	}
       break;
    default:
       p.err = EACCES;
       break;
  }
 switch (p.err)
  { case 0:
       fprintf(stderr,"OK\n");
       break;
    case ESTALE:
       fprintf(stderr,"ESTALE\n");
       break;
    case EACCES:
       fprintf(stderr,"EACCES\n");
       break;
    default:
       fprintf(stderr,"impossible error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
