#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <sys/time.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_getattr(RQARGS)
{
 struct pfs_getattr_req q;
 struct pfs_getattr_rep p;
 struct timeval tv;

 SIZECHECK(sizeof(q),"GETATTR");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: getattr: id %d: ",__progname,q.id);
 maybefail();
 p.err = 0;
 p.attr.va_gen = 0;
 p.attr.va_flags = 0;
 p.attr.va_rdev = 0;
 p.attr.va_bytes = 0;
 p.attr.va_filerev = 0;
 p.attr.va_vaflags = 0;
 p.attr.va_spare = 0;
 switch (q.id)
  { case PFS_ROOT_ID:
       p.attr.va_nlink = 4;
       if (0)
	{
    case PFS_MIN_ID+0:
    case PFS_MIN_ID+5:
	  p.attr.va_nlink = 2;
	}
       p.attr.va_mode = 0555;
       if (0)
	{
    case PFS_MIN_ID+4:
	  p.attr.va_nlink = 2;
	  p.attr.va_mode = 0755;
	}
       gettimeofday(&tv,0);
       p.attr.va_type = VDIR;
       p.attr.va_uid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_uid;
       p.attr.va_gid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_gid;
       p.attr.va_fileid = q.id;
       p.attr.va_size = 512;
       p.attr.va_blocksize = 512;
       p.attr.va_atime.tv_sec = tv.tv_sec;
       p.attr.va_atime.tv_nsec = tv.tv_usec * 1000;
       p.attr.va_mtime.tv_sec = 1000000000; /* 2001-09-09 01:46:40 UTC */
       p.attr.va_mtime.tv_nsec = 0;
       p.attr.va_ctime = p.attr.va_atime;
       break;
    case PFS_MIN_ID+1:
       p.attr.va_size = 16;
       if (0)
	{
    case PFS_MIN_ID+2:
	  p.attr.va_size = 22;
	}
       if (0)
	{
    case PFS_MIN_ID+6:
    case PFS_MIN_ID+7:
	  p.attr.va_size = 12;
	}
       p.attr.va_mode = 0444;
       if (0)
	{ FSTATE *f;
    case NWBASE ... NWBASE+NWFILES-1:
	  f = &filestate[q.id-NWBASE];
	  if (! (f->flags & FSF_PRESENT))
	   { p.err = ESTALE;
	     fprintf(stderr,"ESTALE\n");
	     break;
	   }
	  p.attr.va_size = f->cursize;
	  p.attr.va_mode = f->mode;
	}
       gettimeofday(&tv,0);
       p.attr.va_type = VREG;
       p.attr.va_nlink = 1;
       p.attr.va_uid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_uid;
       p.attr.va_gid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_gid;
       p.attr.va_fileid = q.id;
       p.attr.va_blocksize = 512;
       p.attr.va_atime.tv_sec = tv.tv_sec;
       p.attr.va_atime.tv_nsec = tv.tv_usec * 1000;
       p.attr.va_mtime.tv_sec = 1000000000; /* 2001-09-09 01:46:40 UTC */
       p.attr.va_mtime.tv_nsec = 0;
       p.attr.va_ctime = p.attr.va_atime;
       break;
    case PFS_MIN_ID+3:
       p.attr.va_size = 8;
       gettimeofday(&tv,0);
       p.attr.va_type = VLNK;
       p.attr.va_mode = 0555;
       p.attr.va_nlink = 1;
       p.attr.va_uid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_uid;
       p.attr.va_gid = (q.cred.flags & PFS_NOCRED) ? 0 : q.cred.cred.cr_gid;
       p.attr.va_fileid = q.id;
       p.attr.va_blocksize = 512;
       p.attr.va_atime.tv_sec = tv.tv_sec;
       p.attr.va_atime.tv_nsec = tv.tv_usec * 1000;
       p.attr.va_mtime.tv_sec = 1000000000; /* 2001-09-09 01:46:40 UTC */
       p.attr.va_mtime.tv_nsec = 0;
       p.attr.va_ctime = p.attr.va_atime;
       break;
    default:
       p.err = ESTALE;
       fprintf(stderr,"ESTALE\n");
       break;
  }
 if (! p.err) fprintf(stderr,"OK\n");
 sendreply(fd,req->id,&p,sizeof(p));
}
