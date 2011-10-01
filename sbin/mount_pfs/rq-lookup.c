#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

void rq_lookup(RQARGS)
{
 struct pfs_lookup_req q;
 struct pfs_lookup_rep p;
 int namelen;
 const char *name;

 static int it_exists(int reqdir)
  { switch (q.id)
     { case PFS_ROOT_ID:
	  switch (namelen)
	   { case 3:
		if (!bcmp(name,"dir",3))
		 { p.id = PFS_MIN_ID+0;
		   p.type = VDIR;
		   return(1);
		 }
		break;
	     case 4:
		switch (name[0])
		 { case 'f':
		      if (!bcmp(name,"file",4))
		       { if (reqdir)
			  { p.id = PFS_MIN_ID+5;
			    p.type = VDIR;
			  }
			 else
			  { p.id = PFS_MIN_ID+1;
			    p.type = VREG;
			  }
			 return(1);
		       }
		      break;
		   case 'l':
		      if (!bcmp(name,"link",4))
		       { p.id = PFS_MIN_ID+3;
			 p.type = VLNK;
			 return(1);
		       }
		      break;
		   case 'w':
		      if (!bcmp(name,"wdir",4))
		       { p.id = PFS_MIN_ID+4;
			 p.type = VDIR;
			 return(1);
		       }
		      break;
		 }
		break;
	   }
	  break;
       case PFS_MIN_ID+0:
	  switch (namelen)
	   { case 5:
		if (!bcmp(name,"file2",5))
		 { p.id = PFS_MIN_ID+2;
		   p.type = VREG;
		   return(1);
		 }
		break;
	   }
	  break;
       case PFS_MIN_ID+4:
	   { int i;
	     FSTATE *f;
	     for (i=0;i<NWFILES;i++)
	      { f = &filestate[i];
		if (! (f->flags & FSF_PRESENT)) continue;
		if ((namelen == f->namelen) && !bcmp(name,f->name,namelen))
		 { p.id = NWBASE + i;
		   p.type = VREG;
		   return(1);
		 }
	      }
	   }
	  break;
       case PFS_MIN_ID+5:
	  switch (namelen)
	   { case 4:
		if (!bcmp(name,"sub1",4))
		 { p.id = PFS_MIN_ID+6;
		   p.type = VREG;
		   return(1);
		 }
		if (!bcmp(name,"sub2",4))
		 { p.id = PFS_MIN_ID+7;
		   p.type = VREG;
		   return(1);
		 }
		break;
	   }
	  break;
     }
    return(0);
  }

 if (datalen <= sizeof(q)) protofail("LOOKUP size too small (is %d, overhead %d)\n",datalen,(int)sizeof(q));
 bcopy(data,&q,sizeof(q));
 namelen = datalen - sizeof(q);
 name = sizeof(q) + (const char *)data;
 fprintf(stderr,"%s: lookup: id %d, name %.*s: ",__progname,q.id,namelen,name);
 switch (q.op & PFS_LKUP_OP)
  { case PFS_LKUP_LOOKUP: fprintf(stderr,"LOOKUP: "); break;
    case PFS_LKUP_CREATE: fprintf(stderr,"CREATE: "); break;
    case PFS_LKUP_DELETE: fprintf(stderr,"DELETE: "); break;
    case PFS_LKUP_RENAME: fprintf(stderr,"RENAME: "); break;
    default: fprintf(stderr,"(op%d): ",q.op&PFS_LKUP_OP); break;
  }
 if (q.op & PFS_LKUP_REQDIR) fprintf(stderr,"(REQDIR) ");
 maybefail();
 if (it_exists(q.op&PFS_LKUP_REQDIR))
  { p.err = 0;
    switch (q.op & PFS_LKUP_OP)
     { case PFS_LKUP_DELETE:
       case PFS_LKUP_RENAME:
	  switch (q.id)
	   { case PFS_MIN_ID+4:
		break;
	     default:
		p.err = EACCES;
		break;
	   }
	  break;
     }
  }
 else
  { switch (q.op & PFS_LKUP_OP)
     { case PFS_LKUP_LOOKUP:
       case PFS_LKUP_DELETE:
	  p.err = ENOENT;
	  break;
       case PFS_LKUP_CREATE:
       case PFS_LKUP_RENAME:
	  switch (q.id)
	   { case PFS_MIN_ID+4:
		 { int i;
		   FSTATE *f;
		   p.err = ENOSPC;
		   for (i=0;i<NWFILES;i++)
		    { f = &filestate[i];
		      if (! (f->flags & FSF_PRESENT))
		       { p.err = 0;
			 p.id = PFS_NO_ID;
			 p.type = 0;
			 break;
		       }
		    }
		 }
		break;
	     default:
		p.err = EACCES;
		break;
	   }
	  break;
     }
  }
 switch (p.err)
  { case ENOENT:
       fprintf(stderr,"ENOENT\n");
       break;
    case EACCES:
       fprintf(stderr,"EACCES\n");
       break;
    case 0:
       fprintf(stderr,"id %d, type %d\n",p.id,p.type);
       break;
    default:
       fprintf(stderr,"impossible error %d\n",p.err);
       abort();
       break;
  }
 sendreply(fd,req->id,&p,sizeof(p));
}
