/* Macro definitions for i386 running under NetBSD.
   Copyright 1994 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef TM_NBSD_H
#define TM_NBSD_H

#include "arm/tm-armnbsd.h"
#include "tm-nbsd.h"
#include <machine/setjmp.h>   /* for JMPBUF_REG_R14 definition */

/* On NetBSD, sigtramp is above the user stack and immediately below
   the user area. Using constants here allows for cross debugging. */
#define SIGTRAMP_START(pc)	0xefbfdfc0
#define SIGTRAMP_END(pc)	0xefbfe000	/* USRSTACK */

/* Saved Pc.  Get it from sigcontext if within sigtramp.  */
/* Offset to saved PC in sigcontext, from <sys/signal.h>.  */
#define SIGCONTEXT_PC_OFFSET 20	/* XXX - check this */

#define JB_ELEMENT_SIZE sizeof(int)	/* jmp_buf[_JBLEN] is array of ints */
#define JB_PC	_JB_REG_R14		/* Setjmp()'s return PC saved here */

/* Figure out where the longjmp will land.  Slurp the args out of the stack.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into ADDR.
   This routine returns true on success */

extern int
get_longjmp_target PARAMS ((CORE_ADDR *));

#define GET_LONGJMP_TARGET(ADDR) get_longjmp_target(ADDR)

#endif /* TM_NBSD_H */
