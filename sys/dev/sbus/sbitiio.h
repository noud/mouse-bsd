#ifndef _SBITIIO_H_a93d8650_
#define _SBITIIO_H_a93d8650_

/* This file is in the public domain. */

#include <sys/ioccom.h>

struct sbiti_reg {
  unsigned int off;  /* offset, in units of size */
  unsigned int size; /* 1, 2, or 4 */
  unsigned int val;  /* value: input for write, output for read */
  } ;

#define SBITIIOC_GREGOFF  _IOR('&',0,unsigned int)
#define SBITIIOC_GREGSIZE _IOR('&',1,unsigned int)
#define SBITIIOC_RD_REG   _IOWR('&',2,struct sbiti_reg)
#define SBITIIOC_WR_REG   _IOW('&',3,struct sbiti_reg)

#endif
