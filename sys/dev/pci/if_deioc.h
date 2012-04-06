#ifndef _IF_DEIOC_H_fe88cb21_
#define _IF_DEIOC_H_fe88cb21_

#include <sys/ioctl.h>

struct de_mac {
  unsigned char mac[6];
  } ;

#define DEIOC_GETMAC _IOR('d',1,struct de_mac)
#define DEIOC_SETMAC _IOW('d',2,struct de_mac)

#endif
