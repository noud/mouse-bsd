/*	$NetBSD: extern.h,v 1.21 2000/01/08 23:12:37 itojun Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *      @(#)extern.h	8.1 (Berkeley) 6/6/93
 */

#include <sys/cdefs.h>
#include <fcntl.h>
#include <kvm.h>

extern struct	command global_commands[];
extern struct	mode *curmode;
extern struct	mode modes[];
extern struct	text *xtext;
extern WINDOW	*wnd;
extern char	**dr_name;
extern char	c, *namp, hostname[];
extern double	avenrun[3];
extern float	*dk_mspw;
extern kvm_t	*kd;
extern long	ntext, textp;
extern int	*dk_select;
extern int	CMDLINE;
extern int	dk_ndrive;
extern int	hz, stathz;
extern int	naptime, col;
extern int	nhosts;
extern int	nports;
extern int	protos;
extern int	verbose;
extern int	nflag;

struct inpcb;
#ifdef INET6
struct in6pcb;
#endif

int	 checkhost __P((struct inpcb *));
int	 checkport __P((struct inpcb *));
#ifdef INET6
int	 checkhost6 __P((struct in6pcb *));
int	 checkport6 __P((struct in6pcb *));
#endif
void	 closebufcache __P((WINDOW *));
void	 closeicmp __P ((WINDOW *));
void	 closeiostat __P((WINDOW *));
void	 closeip __P ((WINDOW *));
void	 closekre __P((WINDOW *));
void	 closembufs __P((WINDOW *));
void	 closenetstat __P((WINDOW *));
void	 closepigs __P((WINDOW *));
void	 closeswap __P((WINDOW *));
void	 closetcp __P ((WINDOW *));
void	 command __P((char *));
void	 die __P((int));
void	 disks_add __P((char *));
void	 disks_delete __P((char *));
void	 disks_drives __P((char *));
void	 display __P((int));
int	 dkinit __P((int, gid_t));
void	 error __P((const char *fmt, ...));
void	 fetchbufcache __P((void));
void	 fetchicmp __P((void));
void	 fetchiostat __P((void));
void	 fetchip __P((void));
void	 fetchkre __P((void));
void	 fetchmbufs __P((void));
void	 fetchnetstat __P((void));
void	 fetchpigs __P((void));
void	 fetchswap __P((void));
void	 fetchtcp __P((void));
void	 global_help __P((char *args));
void	 global_interval __P((char *args));
void	 global_load __P((char *args));
void	 global_quit __P((char *args));
void	 global_stop __P((char *args));
int	 initbufcache __P((void));
int	 initicmp __P((void));
int	 initiostat __P((void));
int	 initip __P((void));
int	 initkre __P((void));
int	 initmbufs __P((void));
int	 initnetstat __P((void));
int	 initpigs __P((void));
int	 initswap __P((void));
int	 inittcp __P((void));
void	 iostat_bars __P((char *));
void	 iostat_numbers __P((char *));
void	 iostat_secs __P((char *));
int	 keyboard __P((void)) __attribute__((__noreturn__));
ssize_t	 kvm_ckread __P((void *, void *, size_t));
void	 labelbufcache __P((void));
void	 labelicmp __P((void));
void	 labeliostat __P((void));
void	 labelip __P((void));
void	 labelkre __P((void));
void	 labelmbufs __P((void));
void	 labelnetstat __P((void));
void	 labelpigs __P((void));
void	 labelps __P((void));
void	 labels __P((void));
void	 labelswap __P((void));
void	 labeltcp __P((void));
void	 labeltcpsyn __P((void));
void	 netstat_all __P((char *));
void	 netstat_display __P((char *));
void	 netstat_ignore __P((char *));
void	 netstat_names __P((char *));
void	 netstat_numbers __P((char *));
void	 netstat_reset __P((char *));
void	 netstat_show __P((char *));
void	 netstat_tcp __P((char *));
void	 netstat_udp __P((char *));
void	 nlisterr __P((struct nlist []));
WINDOW	*openbufcache __P((void));
WINDOW	*openicmp __P((void));
WINDOW	*openiostat __P((void));
WINDOW	*openip __P((void));
WINDOW	*openkre __P((void));
WINDOW	*openmbufs __P((void));
WINDOW	*opennetstat __P((void));
WINDOW	*openpigs __P((void));
WINDOW	*openswap __P((void));
WINDOW	*opentcp __P((void));
void	 ps_user __P((char *));
void	 redraw __P((int));
void	 showbufcache __P((void));
void	 showicmp __P((void));
void	 showiostat __P((void));
void	 showip __P((void));
void	 showkre __P((void));
void	 showmbufs __P((void));
void	 shownetstat __P((void));
void	 showpigs __P((void));
void	 showps __P((void));
void	 showswap __P((void));
void	 showtcp __P((void));
void	 showtcpsyn __P((void));
void	 status __P((void));
void	 vmstat_boot __P((char *args));
void	 vmstat_run __P((char *args));
void	 vmstat_time __P((char *args));
void	 vmstat_zero __P((char *args));
#ifdef INET6
void	 closeip6 __P ((WINDOW *));
void	 fetchip6 __P((void));
int	 initip6 __P((void));
void	 labelip6 __P((void));
WINDOW	*openip6 __P((void));
void	 showip6 __P((void));
#endif
#ifdef IPSEC
void	 closeipsec __P ((WINDOW *));
void	 fetchipsec __P((void));
int	 initipsec __P((void));
void	 labelipsec __P((void));
WINDOW	*openipsec __P((void));
void	 showipsec __P((void));
#endif
