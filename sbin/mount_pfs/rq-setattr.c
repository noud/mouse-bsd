#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_setattr(RQARGS)
{
 struct pfs_setattr_req q;
 struct pfs_setattr_rep p;
 FSTATE *f;

 SIZECHECK(sizeof(q),"SETATTR");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: setattr: id %d: ",__progname,q.id);
 maybefail();
 switch (q.id)
  { case NWBASE ... NWBASE+NWFILES-1:
       f = &filestate[q.id-NWBASE];
       if (f->flags & FSF_PRESENT)
	{ /* Ignore everything but va_mode and va_size for now. */
	  if (q.attr.va_size != VNOVAL)
	   { if (q.attr.va_size > f->maxsize)
	      { p.err = EFBIG;
		fprintf(stderr,"EFBIG\n");
		break;
	      }
	     if (q.attr.va_size > f->cursize)
	      { f->contents = realloc(f->contents,q.attr.va_size);
		bzero(f->contents+f->cursize,q.attr.va_size-f->cursize);
	      }
	     f->cursize = q.attr.va_size;
	     fprintf(stderr,"(size) ");
	   }
	  if (q.attr.va_mode != VNOVAL)
	   { f->mode = q.attr.va_mode & 0777;
	     fprintf(stderr,"(mode) ");
	   }
	  fprintf(stderr,"OK\n");
	  p.err = 0;
	  break;
	}
       /* fall through */
    default:
       p.err = EACCES;
       fprintf(stderr,"EACCES\n");
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
