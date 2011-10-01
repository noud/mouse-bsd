#include <stdio.h>
#include <stdlib.h>

extern const char *__progname;

#include "requests.h"

void rq_done(RQARGS)
{
 fprintf(stderr,"%s: done\n",__progname);
 exit(0);
}
