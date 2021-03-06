/*	$NetBSD: dumprmt.c,v 1.21 1998/07/30 18:14:00 thorpej Exp $	*/

/*-
 * Copyright (c) 1980, 1993
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
#if 0
static char sccsid[] = "@(#)dumprmt.c	8.3 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: dumprmt.c,v 1.21 1998/07/30 18:14:00 thorpej Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef sunos
#include <sys/vnode.h>

#include <ufs/inode.h>
#else
#include <ufs/ufs/dinode.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <protocols/dumprestore.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "pathnames.h"
#include "dump.h"

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
static	int rmtape;
static	char *rmtpeer;

static	int	okname __P((char *));
static	int	rmtcall __P((char *, char *));
	void	rmtclose __P((void));
static	void	rmtconnaborted __P((int));
static	int	rmtgetb __P((void));
static	void	rmtgetconn __P((void));
static	void	rmtgets __P((char *, int));
	int	rmthost __P((char *));
	int	rmtioctl __P((int, int));
	int	rmtopen __P((char *, int));
	int	rmtread __P((char *, int));
static	int	rmtreply __P((char *));
	int	rmtseek __P((int, int));
	int	rmtwrite __P((char *, int));

extern	int ntrec;		/* blocking factor on tape */

int
rmthost(host)
	char *host;
{

	rmtpeer = malloc(strlen(host) + 1);
	if (rmtpeer)
		strcpy(rmtpeer, host);
	else
		rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn();
	if (rmtape < 0)
		return (0);
	return (1);
}

static void
rmtconnaborted(dummy)
	int dummy;
{

	errx(1, "Lost connection to remote host.");
}

void
rmtgetconn()
{
	char *cp;
	static struct servent *sp = NULL;
	static struct passwd *pwd = NULL;
	char *tuser, *name;
	int size, opt;

	if (sp == NULL) {
		sp = getservbyname("shell", "tcp");
		if (sp == NULL)
			errx(1, "shell/tcp: unknown service");
		pwd = getpwuid(getuid());
		if (pwd == NULL)
			errx(1, "who are you?");
	}
	if ((name = strdup(pwd->pw_name)) == NULL)
		err(1, "malloc");
	if ((cp = strchr(rmtpeer, '@')) != NULL) {
		tuser = rmtpeer;
		*cp = '\0';
		if (!okname(tuser))
			exit(1);
		rmtpeer = ++cp;
	} else
		tuser = name;

	rmtape = rcmd(&rmtpeer, (u_short)sp->s_port, name, tuser, _PATH_RMT,
	    (int *)0);
	(void)free(name);
	if (rmtape < 0)
		return;

	size = ntrec * TP_BSIZE;
	if (size > 60 * 1024)		/* XXX */
		size = 60 * 1024;
	/* Leave some space for rmt request/response protocol */
	size += 2 * 1024;
	while (size > TP_BSIZE &&
	    setsockopt(rmtape, SOL_SOCKET, SO_SNDBUF, &size, sizeof (size)) < 0)
		    size -= TP_BSIZE;
	(void)setsockopt(rmtape, SOL_SOCKET, SO_RCVBUF, &size, sizeof (size));

	opt = IPTOS_THROUGHPUT;
	(void)setsockopt(rmtape, IPPROTO_IP, IP_TOS, &opt, sizeof (opt));

	opt = 1;
	(void)setsockopt(rmtape, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof (opt));
}

static int
okname(cp0)
	char *cp0;
{
	char *cp;
	int c;

	for (cp = cp0; *cp; cp++) {
		c = *cp;
		if (!isascii(c) || !(isalnum(c) || c == '_' || c == '-')) {
			warnx("invalid user name: %s", cp0);
			return (0);
		}
	}
	return (1);
}

int
rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	(void)snprintf(buf, sizeof buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}

void
rmtclose()
{

	if (rmtstate != TS_OPEN)
		return;
	rmtcall("close", "C\n");
	rmtstate = TS_CLOSED;
}

int
rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;

	(void)snprintf(line, sizeof line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		errno = n;
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted(0);
		}
	}
	return (n);
}

int
rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	(void)snprintf(line, sizeof line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}

#if 0		/* XXX unused? */
void
rmtwrite0(count)
	int count;
{
	char line[30];

	(void)snprintf(line, sizeof line, "W%d\n", count);
	write(rmtape, line, strlen(line));
}

void
rmtwrite1(buf, count)
	char *buf;
	int count;
{

	write(rmtape, buf, count);
}

int
rmtwrite2()
{

	return (rmtreply("write"));
}
#endif

int
rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	(void)snprintf(line, sizeof line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}


#if 0		/* XXX unused? */
struct mtget *
rmtstatus()
{
	struct	mtget mts;
	int i;
	char *cp;

	if (rmtstate != TS_OPEN)
		return (NULL);
	rmtcall("status", "S\n");
	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = rmtgetb();
	return (&mts);
}
#endif

int
rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	(void)snprintf(buf, sizeof buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

static int
rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted(0);
	return (rmtreply(cmd));
}

static int
rmtreply(cmd)
	char *cmd;
{
	char *cp;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		msg("%s: %s", cmd, emsg);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		/* Kill trailing newline */
		cp = code + strlen(code);
		if (cp > code && *--cp == '\n')
			*cp = '\0';

		msg("Protocol to remote tape server botched (code \"%s\").\n",
		    code);
		rmtconnaborted(0);
	}
	return (atoi(code + 1));
}

int
rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1)
		rmtconnaborted(0);
	return (c);
}

/* Get a line (guaranteed to have a trailing newline). */
void
rmtgets(line, len)
	char *line;
	int len;
{
	char *cp = line;

	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = '\0';
			return;
		}
		cp++;
		len--;
	}
	*cp = '\0';
	msg("Protocol to remote tape server botched.\n");
	msg("(rmtgets got \"%s\").\n", line);
	rmtconnaborted(0);
}
