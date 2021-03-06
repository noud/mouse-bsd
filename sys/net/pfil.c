/*	$NetBSD: pfil.c,v 1.10 2000/02/17 10:59:32 darrenr Exp $	*/

/*
 * Copyright (c) 1996 Matthew R. Green
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/protosw.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/pfil.h>

static void pfil_init __P((struct pfil_head *));
static void pfil_list_add(pfil_list_t *,
    int (*) __P((void *, int, struct ifnet *, int, struct mbuf **)), int);
static void pfil_list_remove(pfil_list_t *,
    int (*) __P((void *, int, struct ifnet *, int, struct mbuf **)));

static void
pfil_init(ph)
	 struct pfil_head *ph;
{

	TAILQ_INIT(&ph->ph_in);
	TAILQ_INIT(&ph->ph_out);
	ph->ph_init = 1;
}

/*
 * pfil_add_hook() adds a function to the packet filter hook.  the
 * flags are:
 *	PFIL_IN		call me on incoming packets
 *	PFIL_OUT	call me on outgoing packets
 *	PFIL_ALL	call me on all of the above
 *	PFIL_WAITOK	OK to call malloc with M_WAITOK.
 */
void
pfil_add_hook(func, flags, psw)
	int	(*func) __P((void *, int, struct ifnet *, int,
			     struct mbuf **));
	int	flags;
	struct	protosw	*psw;
{
	struct	pfil_head	*ph = &psw->pr_pfh;

	if (ph->ph_init == 0)
		pfil_init(ph);

	if (flags & PFIL_IN)
		pfil_list_add(&ph->ph_in, func, flags);
	if (flags & PFIL_OUT)
		pfil_list_add(&ph->ph_out, func, flags);
}

static void
pfil_list_add(list, func, flags)
	pfil_list_t *list;
	int	(*func) __P((void *, int, struct ifnet *, int,
			     struct mbuf **));
	int flags;
{
	struct packet_filter_hook *pfh;

	pfh = (struct packet_filter_hook *)malloc(sizeof(*pfh), M_IFADDR,
	    flags & PFIL_WAITOK ? M_WAITOK : M_NOWAIT);
	if (pfh == NULL)
		panic("no memory for packet filter hook");
	pfh->pfil_func = func;
	/*
	 * insert the input list in reverse order of the output list
	 * so that the same path is followed in or out of the kernel.
	 */
	TAILQ_INSERT_TAIL(list, pfh, pfil_link);
}

/*
 * pfil_remove_hook removes a specific function from the packet filter
 * hook list.
 */
void
pfil_remove_hook(func, flags, psw)
	int	(*func) __P((void *, int, struct ifnet *, int,
			     struct mbuf **));
	int	flags;
	struct	protosw	*psw;
{
	struct	pfil_head	*ph = &psw->pr_pfh;

	if (ph->ph_init == 0)
		pfil_init(ph);

	if (flags & PFIL_IN)
		pfil_list_remove(&ph->ph_in, func);
	if (flags & PFIL_OUT)
		pfil_list_remove(&ph->ph_out, func);
}

/*
 * pfil_list_remove is an internal function that takes a function off the
 * specified list.
 */
static void
pfil_list_remove(list, func)
	pfil_list_t *list;
	int	(*func) __P((void *, int, struct ifnet *, int,
			     struct mbuf **));
{
	struct packet_filter_hook *pfh;

	for (pfh = list->tqh_first; pfh; pfh = pfh->pfil_link.tqe_next)
		if (pfh->pfil_func == func) {
			TAILQ_REMOVE(list, pfh, pfil_link);
			free(pfh, M_IFADDR);
			return;
		}
	printf("pfil_list_remove:  no function on list\n");
#ifdef DIAGNOSTIC
	panic("pfil_list_remove");
#endif
}

struct packet_filter_hook *
pfil_hook_get(flag, psw)
	int flag;
	struct protosw *psw;
{
	struct	pfil_head	*ph = &psw->pr_pfh;

	if (ph->ph_init != 0)
		switch (flag) {
		case PFIL_IN:
			return (ph->ph_in.tqh_first);
		case PFIL_OUT:
			return (ph->ph_out.tqh_first);
		}
	return NULL;
}
