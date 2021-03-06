/*	$NetBSD: ln.c,v 1.16 1999/09/05 23:34:40 hubertf Exp $	*/

/*
 * Copyright (c) 1987, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ln.c	8.2 (Berkeley) 3/31/94";
#else
__RCSID("$NetBSD: ln.c,v 1.16 1999/09/05 23:34:40 hubertf Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	fflag;				/* Unlink existing files. */
int	hflag;				/* Check new name for symlink first. */
int	sflag;				/* Symbolic, not hard, link. */
					/* System link call. */
int (*linkf) __P((const char *, const char *));

int	linkit __P((char *, char *, int));
void	usage __P((void));
int	main __P((int, char *[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat sb;
	int ch, exitval;
	char *sourcedir;

	while ((ch = getopt(argc, argv, "fhns")) != -1)
		switch (ch) {
		case 'f':
			fflag = 1;
			break;
		case 'h':
		case 'n':
			hflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	linkf = sflag ? symlink : link;

	switch(argc) {
	case 0:
		usage();
		/* NOTREACHED */
	case 1:				/* ln target */
		exit(linkit(argv[0], ".", 1));
		/* NOTREACHED */
	case 2:				/* ln target source */
		exit(linkit(argv[0], argv[1], 0));
		/* NOTREACHED */
	}
					/* ln target1 target2 directory */
	sourcedir = argv[argc - 1];
	if (hflag && lstat(sourcedir, &sb) == 0 && S_ISLNK(sb.st_mode)) {
		/* we were asked not to follow symlinks, but found one at
		   the target--simulate "not a directory" error */
		errno = ENOTDIR;
		err(1, "%s", sourcedir);
	}
	if (stat(sourcedir, &sb))
		err(1, "%s", sourcedir);
	if (!S_ISDIR(sb.st_mode))
		usage();
	for (exitval = 0; *argv != sourcedir; ++argv)
		exitval |= linkit(*argv, sourcedir, 1);
	exit(exitval);
	/* NOTREACHED */
}

int
linkit(target, source, isdir)
	char *target, *source;
	int isdir;
{
	struct stat sb;
	char *p, path[MAXPATHLEN];

	if (!sflag) {
		/* If target doesn't exist, quit now. */
		if (lstat(target, &sb)) {
			warn("%s", target);
			return (1);
		}
	}

	/* If the source is a directory (and not a symlink if hflag),
	   append the target's name. */
	if (isdir ||
	    (!lstat(source, &sb) && S_ISDIR(sb.st_mode)) ||
	    (!hflag && !stat(source, &sb) && S_ISDIR(sb.st_mode))) {
		if ((p = strrchr(target, '/')) == NULL)
			p = target;
		else
			++p;
		(void)snprintf(path, sizeof(path), "%s/%s", source, p);
		source = path;
	}

	/*
	 * If the file exists, and -f was specified, unlink it.
	 * Attempt the link.
	 */
	if ((fflag && unlink(source) < 0 && errno != ENOENT) ||
	    (*linkf)(target, source)) {
		warn("%s", source);
		return (1);
	}

	return (0);
}

void
usage()
{

	extern char *__progname;
	(void)fprintf(stderr,
	    "Usage:\t%s [-fhns] file1 file2\n\t%s [-fhns] file ... directory\n",
	    __progname, __progname);
	exit(1);
	/* NOTREACHED */
}
