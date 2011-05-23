/*	$NetBSD: util.c,v 1.11 2000/01/20 00:08:08 thorpej Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
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
 *	from: @(#)util.c	8.1 (Berkeley) 6/6/93
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/types.h>
#include "config.h"

static void nomem __P((void));
static void vxerror __P((const char *, int, const char *, va_list));
static void vxwarn __P((const char *, int, const char *, va_list));
static void vxmsg __P((const char *fname, int line, const char *class, 
		       const char *fmt, va_list));

/*
 * Malloc, with abort on error.
 */
void *
emalloc(size)
	size_t size;
{
	void *p;

	if ((p = malloc(size)) == NULL)
		nomem();
	return (p);
}

/*
 * Realloc, with abort on error.
 */
void *
erealloc(p, size)
	void *p;
	size_t size;
{

	if ((p = realloc(p, size)) == NULL)
		nomem();
	return (p);
}

/*
 * Strdup, with abort on error.
 */
char *
estrdup(p)
	const char *p;
{
	char *cp;

	if ((cp = strdup(p)) == NULL)
		nomem();
	return (cp);
}

static void
nomem()
{

	(void)fprintf(stderr, "config: out of memory\n");
	exit(1);
}

/*
 * Push a prefix onto the prefix stack.
 */
void
prefix_push(path)
	const char *path;
{
	struct prefix *pf;
	char *cp;

	pf = emalloc(sizeof(struct prefix));

	if (prefixes != NULL && *path != '/') {
		cp = emalloc(strlen(prefixes->pf_prefix) + 1 +
		    strlen(path) + 1);
		(void) sprintf(cp, "%s/%s", prefixes->pf_prefix, path);
		pf->pf_prefix = intern(cp);
		free(cp);
	} else
		pf->pf_prefix = intern(path);

	pf->pf_next = prefixes;
	prefixes = pf;
}

/*
 * Pop a prefix off the prefix stack.
 */
void
prefix_pop()
{
	struct prefix *pf;

	if ((pf = prefixes) == NULL) {
		error("no prefixes on the stack to pop");
		return;
	}

	prefixes = pf->pf_next;

	/* Remember this prefix for emitting -I... directives later. */
	pf->pf_next = allprefixes;
	allprefixes = pf;
}

/*
 * Prepend the source path to a file name.
 */
char *
sourcepath(file)
	const char *file;
{
	size_t len;
	char *cp;

	if (prefixes != NULL && *prefixes->pf_prefix == '/')
		len = strlen(prefixes->pf_prefix) + 1 + strlen(file) + 1;
	else {
		len = strlen(srcdir) + 1 + strlen(file) + 1;
		if (prefixes != NULL)
			len += strlen(prefixes->pf_prefix) + 1;
	}

	cp = emalloc(len);

	if (prefixes != NULL) {
		if (*prefixes->pf_prefix == '/')
			(void) sprintf(cp, "%s/%s", prefixes->pf_prefix, file);
		else
			(void) sprintf(cp, "%s/%s/%s", srcdir,
			    prefixes->pf_prefix, file);
	} else
		(void) sprintf(cp, "%s/%s", srcdir, file);
	return (cp);
}

static struct nvlist *nvhead;

struct nvlist *
newnv(name, str, ptr, i, next)
	const char *name, *str;
	void *ptr;
	int i;
	struct nvlist *next;
{
	struct nvlist *nv;

	if ((nv = nvhead) == NULL)
		nv = emalloc(sizeof(*nv));
	else
		nvhead = nv->nv_next;
	nv->nv_next = next;
	nv->nv_name = name;
	if (ptr == NULL)
		nv->nv_str = str;
	else {
		if (str != NULL)
			panic("newnv");
		nv->nv_ptr = ptr;
	}
	nv->nv_int = i;
	return (nv);
}

/*
 * Free an nvlist structure (just one).
 */
void
nvfree(nv)
	struct nvlist *nv;
{

	nv->nv_next = nvhead;
	nvhead = nv;
}

/*
 * Free an nvlist (the whole list).
 */
void
nvfreel(nv)
	struct nvlist *nv;
{
	struct nvlist *next;

	for (; nv != NULL; nv = next) {
		next = nv->nv_next;
		nv->nv_next = nvhead;
		nvhead = nv;
	}
}

void
#if __STDC__
warn(const char *fmt, ...)
#else
warn(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	extern const char *yyfile;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vxwarn(yyfile, currentline(), fmt, ap);
	va_end(ap);
}


static void
vxwarn(file, line, fmt, ap)
	const char *file;
	int line;
	const char *fmt;
	va_list ap;
{
	vxmsg(file, line, "warning: ", fmt, ap);
}

/*
 * External (config file) error.  Complain, using current file
 * and line number.
 */
void
#if __STDC__
error(const char *fmt, ...)
#else
error(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif	/* __STDC__ */
{
	va_list ap;
	extern const char *yyfile;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vxerror(yyfile, currentline(), fmt, ap);
	va_end(ap);
}

/*
 * Delayed config file error (i.e., something was wrong but we could not
 * find out about it until later).
 */
void
#if __STDC__
xerror(const char *file, int line, const char *fmt, ...)
#else
xerror(file, line, fmt, va_alist)
	const char *file;
	int line;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vxerror(file, line, fmt, ap);
	va_end(ap);
}

/*
 * Internal form of error() and xerror().
 */
static void
vxerror(file, line, fmt, ap)
	const char *file;
	int line;
	const char *fmt;
	va_list ap;
{
	vxmsg(file, line, "", fmt, ap);
	errors++;
}


/*
 * Internal error, abort.
 */
__dead void
#if __STDC__
panic(const char *fmt, ...)
#else
panic(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)fprintf(stderr, "config: panic: ");
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
	va_end(ap);
	exit(2);
}

/*
 * Internal form of error() and xerror().
 */
static void
vxmsg(file, line, msgclass, fmt, ap)
	const char *file;
	int line;
	const char *msgclass;
	const char *fmt;
	va_list ap;
{

	(void)fprintf(stderr, "%s:%d: %s", file, line, msgclass);
	(void)vfprintf(stderr, fmt, ap);
	(void)putc('\n', stderr);
}
