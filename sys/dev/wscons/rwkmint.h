#ifndef _RWKMINT_H_0de0588a_
#define _RWKMINT_H_0de0588a_

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wseventvar.h>

extern int wskbd_rwkm_open(int, struct wseventvar *);
extern void wskbd_rwkm_close(int);

extern int wsmouse_rwkm_open(int, struct wseventvar *);
extern void wsmouse_rwkm_close(int);

#endif
