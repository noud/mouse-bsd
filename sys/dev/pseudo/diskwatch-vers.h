#ifndef _DISKWATCH_VERS_H_b3f5e0b4_
#define _DISKWATCH_VERS_H_b3f5e0b4_

/*
 * We have to include this very early, to get __NetBSD_Version__.
 *
 * This also hides a lot of include-file bugs; I don't entirely
 *  understand _why_ including <sys/param.h> hides so many of them when
 *  they're clearly not missing parameters - for example, uvm/uvm.h,
 *  through the chain uvm/uvm_extern.h -> uvm/uvm_param.h ->
 *  machine/vmparam.h, uses struct simplelock without making sure it's
 *  defined first.  Since <sys/param.h> doesn't define it, I don't
 *  understand why including it first hides this bug, but it does.
 *
 * This is a mixed blessing.  It means we don't have to fix (or paper
 *  over) a lot of stuff - but it also means we lose out on a chance to
 *  do find (and fix properly) that same lot of stuff.  Since we have
 *  to include it anyway to get __NetBSD_Version__, we make a virtue of
 *  necessity and declare we consider the former the greater. :)
 */
#include <sys/param.h>

/*
 * Unfortunately there is no way for us to create a #define which
 *  diskwatch.c can use to get the appropriate list of include files.
 *  So, instead, there's a flag (DW_INCLUDE_INCLUDES) which can be
 *  defined before including this; when that's done, it includes the
 *  appropriate include files, based on a flag define set up the first
 *  time through (which need not be different from the time with
 *  DW_INCLUDE_INCLUDES set).  DW_INCLUDE_INCLUDES is undefined after
 *  the include files are brought in.
 *
 * This is a bit ugly, in that it separates the includes from the code,
 *  but I consider the alternative - flag macros here which diskwatch.c
 *  uses to include the correct files - even worse; it means that
 *  porting to a new OS version requires creating a new flag macro
 *  visible to diskwatch.c (the current scheme requires creating one
 *  anyway, but it's private to this file), and clutters diskwatch.c
 *  (which I'd like to keep relatively uncluttered) with lots of
 *  "extra" #include lines.
 *
 * However, C's preprocessor design pretty much compels something of
 *  the sort.  This choice means that this file can't have multiple
 *  inclusions streamlined by (eg) gcc's recognition and optimization
 *  of the #ifndef/#define/.../#endif idiom, but I think that counts as
 *  optimizing machine costs (a little unnecessary processing, which
 *  doesn't happen much anyway since we don't include this file very
 *  often) at the expense of human costs (dealing with cluttered code).
 */

#define DW_VERSION_NOT_SUPPORTED

#ifndef __NetBSD_Version__
#error "No __NetBSD_Version__ - this version not supported"
#endif

#if __NetBSD_Version__ == 400000003
/* 4.0.1 */
/*
 * The description of __NetBSD_Version__ in <sys/param.h> leads me to
 *  expect it to be 400000100, not 400000003.  I don't know why the
 *  discrepancy, but 400000003 is what it's set to in 4.0.1.
 */
#undef DW_VERSION_NOT_SUPPORTED
#define DW_DEVICE_ROUTINE_DECLS /* nothing */
#define RING_ALLOC(nb) malloc(nb,M_DEVBUF,M_WAITOK)
#define RING_FREE(p,nb) free(p,M_DEVBUF)
#define IOCTL_SET(e,dev,u)\
	do {								\
		const struct cdevsw *sw;				\
		sw = cdevsw_lookup(dev);				\
		if (sw == 0) e = ENXIO; /* can this happen? */		\
		else e = (*sw->d_ioctl)(dev,DWIOCSET,(void *)&u,FWRITE,0);\
	} while (0)
#define IOCTL_CLR(dev,u)\
	do {								\
		const struct cdevsw *sw;				\
		sw = cdevsw_lookup(dev);				\
		if (sw == 0) {						\
			/* Can this happen?  If the device doesn't */	\
			/* exist any longer, it can probably be */	\
			/* considered implicitly cleared.... */		\
			printf("NOTE: diskwatch clearing vanished device\n");\
		} else {						\
			(*sw->d_ioctl)(dev,DWIOCCLR,(void *)&u,FWRITE,0);\
		}							\
	} while (0)
#define WAKEUP_SELECT(sc)\
	selnotify(&sc->rsel,0)
#define DEVSW_SCLASS static
#define PROCTYPE struct lwp *
#define FD_OUT_OF_RANGE(fd,fdp) (fd > fdp->fd_lastfile)
#define LOCK_FDP(fdp) simple_lock(&fdp->fd_slock)
#define DEAD_FD_TEST(fp) 0
#define LOCK_FP(fdp) simple_lock(&fp->f_slock)
#define USABLE_FP(fp) FILE_IS_USABLE(fp)
#define UNLOCK_FP(fp) simple_unlock(&fp->f_slock)
#define UNLOCK_FDP(fdp) simple_unlock(&fdp->fd_slock)
#define DEFINE_CDEVSW\
	const struct cdevsw diskwatch_cdevsw				\
	 = { &diskwatchopen, &diskwatchclose, &diskwatchread,		\
	     &diskwatchwrite, &diskwatchioctl, nostop, notty,		\
	     &diskwatchpoll, nommap, nokqfilter, D_OTHER };
#define DW_VERSION_4_0_1
#endif

#if __NetBSD_Version__ == 301000000
/* 3.1 */
#undef DW_VERSION_NOT_SUPPORTED
#define DW_DEVICE_ROUTINE_DECLS /* nothing */
#define RING_ALLOC(nb) ((void *)uvm_km_kmemalloc(kernel_map,uvm.kernel_object,nb,0))
#define RING_FREE(p,nb) uvm_km_free(kernel_map,(vaddr_t)p,nb)
#define IOCTL_SET(e,dev,u)\
	do {								\
		const struct cdevsw *sw;				\
		sw = cdevsw_lookup(dev);				\
		if (sw == 0) e = ENXIO; /* can this happen? */		\
		else e = (*sw->d_ioctl)(dev,DWIOCSET,(void *)&u,FWRITE,0);\
	} while (0)
#define IOCTL_CLR(dev,u)\
	do {								\
		const struct cdevsw *sw;				\
		sw = cdevsw_lookup(dev);				\
		if (sw == 0) {						\
			/* Can this happen?  If the device doesn't */	\
			/* exist any longer, it can probably be */	\
			/* considered implicitly cleared.... */		\
			printf("NOTE: diskwatch clearing vanished device\n");\
		} else {						\
			(*sw->d_ioctl)(dev,DWIOCCLR,(void *)&u,FWRITE,0);\
		}							\
	} while (0)
#define WAKEUP_SELECT(sc)\
	selnotify(&sc->rsel,0)
#define DEVSW_SCLASS static
#define PROCTYPE struct proc *
#define FD_OUT_OF_RANGE(fd,fdp) (fd > fdp->fd_lastfile)
#define LOCK_FDP(fdp) simple_lock(&fdp->fd_slock)
#define LOCK_FP(fdp) simple_lock(&fp->f_slock)
#define DEAD_FD_TEST(fp) 0
#define LOCK_FP(fdp) simple_lock(&fp->f_slock)
#define USABLE_FP(fp) FILE_IS_USABLE(fp)
#define UNLOCK_FP(fp) simple_unlock(&fp->f_slock)
#define UNLOCK_FDP(fdp) simple_unlock(&fdp->fd_slock)
#define DEFINE_CDEVSW\
	const struct cdevsw diskwatch_cdevsw				\
	 = { &diskwatchopen, &diskwatchclose, &diskwatchread,		\
	     &diskwatchwrite, &diskwatchioctl, nostop, notty,		\
	     &diskwatchpoll, nommap, nokqfilter };
#define DW_VERSION_3_1
#endif

#if __NetBSD_Version__ == 104200000
/* 1.4T */
#undef DW_VERSION_NOT_SUPPORTED
#define DW_DEVICE_ROUTINE_DECLS \
	extern int diskwatchopen(dev_t, int, int, struct proc *);	\
	extern int diskwatchclose(dev_t, int, int, struct proc *);	\
	extern int diskwatchread(dev_t, struct uio *, int);		\
	extern int diskwatchwrite(dev_t, struct uio *, int);		\
	extern int diskwatchioctl(dev_t, u_long, caddr_t, int, struct proc *);\
	extern int diskwatchpoll(dev_t, int, struct proc *);
#define RING_ALLOC(nb) ((void *)uvm_km_kmemalloc(kernel_map,uvmexp.kmem_object,nb,0))
#define RING_FREE(p,nb) uvm_km_free(kernel_map,(vaddr_t)p,nb)
#define IOCTL_SET(e,dev,u)\
	e = (*cdevsw[major(dev)].d_ioctl)(dev,DWIOCSET,(void *)&u,FWRITE,0)
#define IOCTL_CLR(dev,u)\
	(*cdevsw[major(dev)].d_ioctl)(dev,DWIOCCLR,(void *)&u,FWRITE,0);
#define WAKEUP_SELECT(sc)\
	selwakeup(&sc->rsel)
#define DEVSW_SCLASS /* nothing */
#define PROCTYPE struct proc *
#define FD_OUT_OF_RANGE(fd,fdp) (fd >= fdp->fd_nfiles)
#define LOCK_FDP(fdp) /* nothing */
#define DEAD_FD_TEST(fp) (fp->f_iflags & FIF_WANTCLOSE)
#define LOCK_FP(fdp) /* nothing */
#define USABLE_FP(fp) 1
#define UNLOCK_FP(fp) /* nothing */
#define UNLOCK_FDP(fdp) /* nothing */
#define DEFINE_CDEVSW /* nothing */
#define DW_VERSION_1_4T
#endif

#ifdef DW_VERSION_NOT_SUPPORTED
#error "This NetBSD version not supported"
#endif

#endif

#ifdef DW_INCLUDE_INCLUDES

#ifdef DW_VERSION_4_0_1
#undef DW_INCLUDE_INCLUDES
/* XXX paper over bugs in include files */
/* <sys/fcntl.h> uses off_t */
#include <sys/types.h>
/* End papering over bugs in include files */
#include <sys/lwp.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <sys/selinfo.h>
#include <sys/filedesc.h>
#include <miscfs/specfs/specdev.h>
#endif

#ifdef DW_VERSION_3_1
#undef DW_INCLUDE_INCLUDES
#include <uvm/uvm.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <miscfs/specfs/specdev.h>
#endif

#ifdef DW_VERSION_1_4T
#undef DW_INCLUDE_INCLUDES
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/filio.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/select.h>
#include <vm/vm_kern.h>
#include <sys/filedesc.h>
#include <uvm/uvm_extern.h>
#include <miscfs/specfs/specdev.h>
#endif

#ifdef DW_INCLUDE_INCLUDES
#error "missing includes block - DW_INCLUDE_INCLUDES survived"
#endif

#endif
