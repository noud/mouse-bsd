/*	$NetBSD: parse_args.c,v 1.2 1997/12/17 21:33:10 scw Exp $	*/

/*-
 * Copyright (c) 1995 Theo de Raadt
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
 *	This product includes software developed under OpenBSD by
 *	Theo de Raadt for Willowglen Singapore.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <machine/prom.h>
#include <a.out.h>

#include "stand.h"
#include "libsa.h"

#define KERNEL_NAME "netbsd"

struct flags {
	char c;
	short bit;
} bf[] = {
	{ 'a', RB_ASKNAME },
	{ 'b', RB_HALT },
	{ 'y', RB_NOSYM },
	{ 'd', RB_KDB },
	{ 'm', RB_MINIROOT },
	{ 'r', RB_DFLTROOT },
	{ 's', RB_SINGLE },
};

void
parse_args(filep, flagp)

char **filep;
int *flagp;

{
	char *name = KERNEL_NAME, *ptr;
	int i, howto = 0;
	char c;

	if (bugargs.arg_start != bugargs.arg_end) {
		ptr = bugargs.arg_start;
		while (c = *ptr) {
			while (c == ' ')
				c = *++ptr;
			if (c == '\0')
				return;
			if (c != '-') {
				if ( ptr[1] == ':' ) {
					howto |= RB_ASKNAME;
					if ( ptr[2] == ' ' || ptr[2] == '\0' ) {
						ptr += 2;
						continue;
					}
					name = &(ptr[2]);
				} else
					name = ptr;
				while ((c = *++ptr) && c != ' ')
					;
				if (c)
					*ptr++ = 0;
				continue;
			}
			while ((c = *++ptr) && c != ' ') {
				for (i = 0; i < sizeof(bf)/sizeof(bf[0]); i++)
					if (bf[i].c == c) {
						howto |= bf[i].bit;
					}
			}
		}
	}
	*flagp = howto;
	*filep = name;
}
