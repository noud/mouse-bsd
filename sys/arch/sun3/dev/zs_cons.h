/*	$NetBSD: zs_cons.h,v 1.3 1998/01/05 07:03:22 perry Exp $	*/


struct consdev;

extern void *zs_conschan;

extern void nullcnprobe __P((struct consdev *));

extern int  zs_getc __P((void *arg));
extern void zs_putc __P((void *arg, int c));

struct zschan *
zs_get_chan_addr __P((int zsc_unit, int channel));

#ifdef	KGDB
void zs_kgdb_init __P((void));
#endif
