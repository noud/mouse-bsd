/*	$NetBSD: exec_sun.c,v 1.9 1998/06/29 20:02:49 gwr Exp $ */

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/exec_aout.h>

#include <stand.h>
#include "libsa.h"

int errno;

/*ARGSUSED*/
int exec_sun(file, loadaddr)
	char	*file;
	char	*loadaddr;
{
	char *entry;
	int io, rv;

#ifdef	DEBUG
	printf("exec_sun: file=%s loadaddr=0x%x\n", file, loadaddr);
#endif

	io = open(file, 0);
	if (io < 0)
		return(-1);
	rv = load_sun(io, loadaddr, &entry);
	close(io);

	if (rv == 0) {
		printf("Starting program at 0x%x\n", entry);
		chain_to((void*)entry);
		/* not reached */
	}
	return (rv);
}


/*ARGSUSED*/
int load_sun(io, loadaddr, entry)
	int io;
	char *loadaddr;
	char **entry;
{
	struct exec x;
	int cc, magic;
	register char *cp;
	register int *ip;

	/*
	 * Read in the exec header, and validate it.
	 */
	if (read(io, (char *)&x, sizeof(x)) != sizeof(x))
		goto shread;
	if (N_BADMAG(x)) {
		errno = EFTYPE;
		return (-1);
	}

	cp = loadaddr;
	magic = N_GETMAGIC(x);
	if (magic == ZMAGIC)
		cp += sizeof(x);
	*entry = cp;

	/*
	 * Leave a copy of the exec header before the text.
	 * The sun3 kernel uses this to verify that the
	 * symbols were loaded by this boot program.
	 */
	bcopy(&x, cp - sizeof(x), sizeof(x));

	/*
	 * Read in the text segment.  Note:
	 * a.out header part of text in zmagic
	 */
	printf("%d", x.a_text);
	cc = x.a_text;
	if (magic == ZMAGIC)
		cc -= sizeof(x);
	if (read(io, cp, cc) != cc)
		goto shread;
	cp += cc;

	/*
	 * NMAGIC may have a gap between text and data.
	 */
	if (magic == NMAGIC) {
		register int mask = N_PAGSIZ(x) - 1;
		while ((int)cp & mask)
			*cp++ = 0;
	}

	/*
	 * Read in the data segment.
	 */
	printf("+%d", x.a_data);
	if (read(io, cp, x.a_data) != x.a_data)
		goto shread;
	cp += x.a_data;

	/*
	 * Zero out the BSS section.
	 * (Kernel doesn't care, but do it anyway.)
	 */
	printf("+%d", x.a_bss);
	cc = x.a_bss;
	while ((int)cp & 3) {
		*cp++ = 0;
		--cc;
	}
	ip = (int*)cp;
	cp += cc;
	while ((char*)ip < cp)
		*ip++ = 0;

	/*
	 * Read in the symbol table and strings.
	 * (Always set the symtab size word.)
	 */
	*ip++ = x.a_syms;
	cp = (char*) ip;

	if (x.a_syms > 0) {

		/* Symbol table and string table length word. */
		cc = x.a_syms;
		printf("+[%d", cc);
		cc += sizeof(int);	/* strtab length too */
		if (read(io, cp, cc) != cc)
			goto shread;
		cp += x.a_syms;
		ip = (int*)cp;  	/* points to strtab length */
		cp += sizeof(int);

		/* String table.  Length word includes itself. */
		cc = *ip;
		printf("+%d]", cc);
		cc -= sizeof(int);
		if (cc <= 0)
			goto shread;
		if (read(io, cp, cc) != cc)
			goto shread;
		cp += cc;
	}
	printf("=0x%x\n", cp - loadaddr);
	return (0);

shread:
	printf("exec: short read\n");
	errno = EIO;
	return(-1);
}
