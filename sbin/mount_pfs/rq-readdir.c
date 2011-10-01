#define DEFINE_DIRENT_SIZE

#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <strings.h>

extern const char *__progname;

#include "state.h"
#include "requests.h"

typedef struct f F;
typedef struct fv FV;

struct f {
  const char *name;
  int type;
  int id;
  } ;

struct fv {
  F *v;
  int n;
  } ;
#define FVINIT(arr) { &arr[0], sizeof(arr)/sizeof(arr[0]) }

static F f_root[] = { { ".", DT_DIR, PFS_ROOT_ID },
		      { "..", DT_DIR, PFS_ROOT_ID },
		      { "dir", DT_DIR, PFS_MIN_ID+0 },
		      { "file", DT_REG, PFS_MIN_ID+1 },
		      { "link", DT_LNK, PFS_MIN_ID+3 },
		      { "wdir", DT_DIR, PFS_MIN_ID+4 } };
static FV fv_root = FVINIT(f_root);

static F f_dir[] = { { ".", DT_DIR, PFS_MIN_ID+0 },
		     { "..", DT_DIR, PFS_ROOT_ID },
		     { "file2", DT_REG, PFS_MIN_ID+2 } };
static FV fv_dir = FVINIT(f_dir);

static F f_fileasdir[] = { { ".", DT_DIR, PFS_MIN_ID+0 },
			   { "..", DT_DIR, PFS_ROOT_ID },
			   { "sub1", DT_REG, PFS_MIN_ID+6 },
			   { "sub2", DT_REG, PFS_MIN_ID+7 } };
static FV fv_fileasdir = FVINIT(f_fileasdir);

void rq_readdir(RQARGS)
{
 struct pfs_readdir_req q;
 char dbuf[PFS_REP_MAX]
	__attribute__((__aligned__(__alignof__(struct pfs_readdir_rep))));
#define p (*(struct pfs_readdir_rep *)&dbuf[0])
 char *dp;
 int sizeleft;
 int spaceleft;

 static int maybe_add_entry(int id, int type, const char *name, off_t cookie)
  { struct dirent ent;
    int nl;
    int size;
    int space;
    ent.d_fileno = id;
    ent.d_type = type;
    nl = strlen(name);
    if (nl > MAXNAMLEN)
     { fprintf(stderr,"...\n%s: readdir name too large (%d)\n",__progname,nl);
       exit(1);
     }
    ent.d_namlen = nl;
    bcopy(name,&ent.d_name[0],nl);
    ent.d_reclen = &ent.d_name[nl] - (char *)&ent;
    size = DIRENT_SIZE(&ent);
    space = ent.d_reclen + ((q.flags & PFS_READDIR_Q_COOKIES) ? sizeof(off_t) : 0);
    if ((size > sizeleft) || (space > spaceleft)) return(0);
    sizeleft -= size;
    spaceleft -= space;
    bcopy(&ent,dp,ent.d_reclen);
    dp += ent.d_reclen;
    if (q.flags & PFS_READDIR_Q_COOKIES)
     { bcopy(&cookie,dp,sizeof(off_t));
       dp += sizeof(off_t);
     }
    p.count ++;
    p.lastoff = cookie;
    return(1);
  }

 SIZECHECK(sizeof(q),"READDIR");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: readdir: id %d, off %qd, maxsize %d, flags %#x: ",__progname,q.id,(long long int)q.off,q.maxsize,q.flags);
 maybefail();
 dp = &dbuf[sizeof(struct pfs_readdir_rep)];
 sizeleft = q.maxsize;
 spaceleft = PFS_REP_MAX - sizeof(struct pfs_rep) - sizeof(struct pfs_readdir_rep);
 p.err = 0;
 switch (q.id)
  { case PFS_ROOT_ID:
	{ FV *fv;
	  F *f;
	  int i;
	  fv = &fv_root;
	  if (0)
	   {
    case PFS_MIN_ID+0:
	     fv = &fv_dir;
	   }
	  if (0)
	   {
    case PFS_MIN_ID+5:
	     fv = &fv_fileasdir;
	   }
	  p.lastoff = 0;
	  p.count = 0;
	  if (q.off >= fv->n)
	   { p.flags = PFS_READDIR_P_EOF;
	   }
	  else
	   { p.flags = 0;
	     i = q.off;
	     f = &fv->v[i];
	     while ((i < fv->n) && maybe_add_entry(f->id,f->type,f->name,i+1))
	      { i ++;
		f ++;
	      }
	     if (i >= fv->n) p.flags |= PFS_READDIR_P_EOF;
	   }
	  fprintf(stderr,"count %d, flags %#x, lastoff %qd\n",p.count,p.flags,(long long int)p.lastoff);
	}
       break;
    case PFS_MIN_ID+4:
       p.lastoff = 0;
       p.count = 0;
       if (q.off >= 2+NWFILES)
	{ p.flags = PFS_READDIR_P_EOF;
	  break;
	}
       switch <"offsetswitch"> (q.off)
	{ int i;
	  int j;
	  int k;
	  case 0:
	     if (! maybe_add_entry(PFS_MIN_ID+4,DT_DIR,".",1)) break;
	     /* fall through */
	  case 1:
	     if (! maybe_add_entry(PFS_ROOT_ID,DT_DIR,"..",2)) break;
	     q.off = 2;
	     /* fall through */
	  default:
	     j = q.off - 2;
	     k = 3;
	     for (i=0;i<NWFILES;i++)
	      { FSTATE *f;
		f = &filestate[i];
		if (f->flags & FSF_PRESENT)
		 { if (j > 0)
		    { j --;
		    }
		   else
		    { if (! maybe_add_entry(NWBASE+i,DT_REG,f->name,k)) break <"offsetswitch">;
		    }
		   k ++;
		 }
	      }
	     p.flags = PFS_READDIR_P_EOF;
	     break;
	}
       break;
    default:
       fprintf(stderr,"ESTALE\n");
       p.err = ESTALE;
       break;
  }
 sendreply(fd,req->id,&dbuf[0],dp-&dbuf[0]);
}
