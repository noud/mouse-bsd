/*	$NetBSD: intio.c,v 1.2 1999/01/28 11:46:23 dbj Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Autoconfiguration support for next68k internal i/o space.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <next68k/dev/intiovar.h>

int	intiomatch __P((struct device *, struct cfdata *, void *));
void	intioattach __P((struct device *, struct device *, void *));
int	intioprint __P((void *, const char *));
int	intiosearch __P((struct device *, struct cfdata *, void *));

struct cfattach intio_ca = {
	sizeof(struct device), intiomatch, intioattach
};

#if 0
struct cfdriver intio_cd = {
	NULL, "intio", DV_DULL
};
#endif

int
intiomatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	static int intio_matched = 0;

	/* Allow only one instance. */
	if (intio_matched)
		return (0);

	intio_matched = 1;
	return (1);
}

void
intioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	printf("\n");

	/* Search for and attach children. */
	config_search(intiosearch, self, NULL);
}

int
intioprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct intio_attach_args *ia = aux;

	if (ia->ia_addr != 0)
		printf(" addr %p", ia->ia_addr);
	return (UNCONF);
}

int
intiosearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct intio_attach_args ia;

	bzero(&ia, sizeof(ia));
	if ((*cf->cf_attach->ca_match)(parent, cf, &ia) > 0) {
		config_attach(parent, cf, &ia, intioprint);
	}
	return (0);
}
