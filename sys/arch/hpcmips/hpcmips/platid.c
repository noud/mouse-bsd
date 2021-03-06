/*	$NetBSD: platid.c,v 1.3 2000/02/06 08:47:05 takemura Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */
#include <sys/types.h>
#include <sys/systm.h>
#include <machine/platid.h>

void
platid_ntoh(platid_t *pid)
{
	pid->dw.dw0 = ntohl(pid->dw.dw0);
	pid->dw.dw1 = ntohl(pid->dw.dw1);
}


void
platid_hton(platid_t *pid)
{
	pid->dw.dw0 = htonl(pid->dw.dw0);
	pid->dw.dw1 = htonl(pid->dw.dw1);
}


void
platid_dump(char *name, void* pxx)
{
	int i;
	unsigned char* p = (unsigned char*)pxx;
	printf("%14s: ", name);

	for (i = 0; i < 8; i++) {
		printf("%02x", p[i]);
	}
	printf("\n");
}


platid_t platid_unknown = {{ PLATID_UNKNOWN, PLATID_UNKNOWN }};
platid_t platid_wild = {{ PLATID_WILD, PLATID_WILD }};
platid_t platid = {{ PLATID_UNKNOWN, PLATID_UNKNOWN }};


int
platid_match(platid_t *platid, platid_mask_t *mask)
{
	return platid_match_sub(platid, mask, 0);
}


int
platid_match_sub(platid_t *platid, platid_mask_t *mask, int unknown_is_match)
{
	int match_count;

#define PLATID_MATCH(mbr) \
	if (platid->s.mbr != mask->s.mbr && \
	    mask->s.mbr != platid_wild.s.mbr && \
	    !(platid->s.mbr == platid_unknown.s.mbr && unknown_is_match)) { \
		return (0); \
	} else if (platid->s.mbr == mask->s.mbr) \
		match_count++;

	match_count = 1;
	PLATID_MATCH(cpu_submodel);
	PLATID_MATCH(cpu_model);
	PLATID_MATCH(cpu_series);
	PLATID_MATCH(cpu_arch);

	PLATID_MATCH(submodel);
	PLATID_MATCH(model);
	PLATID_MATCH(series);
	PLATID_MATCH(vendor);

	return (match_count);

#undef PLATID_MATCH
}

char*
platid_name(platid_t *platid)
{
	int match_level;
	struct platid_name *p, *match, *pe;

	match_level = 0;
	pe = &platid_name_table[platid_name_table_size];
	for (p = platid_name_table; p < pe; p++) {
		int res = platid_match(platid, p->mask);
		if (match_level < res) {
			match = p;
			match_level = res;
		}
	}
	if (0 < match_level)
		return (match->name);
	else
		return ("UNKNOWN");
}
