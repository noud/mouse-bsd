#ifndef _ZXVAR_H_6c70ef4e_
#define _ZXVAR_H_6c70ef4e_

#include <sys/device.h>
#include <machine/fbvar.h>
#include <machine/zxintf.h>
#include <dev/sbus/sbusvar.h>

struct zx_softc {
  struct device dev;
  struct sbusdev sbdev;
  struct fbdevice fbdev;
  bus_space_tag_t bustag;
  bus_type_t bustype;
  bus_addr_t paddr;
  volatile struct zx_command_ss0 *cmd0;
  volatile struct zx_command_ss1 *cmd1;
  volatile struct zx_draw_ss0 *draw0[5];
  volatile struct zx_draw_ss1 *draw1[5];
  volatile struct zx_draw_ss0 *draw0bc;
  volatile struct zx_draw_ss1 *draw1bc;
  volatile struct zx_cross *cross;
  volatile unsigned int *pixels;
  } ;

extern void zxattach(struct zx_softc *, const char *, int, int);

#endif
