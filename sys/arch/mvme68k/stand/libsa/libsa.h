/*	$NetBSD: libsa.h,v 1.2 1996/05/19 20:08:15 chuck Exp $	*/

/*
 * libsa prototypes
 */

#include "libbug.h"

/* bugdev.c */
int bugscopen __P((struct open_file *, ...));
int bugscclose __P((struct open_file *));
int bugscioctl __P((struct open_file *, u_long, void *));
int bugscstrategy __P((void *, int, daddr_t, size_t, void *, size_t *));

/* exec_mvme.c */
void exec_mvme __P((char *, int));

/* parse_args.c */
void parse_args __P((char **, int *));
