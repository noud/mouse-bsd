/*	$NetBSD: makebuf.c,v 1.12 1999/09/20 04:39:30 lukem Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
static char sccsid[] = "@(#)makebuf.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: makebuf.c,v 1.12 1999/09/20 04:39:30 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "local.h"

static int gen_dec_num(char *to, unsigned int v)
{
 int n;

 if (v >= 10)
  { n = gen_dec_num(to,v/10);
    v %= 10;
  }
 else
  { n = 0;
  }
 to[n] = '0' + v;
 return(n+1);
}

typedef enum {
	  EB_NO_SPEC = 1,
	  EB_SPEC_NONE,
	  EB_SPEC_LINE,
	  EB_SPEC_FULL,
	  } EBRV;
static EBRV env_buffering(int fd)
{
 char en[64];
 const char *ev;
 int i;

 ev = "STDIO_BUFFERING_";
 for (i=0;ev[i];i++) en[i] = ev[i];
 en[i+gen_dec_num(&en[i],fd)] = '\0';
 ev = getenv(&en[0]);
 if (! ev) ev = getenv("STDIO_BUFFERING");
 if (! ev) return(EB_NO_SPEC);
 switch (*ev)
  { case 'n': case 'N': return(EB_SPEC_NONE); break;
    case 'l': case 'L': return(EB_SPEC_LINE); break;
    case 'f': case 'F': return(EB_SPEC_FULL); break;
  }
 return(EB_NO_SPEC);
}

/*
 * Allocate a file buffer, or switch to unbuffered I/O.
 * Per the ANSI C standard, ALL tty devices default to line buffered.
 *
 * As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
 * optimisation) right after the fstat() that finds the buffer size.
 */
void __smakebuf(FILE *fp)
{
 void *p;
 int flags;
 size_t size;
 int couldbetty;
 EBRV eb;

 _DIAGASSERT(fp != NULL);
 eb = env_buffering(__sfileno(fp));
 if (eb == EB_SPEC_NONE) fp->_flags |= __SNBF;
 if (fp->_flags & __SNBF)
  { fp->_bf._base = fp->_p = fp->_nbuf;
    fp->_bf._size = 1;
    return;
  }
 flags = __swhatbuf(fp, &size, &couldbetty);
 p = malloc(size);
 if (! p)
  { fp->_flags |= __SNBF;
    fp->_bf._base = fp->_p = fp->_nbuf;
    fp->_bf._size = 1;
    return;
  }
 __cleanup = _cleanup;
 flags |= __SMBF;
 fp->_bf._base = fp->_p = p;
 fp->_bf._size = size;
 if ( (eb == EB_SPEC_LINE) ||
      ( (eb != EB_SPEC_FULL) &&
	couldbetty &&
	isatty(fp->_file) ) ) flags |= __SLBF;
 fp->_flags |= flags;
}

/*
 * Internal routine to determine `proper' buffering for a file.
 */
int
__swhatbuf(fp, bufsize, couldbetty)
	FILE *fp;
	size_t *bufsize;
	int *couldbetty;
{
	struct stat st;

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(bufsize != NULL);
	_DIAGASSERT(couldbetty != NULL);

	if (fp->_file < 0 || fstat(fp->_file, &st) < 0) {
		*couldbetty = 0;
		*bufsize = BUFSIZ;
		return (__SNPT);
	}

	/* could be a tty iff it is a character device */
	*couldbetty = S_ISCHR(st.st_mode);
	if (st.st_blksize == 0) {
		*bufsize = BUFSIZ;
		return (__SNPT);
	}

	/*
	 * Optimise fseek() only if it is a regular file.  (The test for
	 * __sseek is mainly paranoia.)  It is safe to set _blksize
	 * unconditionally; it will only be used if __SOPT is also set.
	 */
	*bufsize = st.st_blksize;
	fp->_blksize = st.st_blksize;
	return ((st.st_mode & S_IFMT) == S_IFREG && fp->_seek == __sseek ?
	    __SOPT : __SNPT);
}
