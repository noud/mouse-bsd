#include <stdio.h>
#include <sys/mount.h>

extern const char *__progname;

#include "requests.h"

void rq_statfs(RQARGS)
{
 struct statfs stf;

 SIZECHECK(0,"STATFS");
 fprintf(stderr,"%s: statfs\n",__progname);
 maybefail();
 stf.f_bsize = 512;
 stf.f_blocks = 20;
 stf.f_bfree = 10;
 stf.f_bavail = 10;
 stf.f_files = 1;
 stf.f_ffree = 1;
 stf.f_syncwrites = 0;
 stf.f_asyncwrites = 0;
 stf.f_spare[1] = 0;
 sendreply(fd,req->id,&stf,sizeof(struct statfs));
}
