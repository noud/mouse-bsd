#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_rename(RQARGS)
{
 struct pfs_rename_req q;
 struct pfs_rename_rep p;
 int fnamelen;
 const char *fname;
 int tnamelen;
 const char *tname;
 int i;
 FSTATE *f;
 FSTATE *ff;

 if (datalen <= sizeof(q)) protofail("RENAME size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 fnamelen = q.from_len;
 i = datalen - sizeof(q);
 if ((fnamelen < 0) || (fnamelen > i))
  { fprintf(stderr,"%s: protocol error, rename from_len %d but space %d\n",__progname,fnamelen,i);
    exit(1);
  }
 tnamelen = i - fnamelen;
 fname = sizeof(q) + (const char *)data;
 tname = fname + fnamelen;
 fprintf(stderr,"%s: rename: from %d (%.*s in %d) to ",__progname,q.from_id,fnamelen,fname,q.from_dir);
 if (q.to_id == PFS_NO_ID)
  { fprintf(stderr,"nonexistent");
  }
 else
  { fprintf(stderr,"%d",q.to_id);
  }
 fprintf(stderr," (%.*s in %d): ",tnamelen,tname,q.to_dir);
 maybefail();
 do <"ret">
  { if ((q.from_dir != PFS_MIN_ID+4) || (q.to_dir != PFS_MIN_ID+4))
     { p.err = EACCES;
       fprintf(stderr,"(not both in wdir) ");
       break;
     }
    i = q.from_id - NWBASE;
    if ((i < 0) || (i >= NWFILES))
     { p.err = ESTALE;
       fprintf(stderr,"(from id out of range) ");
       break;
     }
    ff = &filestate[i];
    if (! (ff->flags & FSF_PRESENT))
     { p.err = ESTALE;
       fprintf(stderr,"(from id not present) ");
       break;
     }
    if ((ff->namelen != fnamelen) && bcmp(ff->name,fname,fnamelen))
     { /* What error is appropriate here? */
       p.err = EINVAL;
       fprintf(stderr,"(from name wrong: %d's name is %.*s) ",q.from_id,ff->namelen,ff->name);
       break;
     }
    if (q.to_id == PFS_NO_ID)
     { /* Supposedly nonexistent; check that it is really so. */
       for (i=0;i<NWFILES;i++)
	{ f = &filestate[i];
	  if ( (f->flags & FSF_PRESENT) &&
	       (f->namelen == tnamelen) &&
	       !bcmp(f->name,tname,tnamelen) )
	   { /* What error is appropriate here? */
	     p.err = EINVAL;
	     fprintf(stderr,"(to NO_ID but %.*s exists) ",tnamelen,tname);
	     break <"ret">;
	   }
	}
     }
    else
     { i = q.to_id - NWBASE;
       if ((i < 0) || (i >= NWFILES))
	{ p.err = ESTALE;
	  fprintf(stderr,"(to id out of range) ");
	  break;
	}
       f = &filestate[i];
       if (! (f->flags & FSF_PRESENT))
	{ p.err = ESTALE;
	  fprintf(stderr,"(to id not present) ");
	  break;
	}
       if ((f->namelen != tnamelen) && bcmp(f->name,tname,tnamelen))
	{ /* What error is appropriate here? */
	  p.err = EINVAL;
	  fprintf(stderr,"(to name wrong: %d's name is %.*s) ",q.to_id,f->namelen,f->name);
	  break;
	}
       free(f->name);
       free(f->contents);
       f->flags &= ~FSF_PRESENT;
     }
    free(ff->name);
    ff->name = malloc(tnamelen);
    ff->namelen = tnamelen;
    bcopy(tname,ff->name,tnamelen);
    p.err = 0;
  } while (0);
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
    case EINVAL:
       fprintf(stderr,"EINVAL\n");
       break;
    default:
       fprintf(stderr,"impossible error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
