#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

static void rq_write_nosock(struct pfs_req *req, int fd, const struct pfs_write1_req *q, const void *data, int dlen)
{
 struct pfs_write_rep p;

 fprintf(stderr,"%s: write (no socket): id %d, flags %#x, %qd@%qd: ",__progname,q->id,q->flags,(long long int)dlen,(long long int)q->off);
 maybefail();
 if (q->off < 0)
  { p.err = EINVAL;
    fprintf(stderr,"EINVAL\n");
  }
 else
  { switch (q->id)
     { case NWBASE ... NWBASE+NWFILES-1:
	   { FSTATE *f;
	     f = &filestate[q->id-NWBASE];
	     if (f->flags & FSF_PRESENT)
	      { if (q->off > f->maxsize)
		 { p.err = 0;
		   p.len = 0;
		   fprintf(stderr,"0 (offset>maxsize)\n");
		   break;
		 }
		if (dlen > f->maxsize-q->off) dlen = f->maxsize - q->off;
		if (q->off+dlen > f->cursize)
		 { int newsize;
		   newsize = q->off + dlen;
		   f->contents = realloc(f->contents,newsize);
		   bzero(f->contents+f->cursize,newsize-f->cursize);
		   f->cursize = newsize;
		 }
		bcopy(data,f->contents+q->off,dlen);
		p.err = 0;
		p.len = dlen;
		fprintf(stderr,"%d (OK)\n",dlen);
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
  }
 sendreply(fd,req->id,&p,sizeof(p));
}

static void rq_write_socket(const struct pfs_write2_req *q, int auxsock)
{
 struct pfs_write_rep p;

 fprintf(stderr,"%s: write (with socket): id %d, flags %#x, %qd@%qd: ",__progname,q->id,q->flags,(long long int)q->len,(long long int)q->off);
 maybefail();
 if (q->off < 0)
  { p.err = EINVAL;
    fprintf(stderr,"EINVAL\n");
  }
 else
  { switch (q->id)
     { case NWBASE ... NWBASE+NWFILES-1:
	   { FSTATE *f;
	     f = &filestate[q->id-NWBASE];
	     if (f->flags & FSF_PRESENT)
	      { char *cp;
		int left;
		int dlen;
		if (q->off > f->maxsize)
		 { p.err = 0;
		   p.len = 0;
		   fprintf(stderr,"0 (offset>maxsize)\n");
		   break;
		 }
		dlen = f->maxsize - q->off;
		if (dlen > q->len) dlen = q->len;
		if (q->off+dlen > f->cursize)
		 { int newsize;
		   newsize = q->off + dlen;
		   f->contents = realloc(f->contents,newsize);
		   bzero(f->contents+f->cursize,newsize-f->cursize);
		   f->cursize = newsize;
		 }
		cp = f->contents + q->off;
		left = dlen;
		while (left > 0)
		 { int r;
		   r = read(auxsock,cp,left);
		   if (r < 0)
		    { p.err = EIO;
		      fprintf(stderr,"EIO (data read error: %s)\n",strerror(errno));
		      break;
		    }
		   if (r == 0)
		    { p.err = EIO;
		      fprintf(stderr,"EIO (data read EOF)\n");
		      break;
		    }
		   cp += r;
		   left -= r;
		 }
		p.err = 0;
		p.len = dlen;
		fprintf(stderr,"%d (OK)\n",dlen);
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
  }
 shutdown(auxsock,SHUT_RD);
 write(auxsock,&p,sizeof(p));
}

void rq_write(RQARGS)
{
 if (auxsock < 0)
  { if (datalen <= sizeof(struct pfs_write1_req)) protofail("WRITE (no socket) size too small (is %d, overhead %d)\n",datalen,(int)sizeof(struct pfs_write1_req));
    rq_write_nosock(req,fd,data,sizeof(struct pfs_write1_req)+(const char *)data,datalen-sizeof(struct pfs_write1_req));
  }
 else
  { SIZECHECK(sizeof(struct pfs_write2_req),"WRITE (with socket)");
    rq_write_socket(data,auxsock);
  }
}
