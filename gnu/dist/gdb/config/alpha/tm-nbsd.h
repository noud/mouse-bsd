/* Macro definitions for Alpha running under NetBSD.
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

#include "alpha/tm-alpha.h"
#ifndef S0_REGNUM
#define S0_REGNUM (T7_REGNUM+1)
#endif
#include "tm-nbsd.h"

#undef START_INFERIOR_TRAPS_EXPECTED
#define START_INFERIOR_TRAPS_EXPECTED 2

#undef NO_SINGLE_STEP
#define NO_SINGLE_STEP
#undef CANNOT_STEP_BREAKPOINT 

#define	SOLIB_BKPT_NAME		"__start"

#endif /* TM_NBSD_H */
