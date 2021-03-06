/*	$NetBSD: platid_test.c,v 1.1.1.1 1999/09/16 12:23:21 takemura Exp $	*/

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
#include <machine/platid.h>
#define PLATID_DEFINE_MASK_NICKNAME
#include <machine/platid_mask.h>

platid_bit_test()
{
	platid_t pid;
	int err_count = 0;
	int verbose = 0;

	bzero((char*)&pid, sizeof(pid));
	pid.s.cpu_arch = ~0;
	if (pid.dw.dw0 != PLATID_CPU_ARCH_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("cpu_arch", &pid);
		printf("%14s: %08lx00000000\n", "", PLATID_CPU_ARCH_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.cpu_series = ~0;
	if (pid.dw.dw0 != PLATID_CPU_SERIES_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("cpu_series", &pid);
		printf("%14s: %08lx00000000\n", "", PLATID_CPU_SERIES_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.cpu_model = ~0;
	if (pid.dw.dw0 != PLATID_CPU_MODEL_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("cpu_model", &pid);
		printf("%14s: %08lx00000000\n", "", PLATID_CPU_MODEL_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.cpu_submodel = ~0;
	if (pid.dw.dw0 != PLATID_CPU_SUBMODEL_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("cpu_submodel", &pid);
		printf("%14s: %08lx00000000\n", "", PLATID_CPU_SUBMODEL_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.flags = ~0;
	if (pid.dw.dw0 != PLATID_FLAGS_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("flags", &pid);
		printf("%14s: %08lx00000000\n", "", PLATID_FLAGS_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.vendor = ~0;
	if (pid.dw.dw1 != PLATID_VENDOR_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("vendor", &pid);
		printf("%14s: 00000000%08lx\n", "", PLATID_VENDOR_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.series = ~0;
	if (pid.dw.dw1 != PLATID_SERIES_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("series", &pid);
		printf("%14s: 00000000%08lx\n", "", PLATID_SERIES_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.model = ~0;
	if (pid.dw.dw1 != PLATID_MODEL_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("model", &pid);
		printf("%14s: 00000000%08lx\n", "", PLATID_MODEL_MASK);
	}

	bzero((char*)&pid, sizeof(pid));
	pid.s.submodel = ~0;
	if (pid.dw.dw1 != PLATID_SUBMODEL_MASK && err_count++ || verbose) {
		platid_hton(&pid);
		platid_dump("submodel", &pid);
		printf("%14s: 00000000%08lx\n", "", PLATID_SUBMODEL_MASK);
	}

	if (err_count) {
		printf("error!\n");
	}
}


void
main()
{
	platid_t pid = platid_unknown;

	platid_bit_test();

	pid.dw.dw0 =	PLATID_CPU_MIPS_VR_4111;
	pid.dw.dw1 =	PLATID_MACH_NEC_MCR_500;

	printf("CPU_MIPS:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS) ? "O" : "X");
	printf("CPU_MIPS_VR:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS_VR) ? "O" : "X");
	printf("CPU_MIPS_VR41XX:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS_VR_41XX) ? "O" : "X");
	printf("CPU_MIPS_VR4102:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS_VR_4102) ? "O" : "X");
	printf("CPU_MIPS_VR4111:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS_VR_4111) ? "O" : "X");
	printf("CPU_MIPS_VR4121:\t%s\n",
	       platid_match(&pid, GENERIC_MIPS_VR_4121) ? "O" : "X");
	printf("NEC_MCR:\t%s\n",
	       platid_match(&pid, NEC_MCR) ? "O" : "X");
	printf("NEC_MCR_5XX:\t%s\n",
	       platid_match(&pid, NEC_MCR_5XX) ? "O" : "X");
	printf("NEC_MCR_500:\t%s\n",
	       platid_match(&pid, NEC_MCR_500) ? "O" : "X");
	printf("NEC_MCR_510:\t%s\n",
	       platid_match(&pid, NEC_MCR_510) ? "O" : "X");

	exit(0);
}
