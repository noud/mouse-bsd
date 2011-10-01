#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <sys/vnode.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_access(RQARGS)
{
 struct pfs_access_req q;
 struct pfs_access_rep p;

 SIZECHECK(sizeof(q),"ACCESS");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: access: id %d, mode %#x: ",__progname,q.id,q.mode);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0:
    case PFS_MIN_ID+5:
       p.err = (q.mode & VWRITE) ? EACCES : 0;
       break;
    case PFS_MIN_ID+1:
    case PFS_MIN_ID+2:
    case PFS_MIN_ID+6:
    case PFS_MIN_ID+7:
       p.err = (q.mode & (VWRITE|VEXEC)) ? EACCES : 0;
       break;
    case PFS_MIN_ID+3:
       p.err = (q.mode & VWRITE) ? EACCES : 0;
       break;
    case PFS_MIN_ID+4:
       p.err = 0;
       break;
    case NWBASE ... NWBASE+NWFILES-1:
	{ FSTATE *f;
	  f = &filestate[q.id-NWBASE];
	  if (f->flags & FSF_PRESENT)
	   { p.err = 0;
	     if ((q.mode & VREAD ) && !(f->mode & 0400)) p.err = EACCES;
	     if ((q.mode & VWRITE) && !(f->mode & 0200)) p.err = EACCES;
	     if ((q.mode & VEXEC ) && !(f->mode & 0100)) p.err = EACCES;
	   }
	  else
	   { p.err = ESTALE;
	     break;
	   }
	}
       break;
    default:
       p.err = ESTALE;
       break;
  }
 switch (p.err)
  { case EISDIR:
       fprintf(stderr,"EISDIR\n");
       break;
    case EROFS:
       fprintf(stderr,"EROFS\n");
       break;
    case EACCES:
       fprintf(stderr,"EACCES\n");
       break;
    case ESTALE:
       fprintf(stderr,"ESTALE\n");
       break;
    case 0:
       fprintf(stderr,"OK\n");
       break;
    default:
       fprintf(stderr,"impossible error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
