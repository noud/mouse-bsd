#ifndef WH_SXVAR_H_fe5b7768_
#define WH_SXVAR_H_fe5b7768_

struct sx_softc {
  struct device dev;
  bus_addr_t uregs;
  bus_space_tag_t t;
  bus_space_handle_t h;
  int node;
  } ;

extern struct sx_softc *sxdev;

#endif
