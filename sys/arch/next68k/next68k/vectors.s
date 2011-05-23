| $NetBSD: vectors.s,v 1.6 1998/11/19 08:32:26 dbj Exp $

| This file was taken from from mvme68k/mvme68k/vectors.s
| should probably be re-synced when needed.
| Darrin B. Jewell <jewell@mit.edu>  Tue Nov 10 05:07:16 1998
| original cvs id: NetBSD: vectors.s,v 1.7 1998/10/18 04:42:37 itohy Exp

| Copyright (c) 1988 University of Utah
| Copyright (c) 1990, 1993
|	The Regents of the University of California.  All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions
| are met:
| 1. Redistributions of source code must retain the above copyright
|    notice, this list of conditions and the following disclaimer.
| 2. Redistributions in binary form must reproduce the above copyright
|    notice, this list of conditions and the following disclaimer in the
|    documentation and/or other materials provided with the distribution.
| 3. All advertising materials mentioning features or use of this software
|    must display the following acknowledgement:
|	This product includes software developed by the University of
|	California, Berkeley and its contributors.
| 4. Neither the name of the University nor the names of its contributors
|    may be used to endorse or promote products derived from this software
|    without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
| ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
| IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
| ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
| FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
| DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
| OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
| HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
| OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
| SUCH DAMAGE.
|
|	@(#)vectors.s	8.2 (Berkeley) 1/21/94
|

	.data
	.globl	_illinst,_zerodiv,_chkinst,_trapvinst,_privinst,_trace
	.globl	_badtrap
	.globl	_spurintr,_intrhand_autovec,_lev7intr
	.globl	_trap0
#ifdef COMPAT_13
	.globl	_trap1
#endif
	.globl	_trap2,_trap15
	.globl	_fpfline, _fpunsupp
	.globl	_trap12
	.globl	_vectab

_vectab:
	.long	_badtrap	/* 0: (unused reset SSP) */
	.long	_badtrap	/* 1: NOT USED (reset PC) */
	.long	_badtrap	/* 2: bus error (installed at boot in locore.s) */
	.long	_badtrap	/* 3: address error (installed at boot in locore.s) */
	.long	_illinst	/* 4: illegal instruction */
	.long	_zerodiv	/* 5: zero divide */
	.long	_chkinst	/* 6: CHK instruction */
	.long	_trapvinst	/* 7: TRAPV instruction */
	.long	_privinst	/* 8: privilege violation */
	.long	_trace		/* 9: trace */
	.long	_illinst	/* 10: line 1010 emulator */
	.long	_fpfline	/* 11: line 1111 emulator */
	.long	_badtrap	/* 12: unassigned, reserved */
	.long	_coperr		/* 13: coprocessor protocol violation */
	.long	_fmterr		/* 14: format error */
	.long	_badtrap	/* 15: uninitialized interrupt vector */
	.long	_badtrap	/* 16: unassigned, reserved */
	.long	_badtrap	/* 17: unassigned, reserved */
	.long	_badtrap	/* 18: unassigned, reserved */
	.long	_badtrap	/* 19: unassigned, reserved */
	.long	_badtrap	/* 20: unassigned, reserved */
	.long	_badtrap	/* 21: unassigned, reserved */
	.long	_badtrap	/* 22: unassigned, reserved */
	.long	_badtrap	/* 23: unassigned, reserved */
	.long	_spurintr	/* 24: spurious interrupt */
	.long	_intrhand_autovec /* 25: level 1 interrupt autovector */
	.long	_intrhand_autovec /* 26: level 2 interrupt autovector */
	.long	_intrhand_autovec /* 27: level 3 interrupt autovector */
	.long	_intrhand_autovec /* 28: level 4 interrupt autovector */
	.long	_intrhand_autovec /* 29: level 5 interrupt autovector */
	.long	_intrhand_autovec /* 30: level 6 interrupt autovector */
	.long	_lev7intr	/* 31: level 7 interrupt autovector */
	.long	_trap0		/* 32: syscalls */
#ifdef COMPAT_13
	.long	_trap1		/* 33: compat_13_sigreturn */
#else
	.long	_illinst
#endif
	.long	_trap2		/* 34: trace */
	.long	_trap3		/* 35: sigreturn special syscall */
	.long	_illinst	/* 36: TRAP instruction vector */
	.long	_illinst	/* 37: TRAP instruction vector */
	.long	_illinst	/* 38: TRAP instruction vector */
	.long	_illinst	/* 39: TRAP instruction vector */
	.long	_illinst	/* 40: TRAP instruction vector */
	.long	_illinst	/* 41: TRAP instruction vector */
	.long	_illinst	/* 42: TRAP instruction vector */
	.long	_illinst	/* 43: TRAP instruction vector */
	.long	_trap12		/* 44: TRAP instruction vector */
	.long	_illinst	/* 45: TRAP instruction vector */
	.long	_illinst	/* 46: TRAP instruction vector */
	.long	_trap15		/* 47: TRAP instruction vector */
#ifdef FPSP
	.globl  bsun, inex, dz, unfl, operr, ovfl, snan
	.long   bsun            /* 48: FPCP branch/set on unordered cond */
	.long   inex            /* 49: FPCP inexact result */
	.long   dz              /* 50: FPCP divide by zero */
	.long   unfl            /* 51: FPCP underflow */
	.long   operr           /* 52: FPCP operand error */
	.long   ovfl            /* 53: FPCP overflow */
	.long   snan            /* 54: FPCP signalling NAN */
#else
	.globl  _fpfault
	.long   _fpfault        /* 48: FPCP branch/set on unordered cond */
	.long   _fpfault        /* 49: FPCP inexact result */
	.long   _fpfault        /* 50: FPCP divide by zero */
	.long   _fpfault        /* 51: FPCP underflow */
	.long   _fpfault        /* 52: FPCP operand error */
	.long   _fpfault        /* 53: FPCP overflow */
	.long   _fpfault        /* 54: FPCP signalling NAN */
#endif

	.long	_fpunsupp	/* 55: FPCP unimplemented data type */
	.long	_badtrap	/* 56: unassigned, reserved */
	.long	_badtrap	/* 57: unassigned, reserved */
	.long	_badtrap	/* 58: unassigned, reserved */
	.long	_badtrap	/* 59: unassigned, reserved */
	.long	_badtrap	/* 60: unassigned, reserved */
	.long	_badtrap	/* 61: unassigned, reserved */
	.long	_badtrap	/* 62: unassigned, reserved */
	.long	_badtrap	/* 63: unassigned, reserved */
#define BADTRAP16	.long	_badtrap,_badtrap,_badtrap,_badtrap,\
				_badtrap,_badtrap,_badtrap,_badtrap,\
				_badtrap,_badtrap,_badtrap,_badtrap,\
				_badtrap,_badtrap,_badtrap,_badtrap
	/*
	 * PCC, PCCTWO, MC, and VME vectors are installed from 64-255
	 * by the *intr_extablish() functions.
	 */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
	BADTRAP16		/* 64-255: user interrupt vectors */
