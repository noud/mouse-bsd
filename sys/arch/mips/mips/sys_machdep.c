/*	$NetBSD: sys_machdep.c,v 1.16 1999/04/24 08:10:42 simonb Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sys_machdep.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/trace.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <mips/sysarch.h>
#include <mips/cachectl.h>
#include <mips/locore.h>
#include <vm/vm.h>

#ifdef TRACE
int	nvualarm;

int
vtrace(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct vtrace_args /* {
		syscallarg(int) request;
		syscallarg(int) value;
	} */ *uap = v;
	int vdoualarm();

	switch (SCARG(uap, request)) {

	case VTR_DISABLE:		/* disable a trace point */
	case VTR_ENABLE:		/* enable a trace point */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[SCARG(uap, value)];
		traceflags[SCARG(uap, value)] = SCARG(uap, request);
		break;

	case VTR_VALUE:		/* return a trace point setting */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[SCARG(uap, value)];
		break;

	case VTR_UALARM:	/* set a real-time ualarm, less than 1 min */
		if (SCARG(uap, value) <= 0 || SCARG(uap, value) > 60 * hz ||
		    nvualarm > 5)
			return (EINVAL);
		nvualarm++;
		timeout(vdoualarm, (caddr_t)p->p_pid, SCARG(uap, value));
		break;

	case VTR_STAMP:
		trace(TR_STAMP, SCARG(uap, value), p->p_pid);
		break;
	}
	return (0);
}

void
vdoualarm(arg)
	int arg;
{
	struct proc *p;

	p = pfind(arg);
	if (p)
		psignal(p, 16);
	nvualarm--;
}
#endif /* TRACE */

int
sys_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(void *) parms;
	} */ *uap = v;

	int error = 0;

	switch(SCARG(uap, op)) {
	case MIPS_CACHEFLUSH: {
		struct mips_cacheflush_args cfua;

		error = copyin(SCARG(uap, parms), &cfua, sizeof(cfua));
		if (error != 0) return (error);
		error =  mips_user_cacheflush(p, cfua.va, cfua.nbytes,
		     cfua.whichcache);
		break;
	}
	case MIPS_CACHECTL: {
		struct mips_cachectl_args ccua;

		error = copyin(SCARG(uap, parms), &ccua, sizeof(ccua));
		if (error != 0) return (error);
		error = mips_user_cachectl(p, ccua.va, ccua.nbytes, ccua.ctl);
		break;
	}
	default:
		error = ENOSYS;
		break;
	}
	return (error);
}


/*
 * Hande a user-space  request to flush a given virutal address
 * rangefrom the i-cache, d-cache, or both.
 */
int
mips_user_cacheflush(p, va, nbytes, whichcache)
	struct proc *p;
	vaddr_t va;
	int nbytes, whichcache;
{

	/* validate the cache we're going to  flush. */
	switch (whichcache) {
	    case ICACHE:
	    case DCACHE:
	    case BCACHE:
		break;
	    default:
		return (EINVAL);
	}

#ifndef notyet
	/* For now, just flush all of both caches. */
	MachFlushCache();
	return (0);

#else
	void * uncached_physaddr;
	u_int len;

	/*
	 * Invalidate each page in the virtual-address range,
	 * by manually mapping to a  physical address and
	 * invalidating the PA.
	 */
	for (base = (void*) addr; nbytes > 0; base += len, nbytes -= len) {
		/* XXX vm_fault?  */
		if (whichcache & ICACHE) {
			MachFlushCache(uncached_physaddr, len);
		}
		if (whichcache & DCACHE) {
			MachFlushDCache(uncached_physaddr, len);
		}
	}
#endif
}

/*
 * Hande a user-space to make a given range of virtual addresses
 * non-cacheable.
 */
int
mips_user_cachectl(p, va, nbytes, cachectl)
	struct proc *p;
	vaddr_t va;
	int nbytes, cachectl;
{
	/* validate the cache we're going to  flush. */
	switch (cachectl) {
	case CACHEABLE:
	case UNCACHEABLE:
		break;
	default:
		return (EINVAL);
	}

#ifndef notyet
	return(EOPNOTSUPP);
#else
	/*
	 * Use the merged mips3  pmap cache-control functions
	 * to change the cache attributes of each page in the virtual-address range,
	 * by manually mapping to a  physical address and changing the
	 * pmap attributes of the  PA of each page in the range.
	 * Force misses on non-present pages to be sure the cacheable bits
	 * get set.
	 */
#endif
}
