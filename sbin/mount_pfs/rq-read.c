#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <strings.h>
#include <sys/vnode.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_read(RQARGS)
{
 struct pfs_read_req q;
 char dbuf[PFS_REP_MAX]
	__attribute__((__aligned__(__alignof__(struct pfs_read_rep))));
#define p (*(struct pfs_read_rep *)&dbuf[0])
 const char *contents;
 int clen;

 SIZECHECK(sizeof(q),"READ");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: read: id %d, flags %#x, %qd@%qd: ",__progname,q.id,q.flags,(long long int)q.len,(long long int)q.off);
 maybefail();
 switch (q.id)
  { case PFS_ROOT_ID:
    case PFS_MIN_ID+0:
    case PFS_MIN_ID+4:
    case PFS_MIN_ID+5:
       p.err = EISDIR;
       fprintf(stderr,"EISDIR\n");
       break;
    case PFS_MIN_ID+1:
       p.err = 0;
       contents = "This is a file.\n";
       clen = 16;
       break;
    case PFS_MIN_ID+2:
       p.err = 0;
       contents = "This is another file.\n";
       clen = 22;
       break;
    case PFS_MIN_ID+3:
       p.err = EOPNOTSUPP;
       fprintf(stderr,"EOPNOTSUPP\n");
       break;
    case PFS_MIN_ID+6:
       p.err = 0;
       contents = "Sub file 1.\n";
       clen = 12;
       break;
    case PFS_MIN_ID+7:
       p.err = 0;
       contents = "Sub file 2.\n";
       clen = 12;
       break;
    case NWBASE ... NWBASE+NWFILES-1:
	{ FSTATE *f;
	  f = &filestate[q.id-(NWBASE)];
	  if (f->flags & FSF_PRESENT)
	   { p.err = 0;
	     contents = f->contents;
	     clen = f->cursize;
	   }
	  else
	   { p.err = ESTALE;
	     fprintf(stderr,"ESTALE\n");
	   }
	}
       break;
    default:
       p.err = ESTALE;
       fprintf(stderr,"ESTALE\n");
       break;
  }
 if (q.off < 0)
  { p.err = EINVAL;
    fprintf(stderr,"EINVAL\n");
  }
 p.len = 0;
 p.flags = 0;
 if (!p.err && (q.off < clen))
  { p.len = clen - q.off;
    if (p.len > q.len) p.len = q.len;
    bcopy(contents+q.off,&dbuf[sizeof(struct pfs_read_rep)],p.len);
  }
 if (!p.err && (clen == 22))
  { p.flags |= PFS_READ_P_SOCKET;
    fprintf(stderr,"len %d, flags %#x\n",p.len,p.flags);
    sendreply(fd,req->id,&p,sizeof(p));
    write(auxsock,&dbuf[sizeof(struct pfs_read_rep)],p.len);
  }
 else
  { fprintf(stderr,"len %d, flags %#x\n",p.len,p.flags);
    sendreply(fd,req->id,&p,sizeof(p)+p.len);
  }
}
