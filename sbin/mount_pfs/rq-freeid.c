#include <stdio.h>
#include <strings.h>

extern const char *__progname;

#include "requests.h"

void rq_freeid(RQARGS)
{
 struct pfs_freeid_req q;

 SIZECHECK(sizeof(q),"FREEID");
 bcopy(data,&q,sizeof(q));
 fprintf(stderr,"%s: freeid: id %d\n",__progname,q.id);
 /* we ignore FREEIDs, since our ID<->entity mapping is fixed. */
}
