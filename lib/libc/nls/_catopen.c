/*	$NetBSD: _catopen.c,v 1.4 1997/11/04 23:52:52 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_catopen,catopen)
#else

#include <nl_types.h>
nl_catd	_catopen __P((__const char *, int));	/* XXX */

nl_catd
catopen(name, oflag)
	__const char *name;
	int oflag;
{
	return _catopen(name, oflag);
}

#endif
