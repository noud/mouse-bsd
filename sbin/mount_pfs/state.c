#include "state.h"

FSTATE filestate[NWFILES];

void init_state(void)
{
 int i;

 for (i=0;i<NWFILES;i++)
  { filestate[i].flags = 0;
    filestate[i].maxsize = 65536;
  }
}
