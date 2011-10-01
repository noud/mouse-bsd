#ifndef _STATE_H_4509351b_
#define _STATE_H_4509351b_

typedef struct fstate FSTATE;

struct fstate {
  int maxsize;
  unsigned int flags;
#define FSF_PRESENT 0x00000001
  int mode;
  int namelen;
  char *name;
  int cursize;
  char *contents;
  } ;

#define NWFILES 3
extern FSTATE filestate[NWFILES];
#define NWBASE (PFS_MIN_ID+8)

extern void init_state(void);

#endif
