/*	$NetBSD: version.c,v 1.3 1999/04/12 06:02:26 ross Exp $	*/

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

#include "os.h"
#include "top.h"
#include "patchlevel.h"

static char version[16];

char *version_string()

{
    sprintf(version, "%d.%d", VERSION, PATCHLEVEL);
#ifdef BETA
    strcat(version, BETA);
#endif
    return(version);
}
