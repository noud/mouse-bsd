/*	$NetBSD: execvp.c,v 1.16 2000/01/22 22:19:09 mycroft Exp $	*/

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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)exec.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: execvp.c,v 1.16 2000/01/22 22:19:09 mycroft Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <paths.h>
#include "reentrant.h"

#ifdef __weak_alias
__weak_alias(execvp,_execvp)
#endif

extern char **environ;
#ifdef _REENT
extern rwlock_t __environ_lock;
#endif

int
execvp(name, argv)
	const char *name;
	char * const *argv;
{
	const char **memp;
	int cnt;
	size_t lp, ln;
	char *p;
	int eacces = 0;
	unsigned int etxtbsy = 0;
	char *cur, *path, buf[PATH_MAX];
	const char *bp;

	_DIAGASSERT(name != NULL);

	/* "" is not a valid filename; check this before traversing PATH. */
	if (name[0] == '\0') {
		errno = ENOENT;
		path = NULL;
		goto done;
	}
	/* If it's an absolute or relative path name, it's easy. */
	if (strchr(name, '/')) {
		bp = name;
		cur = path = NULL;
		goto retry;
	}
	bp = buf;

	/* Get the path we're searching. */
	if (!(path = getenv("PATH")))
		path = _PATH_DEFPATH;
	cur = path = strdup(path);

	while ((p = strsep(&cur, ":")) != NULL) {
		/*
		 * It's a SHELL path -- double, leading and trailing colons
		 * mean the current directory.
		 */
		if (!*p) {
			p = ".";
			lp = 1;
		} else
			lp = strlen(p);
		ln = strlen(name);

		/*
		 * If the path is too long complain.  This is a possible
		 * security issue; given a way to make the path too long
		 * the user may execute the wrong program.
		 */
		if (lp + ln + 2 > sizeof(buf)) {
			(void)write(STDERR_FILENO, "execvp: ", 8);
			(void)write(STDERR_FILENO, p, lp);
			(void)write(STDERR_FILENO, ": path too long\n", 16);
			continue;
		}
		memmove(buf, p, lp);
		buf[lp] = '/';
		memmove(buf + lp + 1, name, ln);
		buf[lp + ln + 1] = '\0';

retry:		rwlock_rdlock(&__environ_lock);
		(void)execve(bp, argv, environ);
		rwlock_unlock(&__environ_lock);
		switch (errno) {
		case EACCES:
			eacces = 1;
			break;
		case ENOENT:
			break;
		case ENOEXEC:
			for (cnt = 0; argv[cnt] != NULL; ++cnt);
			if ((memp = malloc((cnt + 2) * sizeof (*memp))) == NULL)
				goto done;
			memp[0] = _PATH_BSHELL;
			memp[1] = bp;
			(void)memmove(&memp[2], &argv[1], cnt * sizeof (*memp));
			rwlock_rdlock(&__environ_lock);
			(void)execve(_PATH_BSHELL, (char * const *)memp, environ);
			rwlock_unlock(&__environ_lock);
			free(memp);
			goto done;
		case ETXTBSY:
			if (etxtbsy < 3)
				(void)sleep(++etxtbsy);
			goto retry;
		default:
			goto done;
		}
	}
	if (eacces)
		errno = EACCES;
	else if (!errno)
		errno = ENOENT;
done:	if (path)
		free(path);
	return (-1);
}
