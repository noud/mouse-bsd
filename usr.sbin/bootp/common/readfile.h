/*	$NetBSD: readfile.h,v 1.2 1998/01/09 08:09:14 perry Exp $	*/

/* readfile.h */

#include "bptypes.h"
#include "hash.h"

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern boolean hwlookcmp P((hash_datum *, hash_datum *));
extern boolean iplookcmp P((hash_datum *, hash_datum *));
extern boolean nmcmp P((hash_datum *, hash_datum *));
extern void readtab P((int));
extern void rdtab_init P((void));

#undef P

