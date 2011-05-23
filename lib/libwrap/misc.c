/*	$NetBSD: misc.c,v 1.7 1999/08/31 13:58:58 itojun Exp $	*/

 /*
  * Misc routines that are used by tcpd and by tcpdchk.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsic[] = "@(#) misc.c 1.2 96/02/11 17:01:29";
#else
__RCSID("$NetBSD: misc.c,v 1.7 1999/08/31 13:58:58 itojun Exp $");
#endif
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "tcpd.h"

#ifndef	INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

/* xgets - fgets() with backslash-newline stripping */

char   *xgets(ptr, len, fp)
char   *ptr;
int     len;
FILE   *fp;
{
    int     got;
    char   *start = ptr;

    while (fgets(ptr, len, fp)) {
	got = strlen(ptr);
	if (got >= 1 && ptr[got - 1] == '\n') {
	    tcpd_context.line++;
	    if (got >= 2 && ptr[got - 2] == '\\') {
		got -= 2;
	    } else {
		return (start);
	    }
	}
	ptr += got;
	len -= got;
	ptr[0] = 0;
    }
    return (ptr > start ? start : 0);
}

/* split_at - break string at delimiter or return NULL */

char   *split_at(string, delimiter)
char   *string;
int     delimiter;
{
#if 0
    char   *cp;

    if ((cp = strchr(string, delimiter)) != 0)
	*cp++ = 0;
    return (cp);
#else
    char *cp;
    int bracket;

    bracket = 0;
    for (cp = string; cp && *cp; cp++) {
	switch (*cp) {
	case '[':
	    bracket++;
	    break;
	case ']':
	    bracket--;
	    break;
	default:
	    if (bracket == 0 && *cp == delimiter) {
		*cp++ = 0;
		return cp;
	    }
	    break;
	}
    }
    return NULL;
#endif
}

/* dot_quad_addr - convert dotted quad to internal form */

int dot_quad_addr(str, addr)
char   *str;
unsigned long *addr;
{
    struct in_addr a;

    if (!inet_aton(str, &a))
	return -1;
    if (addr)
	*addr = a.s_addr;
    return 0;
}
