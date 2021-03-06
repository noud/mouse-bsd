/*	$NetBSD: passwd.c,v 1.19 1999/12/03 16:23:58 mjl Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994, 1995
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: passwd.c,v 1.19 1999/12/03 16:23:58 mjl Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

static void	pw_cont __P((int sig));
static int	pw_equal __P((char *buf, struct passwd *old_pw));

int
pw_lock(retries)
	int retries;
{
	int i, fd;
	mode_t old_mode;
	int oerrno;

	/* Acquire the lock file. */
	old_mode = umask(0);
	fd = open(_PATH_MASTERPASSWD_LOCK, O_WRONLY|O_CREAT|O_EXCL, 0600);
	for (i = 0; i < retries && fd < 0 && errno == EEXIST; i++) {
		sleep(1);
		fd = open(_PATH_MASTERPASSWD_LOCK, O_WRONLY|O_CREAT|O_EXCL,
			  0600);
	}
	oerrno = errno;
	(void)umask(old_mode);
	errno = oerrno;
	return(fd);
}

int
pw_mkdb()
{
	int pstat;
	pid_t pid;

	pid = vfork();
	if (pid == -1)
		return (-1);

	if (pid == 0) {
		execl(_PATH_PWD_MKDB, "pwd_mkdb", "-p",
		      _PATH_MASTERPASSWD_LOCK, NULL);
		_exit(1);
	}
	pid = waitpid(pid, &pstat, 0);
	if (pid == -1 || !WIFEXITED(pstat) || WEXITSTATUS(pstat) != 0)
		return(-1);
	return(0);
}

int
pw_abort()
{
	return(unlink(_PATH_MASTERPASSWD_LOCK));
}

/* Everything below this point is intended for the convenience of programs
 * which allow a user to interactively edit the passwd file.  Errors in the
 * routines below will cause the process to abort. */

static pid_t editpid = -1;

static void
pw_cont(sig)
	int sig;
{

	if (editpid != -1)
		kill(editpid, sig);
}

void
pw_init()
{
	struct rlimit rlim;

	/* Unlimited resource limits. */
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void)setrlimit(RLIMIT_CPU, &rlim);
	(void)setrlimit(RLIMIT_FSIZE, &rlim);
	(void)setrlimit(RLIMIT_STACK, &rlim);
	(void)setrlimit(RLIMIT_DATA, &rlim);
	(void)setrlimit(RLIMIT_RSS, &rlim);

	/* Don't drop core (not really necessary, but GP's). */
	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

	/* Turn off signals. */
	(void)signal(SIGALRM, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);
	(void)signal(SIGCONT, pw_cont);
}

void
pw_edit(notsetuid, filename)
	int notsetuid;
	const char *filename;
{
	int pstat;
	char *p, *editor;
	char *argp[] = { "sh", "-c", NULL, NULL };

#ifdef __GNUC__
	(void) &editor;
#endif

	if (filename == NULL)
		filename = _PATH_MASTERPASSWD_LOCK;

	if ((editor = getenv("EDITOR")) == NULL)
		editor = _PATH_VI;

	p = malloc(strlen(editor) + 1 + strlen(filename) + 1);
	if (p == NULL)
		return;

	sprintf(p, "%s %s", editor, filename);
	argp[2] = p;

	switch(editpid = vfork()) {
	case -1:
		free(p);
		return;
	case 0:
		if (notsetuid) {
			setgid(getgid());
			setuid(getuid());
		}
		execvp(_PATH_BSHELL, argp);
		_exit(1);
	}

	free(p);

	for (;;) {
		editpid = waitpid(editpid, (int *)&pstat, WUNTRACED);
		if (editpid == -1)
			pw_error(editor, 1, 1);
		else if (WIFSTOPPED(pstat))
			raise(WSTOPSIG(pstat));
		else if (WIFEXITED(pstat) && WEXITSTATUS(pstat) == 0)
			break;
		else
			pw_error(editor, 1, 1);
	}
	editpid = -1;
}

void
pw_prompt()
{
	int c;

	(void)printf("re-edit the password file? [y]: ");
	(void)fflush(stdout);
	c = getchar();
	if (c != EOF && c != '\n')
		while (getchar() != '\n');
	if (c == 'n')
		pw_error(NULL, 0, 0);
}

/* for use in pw_copy(). Compare a pw entry to a pw struct. */
static int
pw_equal (buf, pw)
	char *buf;
	struct passwd *pw;
{
	struct passwd buf_pw;
	int len;

	_DIAGASSERT(buf != NULL);
	_DIAGASSERT(pw != NULL);

	len = strlen (buf);
	if (buf[len-1] == '\n')
		buf[len-1] = '\0';
	if (!pw_scan(buf, &buf_pw, NULL))
		return 0;
	return !strcmp(pw->pw_name, buf_pw.pw_name)
		&& pw->pw_uid == buf_pw.pw_uid
		&& pw->pw_gid == buf_pw.pw_gid
		&& !strcmp(pw->pw_class, buf_pw.pw_class)
		&& (long)pw->pw_change == (long)buf_pw.pw_change
		&& (long)pw->pw_expire == (long)buf_pw.pw_expire
		&& !strcmp(pw->pw_gecos, buf_pw.pw_gecos)
		&& !strcmp(pw->pw_dir, buf_pw.pw_dir)
		&& !strcmp(pw->pw_shell, buf_pw.pw_shell);
}

void
pw_copy(ffd, tfd, pw, old_pw)
	int ffd, tfd;
	struct passwd *pw, *old_pw;
{
	FILE *from, *to;
	int done;
	char *p, buf[8192];

	_DIAGASSERT(pw != NULL);
	/* old_pw may be NULL */

	if (!(from = fdopen(ffd, "r")))
		pw_error(_PATH_MASTERPASSWD, 1, 1);
	if (!(to = fdopen(tfd, "w")))
		pw_error(_PATH_MASTERPASSWD_LOCK, 1, 1);

	for (done = 0; fgets(buf, sizeof(buf), from);) {
		if (!strchr(buf, '\n')) {
			warnx("%s: line too long", _PATH_MASTERPASSWD);
			pw_error(NULL, 0, 1);
		}
		if (done) {
			(void)fprintf(to, "%s", buf);
			if (ferror(to))
				goto err;
			continue;
		}
		if (!(p = strchr(buf, ':'))) {
			warnx("%s: corrupted entry", _PATH_MASTERPASSWD);
			pw_error(NULL, 0, 1);
		}
		*p = '\0';
		if (strcmp(buf, pw->pw_name)) {
			*p = ':';
			(void)fprintf(to, "%s", buf);
			if (ferror(to))
				goto err;
			continue;
		}
		*p = ':';
		if (old_pw && !pw_equal(buf, old_pw)) {
			warnx("%s: entry inconsistent",
			      _PATH_MASTERPASSWD);
			pw_error(NULL, 0, 1);
		}
		(void)fprintf(to, "%s:%s:%d:%d:%s:%ld:%ld:%s:%s:%s\n",
		    pw->pw_name, pw->pw_passwd, pw->pw_uid, pw->pw_gid,
		    pw->pw_class, (long)pw->pw_change, (long)pw->pw_expire,
		    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
		done = 1;
		if (ferror(to))
			goto err;
	}
	/* Only append a new entry if real uid is root! */
	if (!done) {
		if (getuid() == 0)
			(void)fprintf(to, "%s:%s:%d:%d:%s:%ld:%ld:%s:%s:%s\n",
			    pw->pw_name, pw->pw_passwd, pw->pw_uid, pw->pw_gid,
			    pw->pw_class, (long)pw->pw_change,
			    (long)pw->pw_expire, pw->pw_gecos, pw->pw_dir,
			    pw->pw_shell);
		else
			warnx("%s: changes not made, no such entry",
			    _PATH_MASTERPASSWD);
	}

	if (ferror(to))
err:		pw_error(NULL, 1, 1);
	(void)fclose(to);
}

void
pw_error(name, err, eval)
	const char *name;
	int err, eval;
{

	if (err)
		warn(name);

	warnx("%s: unchanged", _PATH_MASTERPASSWD);
	pw_abort();
	exit(eval);
}
