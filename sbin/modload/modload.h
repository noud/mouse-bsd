/*	$NetBSD: modload.h,v 1.1 1999/06/13 12:54:40 mrg Exp $	*/

/*
 * Copyright (c) 1993 Terrence R. Lambert.
 * All rights reserved.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __modload_h__
#define __modload_h__

int	elf_mod_sizes	__P((int, size_t *, int *, struct lmc_resrv *,
			     struct stat *));
void	*elf_mod_load	__P((int));
void	elf_linkcmd	__P((char*, size_t, const char*, const char*,
			     const char*, const void*, const char*));
void	elf_mod_symload	__P((int));

int	a_out_mod_sizes __P((int, size_t *, int *, struct lmc_resrv *,
			     struct stat *));
void	*a_out_mod_load	__P((int));
void	a_out_linkcmd	__P((char*, size_t, const char*, const char*,
			     const char*, const void*, const char*));
void	a_out_mod_symload __P((int));

#ifndef USE_AOUT
#define mod_sizes elf_mod_sizes
#define mod_load elf_mod_load
#define mod_symload elf_mod_symload
#define linkcmd elf_linkcmd
#else
#define mod_sizes a_out_mod_sizes
#define mod_load a_out_mod_load
#define mod_symload a_out_mod_symload
#define linkcmd a_out_linkcmd
#endif

void loadbuf(void*, size_t);
void loadspace(size_t);

extern int debug;
extern int verbose;

#endif /* __modload_h__ */
