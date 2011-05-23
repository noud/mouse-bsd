/* NetBSD/powerpc native-dependent code for GDB, the GNU debugger.
   Copyright 1986, 1987, 1989, 1991, 1992, 1994, 1995, 1996, 1997
	     Free Software Foundation, Inc.

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

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcore.h"
#include <sys/param.h>
#include <sys/ptrace.h>
#include <machine/frame.h>
#include <machine/reg.h>
#include <machine/pcb.h>

static void
fetch_core_registers PARAMS ((char *, unsigned int, int, CORE_ADDR));

void
fetch_inferior_registers (regno)
  int regno;
{
  struct reg infreg;

  /* Integer registers */
  ptrace(PT_GETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &infreg, 0);

  memcpy(&registers[REGISTER_BYTE (GP0_REGNUM)], infreg.fixreg,
	 sizeof(infreg.fixreg));
  *(long *) &registers[REGISTER_BYTE (PC_REGNUM)] = infreg.pc;
  *(long *) &registers[REGISTER_BYTE (PS_REGNUM)] = 0;		/* XXX */
  *(long *) &registers[REGISTER_BYTE (CR_REGNUM)] = infreg.cr;
  *(long *) &registers[REGISTER_BYTE (LR_REGNUM)] = infreg.lr;
  *(long *) &registers[REGISTER_BYTE (CTR_REGNUM)] = infreg.ctr;
  *(long *) &registers[REGISTER_BYTE (XER_REGNUM)] = infreg.xer;
  *(long *) &registers[REGISTER_BYTE (MQ_REGNUM)] = 0;

  /* Floating point registers */
  memset(&registers[REGISTER_BYTE (FP0_REGNUM)], 0, 8*32);

  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg infreg;

  /* Integer registers */
  memcpy(infreg.fixreg, &registers[REGISTER_BYTE (GP0_REGNUM)],
	 sizeof(infreg.fixreg));
  infreg.pc = *(long *) &registers[REGISTER_BYTE (PC_REGNUM)];
  infreg.cr = *(long *) &registers[REGISTER_BYTE (CR_REGNUM)];
  infreg.lr = *(long *) &registers[REGISTER_BYTE (LR_REGNUM)];
  infreg.ctr = *(long *) &registers[REGISTER_BYTE (CTR_REGNUM)];
  infreg.xer = *(long *) &registers[REGISTER_BYTE (XER_REGNUM)];

  ptrace(PT_SETREGS, inferior_pid, (PTRACE_ARG3_TYPE) &infreg, 0);

  /* Floating point registers */
  /* XXX not yet */

  registers_fetched ();
}

static void
fetch_core_registers (core_reg_sect, core_reg_size, which, reg_addr)
     char *core_reg_sect;
     unsigned core_reg_size;
     int which;
     CORE_ADDR reg_addr;	/* Unused in this version */
{
  struct md_coredump *core_reg;
  struct trapframe *tf;
  struct fpreg *fs;
  register int regnum;

  /* We get everything from the .reg section. */
  if (which != 0)
    return;

  core_reg = (struct md_coredump *)core_reg_sect;
  tf = &core_reg->frame;
#if 0
  fs = &core_reg->md_fpstate;
#endif

  if (core_reg_size < sizeof(*core_reg)) {
    fprintf_unfiltered (gdb_stderr, "Couldn't read regs from core file\n");
    return;
  }

  /* Integer registers */
  for (regnum = 0; regnum < 32; regnum++)
    *(long *) &registers[REGISTER_BYTE (regnum)] = tf->fixreg[regnum];

#if 0
  /* Floating point registers */
  memcpy (&registers[REGISTER_BYTE (FP0_REGNUM)],
	  &fs->fpr_regs[0], sizeof(fs->fpr_regs));
#endif

  /* Special registers (PC, LR) */
  *(long *) &registers[REGISTER_BYTE (PC_REGNUM)] = tf->srr0;
  *(long *) &registers[REGISTER_BYTE (LR_REGNUM)] = tf->lr;

  registers_fetched ();
}


/* Register that we are able to handle rs6000 core file formats. */

static struct core_fns ppcnbsd_core_fns =
{
  bfd_target_elf_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_core_ppcnbsd ()
{
  add_core_fns (&ppcnbsd_core_fns);
}
