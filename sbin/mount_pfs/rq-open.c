#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_open(RQARGS)
{
 struct pfs_open_req q;
 struct pfs_open_rep p;

 SIZECHECK(sizeof(q),"OPEN");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: open: id %d, mode %#x: ",__progname,q.id,q.mode);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0:
    case PFS_MIN_ID+4:
    case PFS_MIN_ID+5:
       if (q.mode & FWRITE)
	{ p.err = EISDIR;
	}
       else
	{ p.err = 0;
	}
       break;
    case PFS_MIN_ID+1:
    case PFS_MIN_ID+2:
    case PFS_MIN_ID+6:
    case PFS_MIN_ID+7:
       if (q.mode & FWRITE)
	{ p.err = EACCES;
	}
       else
	{ p.err = 0;
	}
       break;
    case PFS_MIN_ID+3:
       /* XXX is this right?  What does VOP_OPEN mean for a symlink? */
       p.err = 0;
       break;
    case NWBASE ... NWBASE+NWFILES-1:
	{ FSTATE *f;
	  f = &filestate[q.id-NWBASE];
	  if (f->flags & FSF_PRESENT)
	   { p.err = 0;
	     if ((q.mode & FREAD ) && !(f->mode & 0400)) p.err = EACCES;
	     if ((q.mode & FWRITE) && !(f->mode & 0200)) p.err = EACCES;
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
  { case 0:
       fprintf(stderr,"OK\n");
       break;
    case EISDIR:
       fprintf(stderr,"EISDIR\n");
       break;
    case EACCES:
       fprintf(stderr,"EACCES\n");
       break;
    case ESTALE:
       fprintf(stderr,"ESTALE\n");
       break;
    default:
       fprintf(stderr,"error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
