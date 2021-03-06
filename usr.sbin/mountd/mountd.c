/* $NetBSD: mountd.c,v 1.62 2000/02/16 04:08:40 enami Exp $	 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Herb Hasler and Rick Macklem at The University of Guelph.
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
 */


/*
 * XXX The ISO support can't possibly work..
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif				/* not lint */

#ifndef lint
#if 0
static char     sccsid[] = "@(#)mountd.c  8.15 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: mountd.c,v 1.62 2000/02/16 04:08:40 enami Exp $");
#endif
#endif				/* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/ucred.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/mount.h>
#ifdef ISO
#include <netiso/iso.h>
#endif
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsmount.h>

#include <ufs/ufs/ufsmount.h>
#include <isofs/cd9660/cd9660_mount.h>
#include <msdosfs/msdosfsmount.h>
#include <adosfs/adosfs.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <netgroup.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netgroup.h>
#include <util.h>
#include "pathnames.h"
#ifdef KERBEROS
#include <kerberosIV/krb.h>
#include "kuid.h"
#endif

#include <stdarg.h>

/*
 * Structures for keeping the mount list and export list
 */
struct mountlist {
	struct mountlist *ml_next;
	char ml_host[RPCMNT_NAMELEN + 1];
	char ml_dirp[RPCMNT_PATHLEN + 1];
	int ml_flag;/* XXX more flags (same as dp_flag) */
};

struct dirlist {
	struct dirlist *dp_left;
	struct dirlist *dp_right;
	int dp_flag;
	struct hostlist *dp_hosts;	/* List of hosts this dir exported to */
	char dp_dirp[1];		/* Actually malloc'd to size of dir */
};
/* dp_flag bits */
#define	DP_DEFSET	0x1
#define DP_HOSTSET	0x2
#define DP_KERB		0x4
#define DP_NORESMNT	0x8

struct exportlist {
	struct exportlist *ex_next;
	struct dirlist *ex_dirl;
	struct dirlist *ex_defdir;
	int             ex_flag;
	fsid_t          ex_fs;
	char           *ex_fsdir;
	char           *ex_indexfile;
};
/* ex_flag bits */
#define	EX_LINKED	0x1

struct netmsk {
	u_int32_t       nt_net;
	u_int32_t       nt_mask;
	char           *nt_name;
};

union grouptypes {
	struct hostent *gt_hostent;
	struct netmsk   gt_net;
#ifdef ISO
	struct sockaddr_iso *gt_isoaddr;
#endif
};

struct grouplist {
	int             gr_type;
	union grouptypes gr_ptr;
	struct grouplist *gr_next;
};
/* Group types */
#define	GT_NULL		0x0
#define	GT_HOST		0x1
#define	GT_NET		0x2
#define	GT_ISO		0x4

struct hostlist {
	int             ht_flag;/* Uses DP_xx bits */
	struct grouplist *ht_grp;
	struct hostlist *ht_next;
};

struct fhreturn {
	int             fhr_flag;
	int             fhr_vers;
	nfsfh_t         fhr_fh;
};

/* Global defs */
static char    *add_expdir __P((struct dirlist **, char *, int));
static void add_dlist __P((struct dirlist **, struct dirlist *,
    struct grouplist *, int));
static void add_mlist __P((char *, char *, int));
static int check_dirpath __P((const char *, size_t, char *));
static int check_options __P((const char *, size_t, struct dirlist *));
static int chk_host __P((struct dirlist *, u_int32_t, int *, int *));
static int del_mlist __P((char *, char *, struct sockaddr *));
static struct dirlist *dirp_search __P((struct dirlist *, char *));
static int do_mount __P((const char *, size_t, struct exportlist *,
    struct grouplist *, int, struct ucred *, char *, int, struct statfs *));
static int do_opt __P((const char *, size_t, char **, char **,
    struct exportlist *, struct grouplist *, int *, int *, struct ucred *));
static struct exportlist *ex_search __P((fsid_t *));
static int parse_directory __P((const char *, size_t, struct grouplist *,
    int, char *, struct exportlist **, struct statfs *));
static int parse_host_netgroup __P((const char *, size_t, struct exportlist *,
    struct grouplist *, char *, int *, struct grouplist **));
static struct exportlist *get_exp __P((void));
static void free_dir __P((struct dirlist *));
static void free_exp __P((struct exportlist *));
static void free_grp __P((struct grouplist *));
static void free_host __P((struct hostlist *));
static void get_exportlist __P((int));
static int get_host __P((const char *, size_t, const char *,
    struct grouplist *));
static struct hostlist *get_ht __P((void));
static void get_mountlist __P((void));
static int get_net __P((char *, struct netmsk *, int));
static void free_exp_grp __P((struct exportlist *, struct grouplist *));
static struct grouplist *get_grp __P((void));
static void hang_dirp __P((struct dirlist *, struct grouplist *,
    struct exportlist *, int));
static void mntsrv __P((struct svc_req *, SVCXPRT *));
static void nextfield __P((char **, char **));
static void parsecred __P((char *, struct ucred *));
static int put_exlist __P((struct dirlist *, XDR *, struct dirlist *, int *));
static int scan_tree __P((struct dirlist *, u_int32_t));
static void send_umntall __P((int));
static int umntall_each __P((caddr_t, struct sockaddr_in *));
static int xdr_dir __P((XDR *, char *));
static int xdr_explist __P((XDR *, caddr_t));
static int xdr_fhs __P((XDR *, caddr_t));
static int xdr_mlist __P((XDR *, caddr_t));
static void *emalloc __P((size_t));
static char *estrdup __P((const char *));
#ifdef ISO
static int get_isoaddr __P((const char *, size_t, char *, struct grouplist *));
#endif
static struct exportlist *exphead;
static struct mountlist *mlhead;
static struct grouplist *grphead;
static char    *exname;
static struct ucred def_anon = {
	1,
	(uid_t) - 2,
	(gid_t) - 2,
	0,
	{}
};
	static int      opt_flags;
/* Bits for above */
#define	OP_MAPROOT	0x001
#define	OP_MAPALL	0x002
#define	OP_KERB		0x004
#define	OP_MASK		0x008
#define	OP_NET		0x010
#define	OP_ISO		0x020
#define	OP_ALLDIRS	0x040
#define OP_NORESPORT	0x080
#define OP_NORESMNT	0x100

static int      debug = 0;
#if 0
static void SYSLOG __P((int, const char *,...));
#endif
int main __P((int, char *[]));

/* Create RPC transports. */
static void create_transports(struct in_addr bindaddr, int bindport, SVCXPRT **udpp, SVCXPRT **tcpp)
{
 struct sockaddr_in sin;
 int usock;
 int tsock;
 SVCXPRT *ux;
 SVCXPRT *tx;

 if ((bindaddr.s_addr != INADDR_ANY) || (bindport != 0))
  { bzero(&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(sin);
    sin.sin_addr = bindaddr;
    sin.sin_port = bindport;
    usock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (usock < 0)
     { syslog(LOG_ERR,"Can't create UDP socket");
       exit(1);
     }
    if (bind(usock,(void *)&sin,sizeof(sin)) < 0)
     { syslog(LOG_ERR,"Can't bind UDP socket");
       exit(1);
     }
    bzero(&sin,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_len = sizeof(sin);
    sin.sin_addr = bindaddr;
    sin.sin_port = bindport;
    tsock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (tsock < 0)
     { syslog(LOG_ERR,"Can't create TCP socket");
       exit(1);
     }
    if (bind(tsock,(void *)&sin,sizeof(sin)) < 0)
     { syslog(LOG_ERR,"Can't bind TCP socket");
       exit(1);
     }
  }
 else
  { usock = RPC_ANYSOCK;
    tsock = RPC_ANYSOCK;
  }
 ux = svcudp_create(usock);
 if (ux == 0)
  { syslog(LOG_ERR,"Can't svcudp_create");
    exit(1);
  }
 tx = svctcp_create(tsock,0,0);
 if (tx == 0)
  { syslog(LOG_ERR,"Can't svctcp_create");
    exit(1);
  }
 *udpp = ux;
 *tcpp = tx;
}

/*
 * Mountd server for NFS mount protocol as described in:
 * NFS: Network File System Protocol Specification, RFC1094, Appendix A
 * The optional arguments are the exports file name
 * default: _PATH_EXPORTS
 * "-d" to enable debugging
 * and "-n" to allow nonroot mount.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *udptransp, *tcptransp;
	struct in_addr bindaddr;
	int bindport;
	int c;

	bindaddr.s_addr = INADDR_ANY;
	bindport = 0;
	while ((c = getopt(argc, argv, "a:p:dnr")) != -1)
		switch (c) {
		case 'a':
			if (! inet_aton(optarg,&bindaddr)) {
				fprintf(stderr, "Bad -a argument `%s'\n",optarg);
				exit(1);
			}
			break;
		case 'p':
			bindport = atoi(optarg);
			if ((bindport < 0) || (bindport > 65535)) {
				fprintf(stderr, "Bad -p argument `%s'\n",optarg);
				exit(1);
			}
			break;
		case 'd':
			debug = 1;
			break;
			/* Compatibility */
		case 'n':
		case 'r':
			break;
		default:
			fprintf(stderr, "Usage: mountd [-a addr] [-d] [export_file]\n");
			exit(1);
		};
	argc -= optind;
	argv += optind;
	grphead = NULL;
	exphead = NULL;
	mlhead = NULL;
	if (argc == 1)
		exname = *argv;
	else
		exname = _PATH_EXPORTS;
	openlog("mountd", LOG_PID, LOG_DAEMON);
	if (debug)
		(void)fprintf(stderr, "Getting export list.\n");
	get_exportlist(0);
	if (debug)
		(void)fprintf(stderr, "Getting mount list.\n");
	get_mountlist();
	if (debug)
		(void)fprintf(stderr, "Here we go.\n");
	if (debug == 0) {
		daemon(0, 0);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
	}
	(void)signal(SIGHUP, get_exportlist);
	(void)signal(SIGTERM, send_umntall);
	pidfile(NULL);
	create_transports(bindaddr, bindport, &udptransp, &tcptransp);
	pmap_unset(RPCPROG_MNT, RPCMNT_VER1);
	pmap_unset(RPCPROG_MNT, RPCMNT_VER3);
	if (!svc_register(udptransp, RPCPROG_MNT, RPCMNT_VER1, mntsrv,
	    IPPROTO_UDP) ||
	    !svc_register(udptransp, RPCPROG_MNT, RPCMNT_VER3, mntsrv,
	    IPPROTO_UDP) ||
	    !svc_register(tcptransp, RPCPROG_MNT, RPCMNT_VER1, mntsrv,
	    IPPROTO_TCP) ||
	    !svc_register(tcptransp, RPCPROG_MNT, RPCMNT_VER3, mntsrv,
	    IPPROTO_TCP)) {
		syslog(LOG_ERR, "Can't register mount");
		exit(1);
	}
#ifdef KERBEROS
	kuidinit();
#endif
	svc_run();
	syslog(LOG_ERR, "Mountd died");
	exit(1);
}

/*
 * The mount rpc service
 */
void
mntsrv(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	struct exportlist *ep;
	struct dirlist *dp;
	struct fhreturn fhr;
	struct stat     stb;
	struct statfs   fsb;
	struct hostent *hp;
	struct in_addr  saddr;
	u_short         sport;
	char            rpcpath[RPCMNT_PATHLEN + 1], dirpath[MAXPATHLEN];
	long            bad = EACCES;
	int             defset, hostset, ret;
	sigset_t        sighup_mask;

	(void)sigemptyset(&sighup_mask);
	(void)sigaddset(&sighup_mask, SIGHUP);
	saddr = transp->xp_raddr.sin_addr;
	sport = ntohs(transp->xp_raddr.sin_port);
	hp = NULL;
#ifdef KERBEROS
	kuidreset();
#endif
	ret = 0;
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, NULL))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case MOUNTPROC_MNT:
		if (!svc_getargs(transp, xdr_dir, rpcpath)) {
			svcerr_decode(transp);
			return;
		}
		/*
		 * Get the real pathname and make sure it is a file or
		 * directory that exists.
		 */
		if (realpath(rpcpath, dirpath) == 0 ||
		    stat(dirpath, &stb) < 0 ||
		    (!S_ISDIR(stb.st_mode) && !S_ISREG(stb.st_mode)) ||
		    statfs(dirpath, &fsb) < 0) {
			(void)chdir("/"); /* Just in case realpath doesn't */
			if (debug)
				(void)fprintf(stderr, "stat failed on %s\n",
				    dirpath);
			if (!svc_sendreply(transp, xdr_long, (caddr_t) &bad))
				syslog(LOG_ERR, "Can't send reply");
			return;
		}
		/* Check in the exports list */
		(void)sigprocmask(SIG_BLOCK, &sighup_mask, NULL);
		ep = ex_search(&fsb.f_fsid);
		hostset = defset = 0;
		if (ep && (chk_host(ep->ex_defdir, saddr.s_addr, &defset,
		   &hostset) || ((dp = dirp_search(ep->ex_dirl, dirpath)) &&
		   chk_host(dp, saddr.s_addr, &defset, &hostset)) ||
		   (defset && scan_tree(ep->ex_defdir, saddr.s_addr) == 0 &&
		   scan_tree(ep->ex_dirl, saddr.s_addr) == 0))) {
			if (sport >= IPPORT_RESERVED &&
			    !(hostset & DP_NORESMNT)) {
				syslog(LOG_NOTICE,
				    "Refused mount RPC from host %s port %d",
				    inet_ntoa(saddr), sport);
				svcerr_weakauth(transp);
				goto out;
			}
			if (hostset & DP_HOSTSET)
				fhr.fhr_flag = hostset;
			else
				fhr.fhr_flag = defset;
			fhr.fhr_vers = rqstp->rq_vers;
			/* Get the file handle */
			(void)memset(&fhr.fhr_fh, 0, sizeof(nfsfh_t));
			if (getfh(dirpath, (fhandle_t *) &fhr.fhr_fh) < 0) {
				bad = errno;
				syslog(LOG_ERR, "Can't get fh for %s", dirpath);
				if (!svc_sendreply(transp, xdr_long,
				    (char *)&bad))
					syslog(LOG_ERR, "Can't send reply");
				goto out;
			}
			if (!svc_sendreply(transp, xdr_fhs, (char *) &fhr))
				syslog(LOG_ERR, "Can't send reply");
			if (hp == NULL)
				hp = gethostbyaddr((const char *) &saddr,
				    sizeof(saddr), AF_INET);
			if (hp)
				add_mlist(hp->h_name, dirpath, hostset);
			else
				add_mlist(inet_ntoa(transp->xp_raddr.sin_addr),
				    dirpath, hostset);
			if (debug)
				(void)fprintf(stderr, "Mount successful.\n");
		} else {
			if (!svc_sendreply(transp, xdr_long, (caddr_t) &bad))
				syslog(LOG_ERR, "Can't send reply");
		}
out:
		(void)sigprocmask(SIG_UNBLOCK, &sighup_mask, NULL);
		return;
	case MOUNTPROC_DUMP:
		if (!svc_sendreply(transp, xdr_mlist, NULL))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case MOUNTPROC_UMNT:
		if (!svc_getargs(transp, xdr_dir, dirpath)) {
			svcerr_decode(transp);
			return;
		}
		hp = gethostbyaddr((caddr_t) &saddr, sizeof(saddr), AF_INET);
		if (hp)
			ret = del_mlist(hp->h_name, dirpath,
			    (struct sockaddr *) &transp->xp_raddr);
		ret |= del_mlist(inet_ntoa(transp->xp_raddr.sin_addr), dirpath,
		    (struct sockaddr *) &transp->xp_raddr);
		if (ret) {
			svcerr_weakauth(transp);
			return;
		}
		if (!svc_sendreply(transp, xdr_void, NULL))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case MOUNTPROC_UMNTALL:
		hp = gethostbyaddr((caddr_t)&saddr, sizeof(saddr), AF_INET);
		if (hp)
			ret = del_mlist(hp->h_name, NULL,
			    (struct sockaddr *)&transp->xp_raddr);
		ret |= del_mlist(inet_ntoa(transp->xp_raddr.sin_addr),
		    NULL, (struct sockaddr *)&transp->xp_raddr);
		if (ret) {
			svcerr_weakauth(transp);
			return;
		}
		if (!svc_sendreply(transp, xdr_void, NULL))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case MOUNTPROC_EXPORT:
	case MOUNTPROC_EXPORTALL:
		if (!svc_sendreply(transp, xdr_explist, NULL))
			syslog(LOG_ERR, "Can't send reply");
		return;

#ifdef KERBEROS
	case MOUNTPROC_KUIDMAP:
	case MOUNTPROC_KUIDUMAP:
	case MOUNTPROC_KUIDPURGE:
	case MOUNTPROC_KUIDUPURGE:
		kuidops(rqstp, transp);
		return;
#endif

	default:
		svcerr_noproc(transp);
		return;
	}
}

/*
 * Xdr conversion for a dirpath string
 */
static int
xdr_dir(xdrsp, dirp)
	XDR *xdrsp;
	char *dirp;
{

	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

/*
 * Xdr routine to generate file handle reply
 */
static int
xdr_fhs(xdrsp, cp)
	XDR *xdrsp;
	caddr_t cp;
{
	struct fhreturn *fhrp = (struct fhreturn *) cp;
	long ok = 0, len, auth;

	if (!xdr_long(xdrsp, &ok))
		return (0);
	switch (fhrp->fhr_vers) {
	case 1:
		return (xdr_opaque(xdrsp, (caddr_t)&fhrp->fhr_fh, NFSX_V2FH));
	case 3:
		len = NFSX_V3FH;
		if (!xdr_long(xdrsp, &len))
			return (0);
		if (!xdr_opaque(xdrsp, (caddr_t)&fhrp->fhr_fh, len))
			return (0);
		if (fhrp->fhr_flag & DP_KERB)
			auth = RPCAUTH_KERB4;
		else
			auth = RPCAUTH_UNIX;
		len = 1;
		if (!xdr_long(xdrsp, &len))
			return (0);
		return (xdr_long(xdrsp, &auth));
	};
	return (0);
}

int
xdr_mlist(xdrsp, cp)
	XDR *xdrsp;
	caddr_t cp;
{
	struct mountlist *mlp;
	int true = 1;
	int false = 0;
	char *strp;

	mlp = mlhead;
	while (mlp) {
		if (!xdr_bool(xdrsp, &true))
			return (0);
		strp = &mlp->ml_host[0];
		if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN))
			return (0);
		strp = &mlp->ml_dirp[0];
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
			return (0);
		mlp = mlp->ml_next;
	}
	if (!xdr_bool(xdrsp, &false))
		return (0);
	return (1);
}

/*
 * Xdr conversion for export list
 */
int
xdr_explist(xdrsp, cp)
	XDR *xdrsp;
	caddr_t cp;
{
	struct exportlist *ep;
	int false = 0;
	int putdef;
	sigset_t sighup_mask;

	(void)sigemptyset(&sighup_mask);
	(void)sigaddset(&sighup_mask, SIGHUP);
	(void)sigprocmask(SIG_BLOCK, &sighup_mask, NULL);
	ep = exphead;
	while (ep) {
		putdef = 0;
		if (put_exlist(ep->ex_dirl, xdrsp, ep->ex_defdir, &putdef))
			goto errout;
		if (ep->ex_defdir && putdef == 0 &&
		    put_exlist(ep->ex_defdir, xdrsp, NULL, &putdef))
			goto errout;
		ep = ep->ex_next;
	}
	(void)sigprocmask(SIG_UNBLOCK, &sighup_mask, NULL);
	if (!xdr_bool(xdrsp, &false))
		return (0);
	return (1);
errout:
	(void)sigprocmask(SIG_UNBLOCK, &sighup_mask, NULL);
	return (0);
}

/*
 * Called from xdr_explist() to traverse the tree and export the
 * directory paths.  Assumes SIGHUP has already been masked.
 */
int
put_exlist(dp, xdrsp, adp, putdefp)
	struct dirlist *dp;
	XDR *xdrsp;
	struct dirlist *adp;
	int *putdefp;
{
	struct grouplist *grp;
	struct hostlist *hp;
	int true = 1;
	int false = 0;
	int gotalldir = 0;
	char *strp;

	if (dp) {
		if (put_exlist(dp->dp_left, xdrsp, adp, putdefp))
			return (1);
		if (!xdr_bool(xdrsp, &true))
			return (1);
		strp = dp->dp_dirp;
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
			return (1);
		if (adp && !strcmp(dp->dp_dirp, adp->dp_dirp)) {
			gotalldir = 1;
			*putdefp = 1;
		}
		if ((dp->dp_flag & DP_DEFSET) == 0 &&
		    (gotalldir == 0 || (adp->dp_flag & DP_DEFSET) == 0)) {
			hp = dp->dp_hosts;
			while (hp) {
				grp = hp->ht_grp;
				if (grp->gr_type == GT_HOST) {
					if (!xdr_bool(xdrsp, &true))
						return (1);
					strp = grp->gr_ptr.gt_hostent->h_name;
					if (!xdr_string(xdrsp, &strp,
							RPCMNT_NAMELEN))
						return (1);
				} else if (grp->gr_type == GT_NET) {
					if (!xdr_bool(xdrsp, &true))
						return (1);
					strp = grp->gr_ptr.gt_net.nt_name;
					if (!xdr_string(xdrsp, &strp,
							RPCMNT_NAMELEN))
						return (1);
				}
				hp = hp->ht_next;
				if (gotalldir && hp == NULL) {
					hp = adp->dp_hosts;
					gotalldir = 0;
				}
			}
		}
		if (!xdr_bool(xdrsp, &false))
			return (1);
		if (put_exlist(dp->dp_right, xdrsp, adp, putdefp))
			return (1);
	}
	return (0);
}

static int
parse_host_netgroup(line, lineno, ep, tgrp, cp, has_host, grp)
	const char *line;
	size_t lineno;
	struct exportlist *ep;
	struct grouplist *tgrp;
	char *cp;
	int *has_host;
	struct grouplist **grp;
{
	const char *hst, *usr, *dom;
	int netgrp;

	if (ep == NULL) {
		syslog(LOG_ERR, "\"%s\", line %ld: No current export",
		    line, (unsigned long)lineno);
		return 0;
	}
	setnetgrent(cp);
	netgrp = getnetgrent(&hst, &usr, &dom);
	do {
		if (*has_host) {
			(*grp)->gr_next = get_grp();
			*grp = (*grp)->gr_next;
		}
		if (netgrp) {
			if (hst == NULL) {
				syslog(LOG_ERR,
				    "\"%s\", line %ld: No host in netgroup %s",
				    line, (unsigned long)lineno, cp);
				goto bad;
			}
			if (get_host(line, lineno, hst, *grp))
				goto bad;
		} else if (get_host(line, lineno, cp, *grp))
			goto bad;
		*has_host = TRUE;
	} while (netgrp && getnetgrent(&hst, &usr, &dom));

	endnetgrent();
	return 1;
bad:
	endnetgrent();
	return 0;

}

static int
parse_directory(line, lineno, tgrp, got_nondir, cp, ep, fsp)
	const char *line;
	size_t lineno;
	struct grouplist *tgrp;
	int got_nondir;
	char *cp;
	struct exportlist **ep;
	struct statfs *fsp;
{
	if (!check_dirpath(line, lineno, cp))
		return 0;

	if (statfs(cp, fsp) == -1) {
		syslog(LOG_ERR, "\"%s\", line %ld: statfs for `%s' failed: %m",
		    line, (unsigned long)lineno, cp);
		return 0;
	}

	if (got_nondir) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: Directories must precede files",
		    line, (unsigned long)lineno);
		return 0;
	}
	if (*ep) {
		if ((*ep)->ex_fs.val[0] != fsp->f_fsid.val[0] ||
		    (*ep)->ex_fs.val[1] != fsp->f_fsid.val[1]) {
			syslog(LOG_ERR,
			    "\"%s\", line %ld: filesystem ids disagree",
			    line, (unsigned long)lineno);
			return 0;
		}
	} else {
		/*
		 * See if this directory is already
		 * in the list.
		 */
		*ep = ex_search(&fsp->f_fsid);
		if (*ep == NULL) {
			*ep = get_exp();
			(*ep)->ex_fs = fsp->f_fsid;
			(*ep)->ex_fsdir = estrdup(fsp->f_mntonname);
			if (debug)
				(void)fprintf(stderr,
				    "Making new ep fs=0x%x,0x%x\n",
				    fsp->f_fsid.val[0], fsp->f_fsid.val[1]);
		} else {
			if (debug)
				(void)fprintf(stderr,
				    "Found ep fs=0x%x,0x%x\n",
				    fsp->f_fsid.val[0], fsp->f_fsid.val[1]);
		}
	}

	return 1;
}


/*
 * Get the export list
 */
/* ARGSUSED */
void
get_exportlist(n)
	int n;
{
	struct exportlist *ep, *ep2;
	struct grouplist *grp, *tgrp;
	struct exportlist **epp;
	struct dirlist *dirhead;
	struct statfs fsb, *fsp;
	struct hostent *hpe;
	struct ucred anon;
	char *cp, *endcp, *dirp, savedc;
	int has_host, exflags, got_nondir, dirplen, num, i;
	FILE *exp_file;
	char *line;
	size_t lineno = 0, len;


	/*
	 * First, get rid of the old list
	 */
	ep = exphead;
	while (ep) {
		ep2 = ep;
		ep = ep->ex_next;
		free_exp(ep2);
	}
	exphead = NULL;

	dirp = NULL;
	dirplen = 0;
	grp = grphead;
	while (grp) {
		tgrp = grp;
		grp = grp->gr_next;
		free_grp(tgrp);
	}
	grphead = NULL;

	/*
	 * And delete exports that are in the kernel for all local
	 * file systems.
	 * XXX: Should know how to handle all local exportable file systems
	 *      instead of just MOUNT_FFS.
	 */
	num = getmntinfo(&fsp, MNT_NOWAIT);
	for (i = 0; i < num; i++) {
		union {
			struct ufs_args ua;
			struct iso_args ia;
			struct mfs_args ma;
			struct msdosfs_args da;
			struct adosfs_args aa;
		} targs;

		if (!strncmp(fsp->f_fstypename, MOUNT_MFS, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_FFS, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_EXT2FS, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_MSDOS, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_ADOSFS, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_NULL, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_UMAP, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_UNION, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_CD9660, MFSNAMELEN) ||
		    !strncmp(fsp->f_fstypename, MOUNT_NTFS, MFSNAMELEN)) {
			bzero((char *) &targs, sizeof(targs));
			targs.ua.fspec = NULL;
			targs.ua.export.ex_flags = MNT_DELEXPORT;
			if (mount(fsp->f_fstypename, fsp->f_mntonname,
			    fsp->f_flags | MNT_UPDATE, &targs) == -1)
				syslog(LOG_ERR, "Can't delete exports for %s",
				    fsp->f_mntonname);
		}
		fsp++;
	}

	/*
	 * Read in the exports file and build the list, calling
	 * mount() as we go along to push the export rules into the kernel.
	 */
	if ((exp_file = fopen(exname, "r")) == NULL) {
		syslog(LOG_ERR, "Can't open %s: %m", exname);
		exit(2);
	}
	dirhead = NULL;
	while ((line = fparseln(exp_file, &len, &lineno, NULL, 0)) != NULL) {
		if (debug)
			(void)fprintf(stderr, "Got line %s\n", line);
		cp = line;
		nextfield(&cp, &endcp);
		if (cp == endcp)
			continue;	/* skip empty line */
		/*
		 * Set defaults.
		 */
		has_host = FALSE;
		anon = def_anon;
		exflags = MNT_EXPORTED;
		got_nondir = 0;
		opt_flags = 0;
		ep = NULL;

		/*
		 * Create new exports list entry
		 */
		len = endcp - cp;
		tgrp = grp = get_grp();
		while (len > 0) {
			if (len > RPCMNT_NAMELEN) {
				*endcp = '\0';
				syslog(LOG_ERR,
				    "\"%s\", line %ld: name `%s' is too long",
				    line, (unsigned long)lineno, cp);
				goto badline;
			}
			switch (*cp) {
			case '-':
				/*
				 * Option
				 */
				if (ep == NULL) {
					syslog(LOG_ERR,
				"\"%s\", line %ld: No current export list",
					    line, (unsigned long)lineno);
					goto badline;
				}
				if (debug)
					(void)fprintf(stderr, "doing opt %s\n",
					    cp);
				got_nondir = 1;
				if (do_opt(line, lineno, &cp, &endcp, ep, grp,
				    &has_host, &exflags, &anon))
					goto badline;
				break;

			case '/':
				/*
				 * Directory
				 */
				savedc = *endcp;
				*endcp = '\0';

				if (!parse_directory(line, lineno, tgrp,
				    got_nondir, cp, &ep, &fsb))
					goto badline;
				/*
				 * Add dirpath to export mount point.
				 */
				dirp = add_expdir(&dirhead, cp, len);
				dirplen = len;

				*endcp = savedc;
				break;

			default:
				/*
				 * Host or netgroup.
				 */
				savedc = *endcp;
				*endcp = '\0';

				if (!parse_host_netgroup(line, lineno, ep,
				    tgrp, cp, &has_host, &grp))
					goto badline;

				got_nondir = 1;

				*endcp = savedc;
				break;
			}

			cp = endcp;
			nextfield(&cp, &endcp);
			len = endcp - cp;
		}
		if (check_options(line, lineno, dirhead))
			goto badline;

		if (!has_host) {
			grp->gr_type = GT_HOST;
			if (debug)
				(void)fprintf(stderr,
				    "Adding a default entry\n");
			/* add a default group and make the grp list NULL */
			hpe = emalloc(sizeof(struct hostent));
			hpe->h_name = estrdup("Default");
			hpe->h_addrtype = AF_INET;
			hpe->h_length = sizeof(u_int32_t);
			hpe->h_addr_list = NULL;
			grp->gr_ptr.gt_hostent = hpe;

		} else if ((opt_flags & OP_NET) && tgrp->gr_next) {
			/*
			 * Don't allow a network export coincide with a list of
			 * host(s) on the same line.
			 */
			syslog(LOG_ERR,
			    "\"%s\", line %ld: Mixed exporting of networks and hosts is disallowed",
			    line, (unsigned long)lineno);
			goto badline;
		}
		/*
		 * Loop through hosts, pushing the exports into the kernel.
		 * After loop, tgrp points to the start of the list and
		 * grp points to the last entry in the list.
		 */
		grp = tgrp;
		do {
			if (do_mount(line, lineno, ep, grp, exflags, &anon,
			    dirp, dirplen, &fsb))
				goto badline;
		} while (grp->gr_next && (grp = grp->gr_next));

		/*
		 * Success. Update the data structures.
		 */
		if (has_host) {
			hang_dirp(dirhead, tgrp, ep, opt_flags);
			grp->gr_next = grphead;
			grphead = tgrp;
		} else {
			hang_dirp(dirhead, NULL, ep, opt_flags);
			free_grp(grp);
		}
		dirhead = NULL;
		if ((ep->ex_flag & EX_LINKED) == 0) {
			ep2 = exphead;
			epp = &exphead;

			/*
			 * Insert in the list in alphabetical order.
			 */
			while (ep2 && strcmp(ep2->ex_fsdir, ep->ex_fsdir) < 0) {
				epp = &ep2->ex_next;
				ep2 = ep2->ex_next;
			}
			if (ep2)
				ep->ex_next = ep2;
			*epp = ep;
			ep->ex_flag |= EX_LINKED;
		}
		goto nextline;
badline:
		free_exp_grp(ep, grp);
nextline:
		if (dirhead) {
			free_dir(dirhead);
			dirhead = NULL;
		}
		free(line);
	}
	(void)fclose(exp_file);
}

/*
 * Allocate an export list element
 */
static struct exportlist *
get_exp()
{
	struct exportlist *ep;

	ep = emalloc(sizeof(struct exportlist));
	(void)memset(ep, 0, sizeof(struct exportlist));
	return (ep);
}

/*
 * Allocate a group list element
 */
static struct grouplist *
get_grp()
{
	struct grouplist *gp;

	gp = emalloc(sizeof(struct grouplist));
	(void)memset(gp, 0, sizeof(struct grouplist));
	return (gp);
}

/*
 * Clean up upon an error in get_exportlist().
 */
static void
free_exp_grp(ep, grp)
	struct exportlist *ep;
	struct grouplist *grp;
{
	struct grouplist *tgrp;

	if (ep && (ep->ex_flag & EX_LINKED) == 0)
		free_exp(ep);
	while (grp) {
		tgrp = grp;
		grp = grp->gr_next;
		free_grp(tgrp);
	}
}

/*
 * Search the export list for a matching fs.
 */
static struct exportlist *
ex_search(fsid)
	fsid_t *fsid;
{
	struct exportlist *ep;

	ep = exphead;
	while (ep) {
		if (ep->ex_fs.val[0] == fsid->val[0] &&
		    ep->ex_fs.val[1] == fsid->val[1])
			return (ep);
		ep = ep->ex_next;
	}
	return (ep);
}

/*
 * Add a directory path to the list.
 */
static char *
add_expdir(dpp, cp, len)
	struct dirlist **dpp;
	char *cp;
	int len;
{
	struct dirlist *dp;

	dp = emalloc(sizeof(struct dirlist) + len);
	dp->dp_left = *dpp;
	dp->dp_right = NULL;
	dp->dp_flag = 0;
	dp->dp_hosts = NULL;
	(void)strcpy(dp->dp_dirp, cp);
	*dpp = dp;
	return (dp->dp_dirp);
}

/*
 * Hang the dir list element off the dirpath binary tree as required
 * and update the entry for host.
 */
void
hang_dirp(dp, grp, ep, flags)
	struct dirlist *dp;
	struct grouplist *grp;
	struct exportlist *ep;
	int flags;
{
	struct hostlist *hp;
	struct dirlist *dp2;

	if (flags & OP_ALLDIRS) {
		if (ep->ex_defdir)
			free(dp);
		else
			ep->ex_defdir = dp;
		if (grp == NULL) {
			ep->ex_defdir->dp_flag |= DP_DEFSET;
			if (flags & OP_KERB)
				ep->ex_defdir->dp_flag |= DP_KERB;
			if (flags & OP_NORESMNT)
				ep->ex_defdir->dp_flag |= DP_NORESMNT;
		} else
			while (grp) {
				hp = get_ht();
				if (flags & OP_KERB)
					hp->ht_flag |= DP_KERB;
				if (flags & OP_NORESMNT)
					hp->ht_flag |= DP_NORESMNT;
				hp->ht_grp = grp;
				hp->ht_next = ep->ex_defdir->dp_hosts;
				ep->ex_defdir->dp_hosts = hp;
				grp = grp->gr_next;
			}
	} else {

		/*
		 * Loop throught the directories adding them to the tree.
		 */
		while (dp) {
			dp2 = dp->dp_left;
			add_dlist(&ep->ex_dirl, dp, grp, flags);
			dp = dp2;
		}
	}
}

/*
 * Traverse the binary tree either updating a node that is already there
 * for the new directory or adding the new node.
 */
static void
add_dlist(dpp, newdp, grp, flags)
	struct dirlist **dpp;
	struct dirlist *newdp;
	struct grouplist *grp;
	int flags;
{
	struct dirlist *dp;
	struct hostlist *hp;
	int cmp;

	dp = *dpp;
	if (dp) {
		cmp = strcmp(dp->dp_dirp, newdp->dp_dirp);
		if (cmp > 0) {
			add_dlist(&dp->dp_left, newdp, grp, flags);
			return;
		} else if (cmp < 0) {
			add_dlist(&dp->dp_right, newdp, grp, flags);
			return;
		} else
			free(newdp);
	} else {
		dp = newdp;
		dp->dp_left = NULL;
		*dpp = dp;
	}
	if (grp) {

		/*
		 * Hang all of the host(s) off of the directory point.
		 */
		do {
			hp = get_ht();
			if (flags & OP_KERB)
				hp->ht_flag |= DP_KERB;
			if (flags & OP_NORESMNT)
				hp->ht_flag |= DP_NORESMNT;
			hp->ht_grp = grp;
			hp->ht_next = dp->dp_hosts;
			dp->dp_hosts = hp;
			grp = grp->gr_next;
		} while (grp);
	} else {
		dp->dp_flag |= DP_DEFSET;
		if (flags & OP_KERB)
			dp->dp_flag |= DP_KERB;
		if (flags & OP_NORESMNT)
			dp->dp_flag |= DP_NORESMNT;
	}
}

/*
 * Search for a dirpath on the export point.
 */
static struct dirlist *
dirp_search(dp, dirp)
	struct dirlist *dp;
	char *dirp;
{
	int cmp;

	if (dp) {
		cmp = strcmp(dp->dp_dirp, dirp);
		if (cmp > 0)
			return (dirp_search(dp->dp_left, dirp));
		else if (cmp < 0)
			return (dirp_search(dp->dp_right, dirp));
		else
			return (dp);
	}
	return (dp);
}

/*
 * Scan for a host match in a directory tree.
 */
static int
chk_host(dp, saddr, defsetp, hostsetp)
	struct dirlist *dp;
	u_int32_t saddr;
	int *defsetp;
	int *hostsetp;
{
	struct hostlist *hp;
	struct grouplist *grp;
	u_int32_t **addrp;

	if (dp) {
		if (dp->dp_flag & DP_DEFSET)
			*defsetp = dp->dp_flag;
		hp = dp->dp_hosts;
		while (hp) {
			grp = hp->ht_grp;
			switch (grp->gr_type) {
			case GT_HOST:
				addrp = (u_int32_t **)
				    grp->gr_ptr.gt_hostent->h_addr_list;
				for (; *addrp; addrp++) {
					if (**addrp != saddr)
						continue;
					*hostsetp = (hp->ht_flag | DP_HOSTSET);
					return (1);
				}
				break;
			case GT_NET:
				if ((saddr & grp->gr_ptr.gt_net.nt_mask) ==
				    grp->gr_ptr.gt_net.nt_net) {
					*hostsetp = (hp->ht_flag | DP_HOSTSET);
					return (1);
				}
				break;
			};
			hp = hp->ht_next;
		}
	}
	return (0);
}

/*
 * Scan tree for a host that matches the address.
 */
static int
scan_tree(dp, saddr)
	struct dirlist *dp;
	u_int32_t saddr;
{
	int defset, hostset;

	if (dp) {
		if (scan_tree(dp->dp_left, saddr))
			return (1);
		if (chk_host(dp, saddr, &defset, &hostset))
			return (1);
		if (scan_tree(dp->dp_right, saddr))
			return (1);
	}
	return (0);
}

/*
 * Traverse the dirlist tree and free it up.
 */
static void
free_dir(dp)
	struct dirlist *dp;
{

	if (dp) {
		free_dir(dp->dp_left);
		free_dir(dp->dp_right);
		free_host(dp->dp_hosts);
		free(dp);
	}
}

/*
 * Parse the option string and update fields.
 * Option arguments may either be -<option>=<value> or
 * -<option> <value>
 */
static int
do_opt(line, lineno, cpp, endcpp, ep, grp, has_hostp, exflagsp, cr)
	const char *line;
	size_t lineno;
	char **cpp, **endcpp;
	struct exportlist *ep;
	struct grouplist *grp;
	int *has_hostp;
	int *exflagsp;
	struct ucred *cr;
{
	char *cpoptarg, *cpoptend;
	char *cp, *endcp, *cpopt, savedc, savedc2;
	int allflag, usedarg;

	cpopt = *cpp;
	cpopt++;
	cp = *endcpp;
	savedc = *cp;
	*cp = '\0';
	while (cpopt && *cpopt) {
		allflag = 1;
		usedarg = -2;
		savedc2 = '\0';
		if ((cpoptend = strchr(cpopt, ',')) != NULL) {
			*cpoptend++ = '\0';
			if ((cpoptarg = strchr(cpopt, '=')) != NULL)
				*cpoptarg++ = '\0';
		} else {
			if ((cpoptarg = strchr(cpopt, '=')) != NULL)
				*cpoptarg++ = '\0';
			else {
				*cp = savedc;
				nextfield(&cp, &endcp);
				**endcpp = '\0';
				if (endcp > cp && *cp != '-') {
					cpoptarg = cp;
					savedc2 = *endcp;
					*endcp = '\0';
					usedarg = 0;
				}
			}
		}
		if (!strcmp(cpopt, "ro") || !strcmp(cpopt, "o")) {
			*exflagsp |= MNT_EXRDONLY;
		} else if (cpoptarg && (!strcmp(cpopt, "maproot") ||
		    !(allflag = strcmp(cpopt, "mapall")) ||
		    !strcmp(cpopt, "root") || !strcmp(cpopt, "r"))) {
			usedarg++;
			parsecred(cpoptarg, cr);
			if (allflag == 0) {
				*exflagsp |= MNT_EXPORTANON;
				opt_flags |= OP_MAPALL;
			} else
				opt_flags |= OP_MAPROOT;
		} else if (!strcmp(cpopt, "kerb") || !strcmp(cpopt, "k")) {
			*exflagsp |= MNT_EXKERB;
			opt_flags |= OP_KERB;
		} else if (cpoptarg && (!strcmp(cpopt, "mask") ||
		    !strcmp(cpopt, "m"))) {
			if (get_net(cpoptarg, &grp->gr_ptr.gt_net, 1)) {
				syslog(LOG_ERR,
				    "\"%s\", line %ld: Bad mask: %s",
				    line, (unsigned long)lineno, cpoptarg);
				return (1);
			}
			usedarg++;
			opt_flags |= OP_MASK;
		} else if (cpoptarg && (!strcmp(cpopt, "network") ||
		    !strcmp(cpopt, "n"))) {
			if (grp->gr_type != GT_NULL) {
				syslog(LOG_ERR,
				    "\"%s\", line %ld: Network/host conflict",
				    line, (unsigned long)lineno);
				return (1);
			} else if (get_net(cpoptarg, &grp->gr_ptr.gt_net, 0)) {
				syslog(LOG_ERR,
				    "\"%s\", line %ld: Bad net: %s",
				    line, (unsigned long)lineno, cpoptarg);
				return (1);
			}
			grp->gr_type = GT_NET;
			*has_hostp = 1;
			usedarg++;
			opt_flags |= OP_NET;
		} else if (!strcmp(cpopt, "alldirs")) {
			opt_flags |= OP_ALLDIRS;
		} else if (!strcmp(cpopt, "noresvmnt")) {
			opt_flags |= OP_NORESMNT;
		} else if (!strcmp(cpopt, "noresvport")) {
			opt_flags |= OP_NORESPORT;
			*exflagsp |= MNT_EXNORESPORT;
		} else if (!strcmp(cpopt, "public")) {
			*exflagsp |= (MNT_EXNORESPORT | MNT_EXPUBLIC);
			opt_flags |= OP_NORESPORT;
		} else if (!strcmp(cpopt, "webnfs")) {
			*exflagsp |= (MNT_EXNORESPORT | MNT_EXPUBLIC |
			    MNT_EXRDONLY | MNT_EXPORTANON);
			opt_flags |= (OP_MAPALL | OP_NORESPORT);
		} else if (cpoptarg && !strcmp(cpopt, "index")) {
			ep->ex_indexfile = strdup(cpoptarg);
#ifdef ISO
		} else if (cpoptarg && !strcmp(cpopt, "iso")) {
			if (get_isoaddr(line, lineno, cpoptarg, grp))
				return (1);
			*has_hostp = 1;
			usedarg++;
			opt_flags |= OP_ISO;
#endif /* ISO */
		} else {
			syslog(LOG_ERR,
			    "\"%s\", line %ld: Bad opt %s",
			    line, (unsigned long)lineno, cpopt);
			return (1);
		}
		if (usedarg >= 0) {
			*endcp = savedc2;
			**endcpp = savedc;
			if (usedarg > 0) {
				*cpp = cp;
				*endcpp = endcp;
			}
			return (0);
		}
		cpopt = cpoptend;
	}
	**endcpp = savedc;
	return (0);
}

/*
 * Translate a character string to the corresponding list of network
 * addresses for a hostname.
 */
static int
get_host(line, lineno, cp, grp)
	const char *line;
	size_t lineno;
	const char *cp;
	struct grouplist *grp;
{
	struct hostent *hp, *nhp;
	char **addrp, **naddrp;
	struct hostent  t_host;
	int i;
	u_int32_t saddr;
	char *aptr[2];

	if (grp->gr_type != GT_NULL) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: Bad netgroup type for ip host %s",
		    line, (unsigned long)lineno, cp);
		return (1);
	}
	if ((hp = gethostbyname(cp)) == NULL) {
		if (isdigit(*cp)) {
			saddr = inet_addr(cp);
			if (saddr == -1) {
				syslog(LOG_ERR,
				    "\"%s\", line %ld: inet_addr failed for %s",
				    line, (unsigned long) lineno, cp);
				return (1);
			}
			if ((hp = gethostbyaddr((const char *) &saddr,
			    sizeof(saddr), AF_INET)) == NULL) {
				hp = &t_host;
				hp->h_name = (char *) cp;
				hp->h_addrtype = AF_INET;
				hp->h_length = sizeof(u_int32_t);
				hp->h_addr_list = aptr;
				aptr[0] = (char *) &saddr;
				aptr[1] = NULL;
			}
		} else {
			syslog(LOG_ERR,
			    "\"%s\", line %ld: gethostbyname failed for %s: %s",
			    line, (unsigned long) lineno, cp,
			    hstrerror(h_errno));
			return (1);
		}
	}
	grp->gr_type = GT_HOST;
	nhp = grp->gr_ptr.gt_hostent = emalloc(sizeof(struct hostent));
	(void)memcpy(nhp, hp, sizeof(struct hostent));
	nhp->h_name = estrdup(hp->h_name);
	addrp = hp->h_addr_list;
	i = 1;
	while (*addrp++)
		i++;
	naddrp = nhp->h_addr_list = emalloc(i * sizeof(char *));
	addrp = hp->h_addr_list;
	while (*addrp) {
		*naddrp = emalloc(hp->h_length);
		(void)memcpy(*naddrp, *addrp, hp->h_length);
		addrp++;
		naddrp++;
	}
	*naddrp = NULL;
	if (debug)
		(void)fprintf(stderr, "got host %s\n", hp->h_name);
	return (0);
}

/*
 * Free up an exports list component
 */
static void
free_exp(ep)
	struct exportlist *ep;
{

	if (ep->ex_defdir) {
		free_host(ep->ex_defdir->dp_hosts);
		free(ep->ex_defdir);
	}
	if (ep->ex_fsdir)
		free(ep->ex_fsdir);
	if (ep->ex_indexfile)
		free(ep->ex_indexfile);
	free_dir(ep->ex_dirl);
	free(ep);
}

/*
 * Free hosts.
 */
static void
free_host(hp)
	struct hostlist *hp;
{
	struct hostlist *hp2;

	while (hp) {
		hp2 = hp;
		hp = hp->ht_next;
		free(hp2);
	}
}

static struct hostlist *
get_ht()
{
	struct hostlist *hp;

	hp = emalloc(sizeof(struct hostlist));
	hp->ht_next = NULL;
	hp->ht_flag = 0;
	return (hp);
}

#ifdef ISO
/*
 * Translate an iso address.
 */
static int
get_isoaddr(line, lineno, cp, grp)
	const char *line;
	size_t lineno;
	char *cp;
	struct grouplist *grp;
{
	struct iso_addr *isop;
	struct sockaddr_iso *isoaddr;

	if (grp->gr_type != GT_NULL) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: Bad netgroup type for iso addr %s",
		    line, (unsigned long)lineno, cp);
		return (1);
	}
	if ((isop = iso_addr(cp)) == NULL) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: Bad iso addr %s",
		    line, (unsigned long)lineno, cp);
		return (1);
	}
	isoaddr = emalloc(sizeof(struct sockaddr_iso));
	(void)memset(isoaddr, 0, sizeof(struct sockaddr_iso));
	(void)memcpy(&isoaddr->siso_addr, isop, sizeof(struct iso_addr));
	isoaddr->siso_len = sizeof(struct sockaddr_iso);
	isoaddr->siso_family = AF_ISO;
	grp->gr_type = GT_ISO;
	grp->gr_ptr.gt_isoaddr = isoaddr;
	return (0);
}
#endif				/* ISO */

/*
 * error checked malloc and strdup
 */
static void *
emalloc(n)
	size_t n;
{
	void *ptr = malloc(n);

	if (ptr == NULL) {
		syslog(LOG_ERR, "%m");
		exit(2);
	}
	return ptr;
}

static char *
estrdup(s)
	const char *s;
{
	char *n = strdup(s);

	if (n == NULL) {
		syslog(LOG_ERR, "%m");
		exit(2);
	}
	return n;
}

/*
 * Do the mount syscall with the update flag to push the export info into
 * the kernel.
 */
static int
do_mount(line, lineno, ep, grp, exflags, anoncrp, dirp, dirplen, fsb)
	const char *line;
	size_t lineno;
	struct exportlist *ep;
	struct grouplist *grp;
	int exflags;
	struct ucred *anoncrp;
	char *dirp;
	int dirplen;
	struct statfs *fsb;
{
	char *cp = NULL;
	u_int32_t **addrp;
	int done;
	char savedc = '\0';
	struct sockaddr_in sin, imask;
	union {
		struct ufs_args ua;
		struct iso_args ia;
		struct mfs_args ma;
		struct msdosfs_args da;
		struct adosfs_args aa;
	} args;
	u_int32_t net;

	args.ua.fspec = 0;
	args.ua.export.ex_flags = exflags;
	args.ua.export.ex_anon = *anoncrp;
	args.ua.export.ex_indexfile = ep->ex_indexfile;
	(void)memset(&sin, 0, sizeof(sin));
	(void)memset(&imask, 0, sizeof(imask));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(sin);
	imask.sin_family = AF_INET;
	imask.sin_len = sizeof(sin);
	if (grp->gr_type == GT_HOST)
		addrp = (u_int32_t **) grp->gr_ptr.gt_hostent->h_addr_list;
	else
		addrp = NULL;
	done = FALSE;
	while (!done) {
		switch (grp->gr_type) {
		case GT_HOST:
			if (addrp) {
				sin.sin_addr.s_addr = **addrp;
				args.ua.export.ex_addrlen = sizeof(sin);
			} else
				args.ua.export.ex_addrlen = 0;
			args.ua.export.ex_addr = (struct sockaddr *)&sin;
			args.ua.export.ex_masklen = 0;
			break;
		case GT_NET:
			if (grp->gr_ptr.gt_net.nt_mask)
				imask.sin_addr.s_addr =
				    grp->gr_ptr.gt_net.nt_mask;
			else {
				net = ntohl(grp->gr_ptr.gt_net.nt_net);
				if (IN_CLASSA(net))
					imask.sin_addr.s_addr =
					    inet_addr("255.0.0.0");
				else if (IN_CLASSB(net))
					imask.sin_addr.s_addr =
					    inet_addr("255.255.0.0");
				else
					imask.sin_addr.s_addr =
					    inet_addr("255.255.255.0");
				grp->gr_ptr.gt_net.nt_mask =
				    imask.sin_addr.s_addr;
			}
			sin.sin_addr.s_addr = grp->gr_ptr.gt_net.nt_net;
			args.ua.export.ex_addr = (struct sockaddr *) &sin;
			args.ua.export.ex_addrlen = sizeof(sin);
			args.ua.export.ex_mask = (struct sockaddr *) &imask;
			args.ua.export.ex_masklen = sizeof(imask);
			break;
#ifdef ISO
		case GT_ISO:
			args.ua.export.ex_addr =
			    (struct sockaddr *) grp->gr_ptr.gt_isoaddr;
			args.ua.export.ex_addrlen =
			    sizeof(struct sockaddr_iso);
			args.ua.export.ex_masklen = 0;
			break;
#endif				/* ISO */
		default:
			syslog(LOG_ERR, "\"%s\", line %ld: Bad netgroup type",
			    line, (unsigned long)lineno);
			if (cp)
				*cp = savedc;
			return (1);
		};

		/*
		 * XXX:
		 * Maybe I should just use the fsb->f_mntonname path instead
		 * of looping back up the dirp to the mount point??
		 * Also, needs to know how to export all types of local
		 * exportable file systems and not just MOUNT_FFS.
		 */
		while (mount(fsb->f_fstypename, dirp,
		    fsb->f_flags | MNT_UPDATE, &args) == -1) {
			if (cp)
				*cp-- = savedc;
			else
				cp = dirp + dirplen - 1;
			if (errno == EPERM) {
				syslog(LOG_ERR,
		    "\"%s\", line %ld: Can't change attributes for %s to %s",
				    line, (unsigned long)lineno,
				    dirp, (grp->gr_type == GT_HOST) ?
				    grp->gr_ptr.gt_hostent->h_name :
				    (grp->gr_type == GT_NET) ?
				    grp->gr_ptr.gt_net.nt_name :
				    "Unknown");
				return (1);
			}
			/* back up over the last component */
			while (*cp == '/' && cp > dirp)
				cp--;
			while (*(cp - 1) != '/' && cp > dirp)
				cp--;
			if (cp == dirp) {
				if (debug)
					(void)fprintf(stderr, "mnt unsucc\n");
				syslog(LOG_ERR,
				    "\"%s\", line %ld: Can't export %s",
				    line, (unsigned long)lineno, dirp);
				return (1);
			}
			savedc = *cp;
			*cp = '\0';
		}
		if (addrp) {
			++addrp;
			if (*addrp == NULL)
				done = TRUE;
		} else
			done = TRUE;
	}
	if (cp)
		*cp = savedc;
	return (0);
}

/*
 * Translate a net address.
 */
static int
get_net(cp, net, maskflg)
	char *cp;
	struct netmsk *net;
	int maskflg;
{
	struct netent *np;
	long netaddr;
	struct in_addr inetaddr, inetaddr2;
	char *name;

	if ((np = getnetbyname(cp)) != NULL)
		inetaddr = inet_makeaddr(np->n_net, 0);
	else if (isdigit(*cp)) {
		if ((netaddr = inet_network(cp)) == -1)
			return (1);
		inetaddr = inet_makeaddr(netaddr, 0);
		/*
		 * Due to arbritrary subnet masks, you don't know how many
		 * bits to shift the address to make it into a network,
		 * however you do know how to make a network address into
		 * a host with host == 0 and then compare them.
		 * (What a pest)
		 */
		if (!maskflg) {
			setnetent(0);
			while ((np = getnetent()) != NULL) {
				inetaddr2 = inet_makeaddr(np->n_net, 0);
				if (inetaddr2.s_addr == inetaddr.s_addr)
					break;
			}
			endnetent();
		}
	} else
		return (1);
	if (maskflg)
		net->nt_mask = inetaddr.s_addr;
	else {
		if (np)
			name = np->n_name;
		else
			name = inet_ntoa(inetaddr);
		net->nt_name = estrdup(name);
		net->nt_net = inetaddr.s_addr;
	}
	return (0);
}

/*
 * Parse out the next white space separated field
 */
static void
nextfield(cp, endcp)
	char **cp;
	char **endcp;
{
	char *p;

	p = *cp;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\n' || *p == '\0')
		*cp = *endcp = p;
	else {
		*cp = p++;
		while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
			p++;
		*endcp = p;
	}
}

/*
 * Parse a description of a credential.
 */
static void
parsecred(namelist, cr)
	char *namelist;
	struct ucred *cr;
{
	char *name;
	int cnt;
	char *names;
	struct passwd *pw;
	struct group *gr;
	int ngroups, groups[NGROUPS + 1];

	/*
	 * Set up the unprivileged user.
	 */
	cr->cr_ref = 1;
	cr->cr_uid = -2;
	cr->cr_gid = -2;
	cr->cr_ngroups = 0;
	/*
	 * Get the user's password table entry.
	 */
	names = strsep(&namelist, " \t\n");
	name = strsep(&names, ":");
	if (isdigit(*name) || *name == '-')
		pw = getpwuid(atoi(name));
	else
		pw = getpwnam(name);
	/*
	 * Credentials specified as those of a user.
	 */
	if (names == NULL) {
		if (pw == NULL) {
			syslog(LOG_ERR, "Unknown user: %s", name);
			return;
		}
		cr->cr_uid = pw->pw_uid;
		ngroups = NGROUPS + 1;
		if (getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups))
			syslog(LOG_ERR, "Too many groups");
		/*
		 * Convert from int's to gid_t's and compress out duplicate
		 */
		cr->cr_ngroups = ngroups - 1;
		cr->cr_gid = groups[0];
		for (cnt = 1; cnt < ngroups; cnt++)
			cr->cr_groups[cnt - 1] = groups[cnt];
		return;
	}
	/*
	 * Explicit credential specified as a colon separated list:
	 *	uid:gid:gid:...
	 */
	if (pw != NULL)
		cr->cr_uid = pw->pw_uid;
	else if (isdigit(*name) || *name == '-')
		cr->cr_uid = atoi(name);
	else {
		syslog(LOG_ERR, "Unknown user: %s", name);
		return;
	}
	cr->cr_ngroups = 0;
	while (names != NULL && *names != '\0' && cr->cr_ngroups < NGROUPS) {
		name = strsep(&names, ":");
		if (isdigit(*name) || *name == '-') {
			cr->cr_groups[cr->cr_ngroups++] = atoi(name);
		} else {
			if ((gr = getgrnam(name)) == NULL) {
				syslog(LOG_ERR, "Unknown group: %s", name);
				continue;
			}
			cr->cr_groups[cr->cr_ngroups++] = gr->gr_gid;
		}
	}
	if (names != NULL && *names != '\0' && cr->cr_ngroups == NGROUPS)
		syslog(LOG_ERR, "Too many groups");
}

#define	STRSIZ	(RPCMNT_NAMELEN+RPCMNT_PATHLEN+50)
/*
 * Routines that maintain the remote mounttab
 */
static void
get_mountlist()
{
	struct mountlist *mlp, **mlpp;
	char *host, *dirp, *cp;
	char str[STRSIZ];
	FILE *mlfile;

	if ((mlfile = fopen(_PATH_RMOUNTLIST, "r")) == NULL) {
		syslog(LOG_ERR, "Can't open %s: %m", _PATH_RMOUNTLIST);
		return;
	}
	mlpp = &mlhead;
	while (fgets(str, STRSIZ, mlfile) != NULL) {
		cp = str;
		host = strsep(&cp, " \t\n");
		dirp = strsep(&cp, " \t\n");
		if (host == NULL || dirp == NULL)
			continue;
		mlp = emalloc(sizeof(*mlp));
		(void)strncpy(mlp->ml_host, host, RPCMNT_NAMELEN);
		mlp->ml_host[RPCMNT_NAMELEN] = '\0';
		(void)strncpy(mlp->ml_dirp, dirp, RPCMNT_PATHLEN);
		mlp->ml_dirp[RPCMNT_PATHLEN] = '\0';
		mlp->ml_next = NULL;
		*mlpp = mlp;
		mlpp = &mlp->ml_next;
	}
	(void)fclose(mlfile);
}

static int
del_mlist(hostp, dirp, saddr)
	char *hostp, *dirp;
	struct sockaddr *saddr;
{
	struct mountlist *mlp, **mlpp;
	struct mountlist *mlp2;
	struct sockaddr_in *sin = (struct sockaddr_in *)saddr;
	FILE *mlfile;
	int fnd = 0, ret = 0;

	mlpp = &mlhead;
	mlp = mlhead;
	while (mlp) {
		if (!strcmp(mlp->ml_host, hostp) &&
		    (!dirp || !strcmp(mlp->ml_dirp, dirp))) {
			if (!(mlp->ml_flag & DP_NORESMNT) &&
			    ntohs(sin->sin_port) >= IPPORT_RESERVED) {
				syslog(LOG_NOTICE,
				"Umount request for %s:%s from %s refused\n",
				    mlp->ml_host, mlp->ml_dirp,
				    inet_ntoa(sin->sin_addr));
				ret = -1;
				goto cont;
			}
			fnd = 1;
			mlp2 = mlp;
			*mlpp = mlp = mlp->ml_next;
			free(mlp2);
		} else {
cont:
			mlpp = &mlp->ml_next;
			mlp = mlp->ml_next;
		}
	}
	if (fnd) {
		if ((mlfile = fopen(_PATH_RMOUNTLIST, "w")) == NULL) {
			syslog(LOG_ERR, "Can't update %s: %m",
			    _PATH_RMOUNTLIST);
			return ret;
		}
		mlp = mlhead;
		while (mlp) {
			(void)fprintf(mlfile, "%s %s\n", mlp->ml_host,
		            mlp->ml_dirp);
			mlp = mlp->ml_next;
		}
		(void)fclose(mlfile);
	}
	return ret;
}

static void
add_mlist(hostp, dirp, flags)
	char *hostp, *dirp;
	int flags;
{
	struct mountlist *mlp, **mlpp;
	FILE *mlfile;

	mlpp = &mlhead;
	mlp = mlhead;
	while (mlp) {
		if (!strcmp(mlp->ml_host, hostp) && !strcmp(mlp->ml_dirp, dirp))
			return;
		mlpp = &mlp->ml_next;
		mlp = mlp->ml_next;
	}
	mlp = emalloc(sizeof(*mlp));
	strncpy(mlp->ml_host, hostp, RPCMNT_NAMELEN);
	mlp->ml_host[RPCMNT_NAMELEN] = '\0';
	strncpy(mlp->ml_dirp, dirp, RPCMNT_PATHLEN);
	mlp->ml_dirp[RPCMNT_PATHLEN] = '\0';
	mlp->ml_flag = flags;
	mlp->ml_next = NULL;
	*mlpp = mlp;
	if ((mlfile = fopen(_PATH_RMOUNTLIST, "a")) == NULL) {
		syslog(LOG_ERR, "Can't update %s: %m", _PATH_RMOUNTLIST);
		return;
	}
	(void)fprintf(mlfile, "%s %s\n", mlp->ml_host, mlp->ml_dirp);
	(void)fclose(mlfile);
}

/*
 * This function is called via. SIGTERM when the system is going down.
 * It sends a broadcast RPCMNT_UMNTALL.
 */
/* ARGSUSED */
static void
send_umntall(n)
	int n;
{
	(void)clnt_broadcast(RPCPROG_MNT, RPCMNT_VER1, RPCMNT_UMNTALL,
	    xdr_void, NULL, xdr_void, NULL, umntall_each);
	exit(0);
}

static int
umntall_each(resultsp, raddr)
	caddr_t resultsp;
	struct sockaddr_in *raddr;
{
	return (1);
}

/*
 * Free up a group list.
 */
static void
free_grp(grp)
	struct grouplist *grp;
{
	char **addrp;

	if (grp->gr_type == GT_HOST) {
		if (grp->gr_ptr.gt_hostent->h_name) {
			addrp = grp->gr_ptr.gt_hostent->h_addr_list;
			if (addrp) {
				while (*addrp)
					free(*addrp++);
				free(grp->gr_ptr.gt_hostent->h_addr_list);
			}
			free(grp->gr_ptr.gt_hostent->h_name);
		}
		free(grp->gr_ptr.gt_hostent);
	} else if (grp->gr_type == GT_NET) {
		if (grp->gr_ptr.gt_net.nt_name)
			free(grp->gr_ptr.gt_net.nt_name);
	}
#ifdef ISO
	else if (grp->gr_type == GT_ISO)
		free(grp->gr_ptr.gt_isoaddr);
#endif
	free(grp);
}

#if 0
static void
SYSLOG(int pri, const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);

	if (debug)
		vfprintf(stderr, fmt, ap);
	else
		vsyslog(pri, fmt, ap);

	va_end(ap);
}
#endif

/*
 * Check options for consistency.
 */
static int
check_options(line, lineno, dp)
	const char *line;
	size_t lineno;
	struct dirlist *dp;
{

	if (dp == NULL) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: missing directory list",
		    line, (unsigned long)lineno);
		return (1);
	}
	if ((opt_flags & (OP_MAPROOT|OP_MAPALL)) == (OP_MAPROOT|OP_MAPALL) ||
	    (opt_flags & (OP_MAPROOT|OP_KERB)) == (OP_MAPROOT|OP_KERB) ||
	    (opt_flags & (OP_MAPALL|OP_KERB)) == (OP_MAPALL|OP_KERB)) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: -mapall, -maproot and -kerb mutually exclusive",
		    line, (unsigned long)lineno);
		return (1);
	}
	if ((opt_flags & OP_MASK) && (opt_flags & OP_NET) == 0) {
		syslog(LOG_ERR, "\"%s\", line %ld: -mask requires -net",
		    line, (unsigned long)lineno);
		return (1);
	}
	if ((opt_flags & (OP_NET|OP_ISO)) == (OP_NET|OP_ISO)) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: -net and -iso mutually exclusive",
		    line, (unsigned long)lineno);
		return (1);
	}
	if ((opt_flags & OP_ALLDIRS) && dp->dp_left) {
		syslog(LOG_ERR,
		    "\"%s\", line %ld: -alldir has multiple directories",
		    line, (unsigned long)lineno);
		return (1);
	}
	return (0);
}

/*
 * Check an absolute directory path for any symbolic links. Return true
 * if no symbolic links are found.
 */
static int
check_dirpath(line, lineno, dirp)
	const char *line;
	size_t lineno;
	char *dirp;
{
	char *cp;
	struct stat sb;
	char *file = "";

	for (cp = dirp + 1; *cp; cp++) {
		if (*cp == '/') {
			*cp = '\0';
			if (lstat(dirp, &sb) == -1)
				goto bad;
			if (!S_ISDIR(sb.st_mode))
				goto bad1;
			*cp = '/';
		}
	}

	cp = NULL;
	if (lstat(dirp, &sb) == -1)
		goto bad;

	if (!S_ISDIR(sb.st_mode) && !S_ISREG(sb.st_mode)) {
		file = " file or a";
		goto bad1;
	}

	return 1;

bad:
	syslog(LOG_ERR,
	    "\"%s\", line %ld: lstat for `%s' failed: %m",
	    line, (unsigned long)lineno, dirp);
	if (cp)
		*cp = '/';
	return 0;

bad1:
	syslog(LOG_ERR,
	    "\"%s\", line %ld: `%s' is not a%s directory",
	    line, (unsigned long)lineno, dirp, file);
	if (cp)
		*cp = '/';
	return 0;
}
