/*	$NetBSD: kdump.c,v 1.25 1999/12/31 22:27:59 eeh Exp $	*/

/*-
 * Copyright (c) 1988, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)kdump.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: kdump.c,v 1.25 1999/12/31 22:27:59 eeh Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#define _KERNEL
#include <sys/errno.h>
#undef _KERNEL
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>
#include <ctype.h>

#include "ktrace.h"

int timestamp, decimal, fancy = 1, tail, maxdata, hexdata;
char *tracefile = DEF_TRACEFILE;
struct ktr_header ktr_header;

#define eqs(s1, s2)	(strcmp((s1), (s2)) == 0)

#include <sys/syscall.h>

#include "../../sys/compat/netbsd32/netbsd32_syscall.h"
#include "../../sys/compat/freebsd/freebsd_syscall.h"
#include "../../sys/compat/hpux/hpux_syscall.h"
#include "../../sys/compat/ibcs2/ibcs2_syscall.h"
#include "../../sys/compat/linux/linux_syscall.h"
#include "../../sys/compat/osf1/osf1_syscall.h"
#include "../../sys/compat/sunos/sunos_syscall.h"
#include "../../sys/compat/svr4/svr4_syscall.h"
#include "../../sys/compat/ultrix/ultrix_syscall.h"

#define KTRACE
#include "../../sys/kern/syscalls.c"

#include "../../sys/compat/netbsd32/netbsd32_syscalls.c"
#include "../../sys/compat/freebsd/freebsd_syscalls.c"
#include "../../sys/compat/hpux/hpux_syscalls.c"
#include "../../sys/compat/ibcs2/ibcs2_syscalls.c"
#include "../../sys/compat/linux/linux_syscalls.c"
#include "../../sys/compat/osf1/osf1_syscalls.c"
#include "../../sys/compat/sunos/sunos_syscalls.c"
#include "../../sys/compat/svr4/svr4_syscalls.c"
#include "../../sys/compat/ultrix/ultrix_syscalls.c"

#include "../../sys/compat/hpux/hpux_errno.c"
#include "../../sys/compat/svr4/svr4_errno.c"
#include "../../sys/compat/ibcs2/ibcs2_errno.c"
#include "../../sys/compat/linux/common/linux_errno.c"
#undef KTRACE

struct emulation {
	char *name;		/* Emulation name */
	char **sysnames;	/* Array of system call names */
	int  nsysnames;		/* Number of */
	int  *errno;		/* Array of error number mapping */
	int  nerrno;		/* number of elements in array */
};

#define NELEM(a) (sizeof(a) / sizeof(a[0]))

static struct emulation emulations[] = {
	{   "netbsd",	       syscallnames,         SYS_MAXSYSCALL,
	        NULL,		        0 },
	{   "netbsd32", netbsd32_syscallnames,	     SYS_MAXSYSCALL,
	        NULL,		        0 },
	{  "freebsd",  freebsd_syscallnames, FREEBSD_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{     "hpux",	  hpux_syscallnames,    HPUX_SYS_MAXSYSCALL,
	  native_to_hpux_errno,  NELEM(native_to_hpux_errno)  },
	{    "ibcs2",    ibcs2_syscallnames,   IBCS2_SYS_MAXSYSCALL,
	 native_to_ibcs2_errno,  NELEM(native_to_ibcs2_errno) },
	{    "linux",    linux_syscallnames,   LINUX_SYS_MAXSYSCALL,
	 native_to_linux_errno,  NELEM(native_to_linux_errno) },
	{     "osf1",     osf1_syscallnames,    OSF1_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{    "sunos",    sunos_syscallnames,   SUNOS_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{     "svr4",     svr4_syscallnames,    SVR4_SYS_MAXSYSCALL,
	  native_to_svr4_errno,  NELEM(native_to_svr4_errno)  },
	{   "ultrix",   ultrix_syscallnames,  ULTRIX_SYS_MAXSYSCALL,
	        NULL,			0 },
	{       NULL,		       NULL,		          0,
	        NULL,			0 }
};

struct emulation *current;

static const char *ptrace_ops[] = {
	"PT_TRACE_ME",	"PT_READ_I",	"PT_READ_D",	"PT_READ_U",
	"PT_WRITE_I",	"PT_WRITE_D",	"PT_WRITE_U",	"PT_CONTINUE",
	"PT_KILL",	"PT_ATTACH",	"PT_DETACH",
};

int	main __P((int, char **));
int	fread_tail __P((char *, int, int));
void	dumpheader __P((struct ktr_header *));
void	ioctldecode __P((u_long));
void	ktrsyscall __P((struct ktr_syscall *));
void	ktrsysret __P((struct ktr_sysret *));
void	ktrnamei __P((char *, int));
void	ktremul __P((char *, int));
void	ktrgenio __P((struct ktr_genio *, int));
void	ktrpsig __P((struct ktr_psig *));
void	ktrcsw __P((struct ktr_csw *));
void	usage __P((void));
void	setemul __P((char *));
void	eprint __P((int));
char	*ioctlname __P((long));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, ktrlen, size;
	void *m;
	int trpoints = ALL_POINTS;
	int *pidlist;
	int pidn;
	int printit;

	current = &emulations[0];	/* NetBSD */
	pidlist = 0;
	pidn = 0;

	while ((ch = getopt(argc, argv, "e:f:dlm:np:RTt:x")) != -1)
		switch (ch) {
		case 'e':
			setemul(optarg);
			break;
		case 'f':
			tracefile = optarg;
			break;
		case 'd':
			decimal = 1;
			break;
		case 'l':
			tail = 1;
			break;
		case 'm':
			maxdata = atoi(optarg);
			break;
		case 'n':
			fancy = 0;
			break;
		case 'p':
			pidlist = realloc(pidlist,(pidn+1)*sizeof(int));
			pidlist[pidn++] = atoi(optarg);
			break;
		case 'R':
			timestamp = 2;	/* relative timestamp */
			break;
		case 'T':
			timestamp = 1;
			break;
		case 't':
			trpoints = getpoints(optarg);
			if (trpoints < 0)
				errx(1, "unknown trace point in %s", optarg);
			break;
		case 'x':
			hexdata = 1;
			break;
		default:
			usage();
		}
	argv += optind;
	argc -= optind;

	if (argc > 1)
		usage();

	m = malloc(size = 1025);
	if (m == NULL)
		errx(1, "%s", strerror(ENOMEM));
	if (!freopen(tracefile, "r", stdin))
		err(1, "%s", tracefile);
	printit = 1;
	while (fread_tail((char *)&ktr_header, sizeof(struct ktr_header), 1)) {
		if (pidn) {
			int i;
			printit = 0;
			for ( i = pidn - 1;
			      (i >= 0) &&
				(ktr_header.ktr_pid != pidlist[i]);
			      i -- ) ;
			if (i >= 0)
				printit = 1;
		}
		if (printit && (trpoints & (1<<ktr_header.ktr_type)))
			dumpheader(&ktr_header);
		if ((ktrlen = ktr_header.ktr_len) < 0)
			errx(1, "bogus length 0x%x", ktrlen);
		if (ktrlen > size) {
			m = (void *)realloc(m, ktrlen+1);
			if (m == NULL)
				errx(1, "%s", strerror(ENOMEM));
			size = ktrlen;
		}
		if (ktrlen && fread_tail(m, ktrlen, 1) == 0)
			errx(1, "data too short");
		if ( !printit ||
		     !(trpoints & (1<<ktr_header.ktr_type)) )
			continue;
		switch (ktr_header.ktr_type) {
		case KTR_SYSCALL:
			ktrsyscall((struct ktr_syscall *)m);
			break;
		case KTR_SYSRET:
			ktrsysret((struct ktr_sysret *)m);
			break;
		case KTR_NAMEI:
			ktrnamei(m, ktrlen);
			break;
		case KTR_GENIO:
			ktrgenio((struct ktr_genio *)m, ktrlen);
			break;
		case KTR_PSIG:
			ktrpsig((struct ktr_psig *)m);
			break;
		case KTR_CSW:
			ktrcsw((struct ktr_csw *)m);
			break;
		case KTR_EMUL:
			ktremul(m, ktrlen);
			break;
		}
		if (tail)
			(void)fflush(stdout);
	}
	return (0);
}

int
fread_tail(buf, size, num)
	char *buf;
	int num, size;
{
	int i;

	while ((i = fread(buf, size, num, stdin)) == 0 && tail) {
		(void)sleep(1);
		clearerr(stdin);
	}
	return (i);
}

void
dumpheader(kth)
	struct ktr_header *kth;
{
	char unknown[64], *type;
	static struct timeval prevtime;
	struct timeval temp;

	switch (kth->ktr_type) {
	case KTR_SYSCALL:
		type = "CALL";
		break;
	case KTR_SYSRET:
		type = "RET ";
		break;
	case KTR_NAMEI:
		type = "NAMI";
		break;
	case KTR_GENIO:
		type = "GIO ";
		break;
	case KTR_PSIG:
		type = "PSIG";
		break;
	case KTR_CSW:
		type = "CSW";
		break;
	case KTR_EMUL:
		type = "EMUL";
		break;
	default:
		(void)sprintf(unknown, "UNKNOWN(%d)", kth->ktr_type);
		type = unknown;
	}

	(void)printf("%6d %-8.*s ", kth->ktr_pid, MAXCOMLEN, kth->ktr_comm);
	if (timestamp) {
		if (timestamp == 2) {
			timersub(&kth->ktr_time, &prevtime, &temp);
			prevtime = kth->ktr_time;
		} else
			temp = kth->ktr_time;
		(void)printf("%lld.%06ld ", temp.tv_sec, temp.tv_usec);
	}
	(void)printf("%s  ", type);
}

void
ioctldecode(cmd)
	u_long cmd;
{
	char dirbuf[4], *dir = dirbuf;

	if (cmd & IOC_IN)
		*dir++ = 'W';
	if (cmd & IOC_OUT)
		*dir++ = 'R';
	*dir = '\0';

	/* XXX "cmd & 0xff" should be some #define from <sys/ioccom.h> */
	printf(decimal ? ",_IO%s('%c',%ld" : ",_IO%s('%c',%#lx",
	    dirbuf, (int)IOCGROUP(cmd), cmd & 0xff);
	if ((cmd & IOC_VOID) == 0)
		printf(decimal ? ",%ld)" : ",%#lx)", IOCPARM_LEN(cmd));
	else
		printf(")");
}

void
ktrsyscall(ktr)
	struct ktr_syscall *ktr;
{
	int argsize = ktr->ktr_argsize;
	register_t *ap;

	if (ktr->ktr_code >= current->nsysnames || ktr->ktr_code < 0)
		(void)printf("[%d]", ktr->ktr_code);
	else
		(void)printf("%s", current->sysnames[ktr->ktr_code]);
	ap = (register_t *)((char *)ktr + sizeof(struct ktr_syscall));
	if (argsize) {
		char c = '(';
		if (fancy) {
			if (ktr->ktr_code == SYS_ioctl) {
				char *cp;
				if (decimal)
					(void)printf("(%ld", (long)*ap);
				else
					(void)printf("(%#lx", (long)*ap);
				ap++;
				argsize -= sizeof(register_t);
				if ((cp = ioctlname(*ap)) != NULL)
					(void)printf(",%s", cp);
				else
					ioctldecode(*ap);
				c = ',';
				ap++;
				argsize -= sizeof(register_t);
			} else if (ktr->ktr_code == SYS_ptrace) {
				if (*ap >= 0 && *ap <=
				    sizeof(ptrace_ops) / sizeof(ptrace_ops[0]))
					(void)printf("(%s", ptrace_ops[*ap]);
				else
					(void)printf("(%ld", (long)*ap);
				c = ',';
				ap++;
				argsize -= sizeof(register_t);
			}
		}
		while (argsize) {
			if (decimal)
				(void)printf("%c%ld", c, (long)*ap);
			else
				(void)printf("%c%#lx", c, (long)*ap);
			c = ',';
			ap++;
			argsize -= sizeof(register_t);
		}
		(void)putchar(')');
	}
	(void)putchar('\n');
}

void
ktrsysret(ktr)
	struct ktr_sysret *ktr;
{
	register_t ret = ktr->ktr_retval;
	int error = ktr->ktr_error;
	int code = ktr->ktr_code;

	if (code >= current->nsysnames || code < 0)
		(void)printf("[%d] ", code);
	else
		(void)printf("%s ", current->sysnames[code]);

	switch (error) {
	case 0:
		if (fancy) {
			(void)printf("%ld", (long)ret);
			if (ret < 0 || ret > 9)
				(void)printf("/%#lx", (long)ret);
		} else {
			if (decimal)
				(void)printf("%ld", (long)ret);
			else
				(void)printf("%#lx", (long)ret);
		}
		break;

	default:
		eprint(error);
		break;
	}
	(void)putchar('\n');

}

/*
 * We print the original emulation's error numerically, but we
 * translate it to netbsd to print it symbolically.
 */
void
eprint(e)
	int e;
{
	int i = e;

	if (current->errno) {

		/* No remapping for ERESTART and EJUSTRETURN */
		/* Kludge for linux that has negative error numbers */
		if (current->errno[2] > 0 && e < 0)
			goto normal;

		for (i = 0; i < current->nerrno; i++)
			if (e == current->errno[i])
				break;

		if (i == current->nerrno) {
			printf("-1 unknown errno %d", e);
			return;
		}
	}

normal:
	switch (i) {
	case ERESTART:
		(void)printf("RESTART");
		break;

	case EJUSTRETURN:
		(void)printf("JUSTRETURN");
		break;

	default:
		(void)printf("-1 errno %d", e);
		if (fancy)
			(void)printf(" %s", strerror(i));
	}
}

void
ktrnamei(cp, len)
	char *cp;
	int len;
{

	(void)printf("\"%.*s\"\n", len, cp);
}

void
ktremul(cp, len)
	char *cp;
	int len;
{
	char name[1024];

	if (len >= sizeof(name))
		errx(1, "Emulation name too long");

	strncpy(name, cp, len);
	name[len] = '\0';
	(void)printf("\"%s\"\n", name);

	setemul(name);
}

static void print_data_text(const void *data, int len, int maxlen, int screenwidth)
{
 int n;
 const char *dp;
 char visbuf[5];
 char *cp;
 int width;
 int col;

 n = (maxlen && (len > maxlen)) ? maxlen : len;
 printf("       \"");
 col = 8;
 for (dp=data;n>0;n--,dp++)
  { vis(&visbuf[0],*dp,VIS_CSTYLE,(n>1)?dp[1]:'\0');
    cp = &visbuf[0];
    if (col == 0)
     { putchar('\t');
       col = 8;
     }
    switch (*cp)
     { case '\n':
	  col = 0;
	  putchar('\n');
	  continue;
	  break;
       case '\t':
	  width = 8 - (col & 7);
	  break;
       default:
	  width = strlen(cp);
     }
    if (col+width > (screenwidth-2))
     { printf("\\\n\t");
       col = 8;
     }
    col += width;
    do { putchar(*cp++); } while (*cp);
  }
 if (col == 0) printf("       ");
 printf("\"");
 if (maxlen && (len > maxlen)) printf("...");
 printf("\n");
}

static void print_data_hex(const void *data, int len, int maxlen, int screenwidth)
{
 int npl;
 int l0;
 int i;
 int n;
 const unsigned char *dp;

 n = (maxlen && (len > maxlen)) ? maxlen : len;
 npl = (screenwidth - 8 - 1 - 6) / 4;
 if (npl < 1) npl = 1;
 l0 = 0;
 while (l0 < n)
  { dp = ((const unsigned char *)data) + l0;
    printf("\t");
    for (i=0;(i<npl)&&(l0+i<n);i++) printf(" %02x",dp[i]);
    printf("%s",((n<len)&&(l0+i==n))?"...":"   ");
    for (;i<npl;i++) printf("   ");
    printf(" |");
    for (i=0;(i<npl)&&(l0+i<n);i++) putchar(isprint(dp[i])?dp[i]:'.');
    printf("|\n");
    l0 += i;
  }
}

void
ktrgenio(ktr, len)
	struct ktr_genio *ktr;
	int len;
{
	int datalen = len - sizeof (struct ktr_genio);
	char *dp = (char *)ktr + sizeof (struct ktr_genio);
	static int screenwidth = 0;

	if (screenwidth == 0) {
		struct winsize ws;

		if (fancy && ioctl(fileno(stderr), TIOCGWINSZ, &ws) != -1 &&
		    ws.ws_col > 8)
			screenwidth = ws.ws_col;
		else
			screenwidth = 80;
	}
	printf("fd %d %s %d bytes\n", ktr->ktr_fd,
		ktr->ktr_rw == UIO_READ ? "read" : "wrote", datalen);
	if (hexdata)
		print_data_hex(dp,datalen,maxdata,screenwidth);
	else
		print_data_text(dp,datalen,maxdata,screenwidth);
}

void
ktrpsig(psig)
	struct ktr_psig *psig;
{
	int signo, first;

	(void)printf("SIG%s ", sys_signame[psig->signo]);
	if (psig->action == SIG_DFL)
		(void)printf("SIG_DFL\n");
	else {
		(void)printf("caught handler=0x%lx mask=(",
		    (u_long)psig->action);
		first = 1;
		for (signo = 1; signo < NSIG; signo++) {
			if (sigismember(&psig->mask, signo)) {
				if (first)
					first = 0;
				else
					(void)printf(",");
				(void)printf("%d", signo);
			}
		}
		(void)printf(") code=0x%x\n", psig->code);
	}
}

void
ktrcsw(cs)
	struct ktr_csw *cs;
{

	(void)printf("%s %s\n", cs->out ? "stop" : "resume",
	    cs->user ? "user" : "kernel");
}

void
usage()
{

	(void)fprintf(stderr,
"usage: kdump [-dnlRTx] [-e emulation] [-f trfile] [-m maxdata] [-t [cnis]]\n");
	exit(1);
}

void
setemul(name)
	char *name;
{
	int i;

	for (i = 0; emulations[i].name != NULL; i++)
		if (strcmp(emulations[i].name, name) == 0) {
			current = &emulations[i];
			return;
		}
	warnx("Emulation `%s' unknown", name);
}
