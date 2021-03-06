/*	$NetBSD: sbusvar.h,v 1.10 2000/01/11 12:59:44 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#ifndef _SBUS_VAR_H
#define _SBUS_VAR_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <machine/bus.h>

struct sbus_softc;

/*
 * S-bus variables.
 */
struct sbusdev {
	struct	device *sd_dev;		/* backpointer to generic */
	struct	sbusdev *sd_bchain;	/* forward link in bus chain */
	void	(*sd_reset) __P((struct device *));
};


/* Device register space description */
struct sbus_reg {
	u_int32_t	sbr_slot;
	u_int32_t	sbr_offset;
	u_int32_t	sbr_size;
};

/* Interrupt information */
struct sbus_intr {
	u_int32_t	sbi_pri;	/* priority (IPL) */
	u_int32_t	sbi_vec;	/* vector (always 0?) */
};

/* Address translation accross busses */
struct sbus_range {
	u_int32_t	cspace;		/* Client space */
	u_int32_t	coffset;	/* Client offset */
	u_int32_t	pspace;		/* Parent space */
	u_int32_t	poffset;	/* Parent offset */
	u_int32_t	size;		/* Size in bytes of this range */
};

/*
 * Sbus driver attach arguments.
 */
struct sbus_attach_args {
	int		sa_placeholder;	/* for obio attach args sharing */
	bus_space_tag_t	sa_bustag;
	bus_dma_tag_t	sa_dmatag;
	char		*sa_name;	/* PROM node name */
	int		sa_node;	/* PROM handle */
	struct sbus_reg	*sa_reg;	/* Sbus register space for device */
	int		sa_nreg;	/* Number of Sbus register spaces */
#define sa_slot		sa_reg[0].sbr_slot
#define sa_offset	sa_reg[0].sbr_offset
#define sa_size		sa_reg[0].sbr_size

	struct sbus_intr *sa_intr;	/* Sbus interrupts for device */
	int		sa_nintr;	/* Number of interrupts */
#define sa_pri		sa_intr[0].sbi_pri

	u_int32_t	*sa_promvaddrs;/* PROM-supplied virtual addresses -- 32-bit */
	int		sa_npromvaddrs;	/* Number of PROM VAs */
#define sa_promvaddr	sa_promvaddrs[0]
};

/* sbus_attach_internal() is also used from obio.c */
void	sbus_attach_common __P((struct sbus_softc *, char *, int,
				const char * const *));
int	sbus_print __P((void *, const char *));

void	sbus_establish __P((struct sbusdev *, struct device *));

int	sbus_setup_attach_args __P((
		struct sbus_softc *,
		bus_space_tag_t,
		bus_dma_tag_t,
		int,			/*node*/
		struct sbus_attach_args *));

void	sbus_destroy_attach_args __P((struct sbus_attach_args *));

#define sbus_bus_map(t, bt, a, s, f, v, hp) \
	bus_space_map2(t, bt, a, s, f, v, hp)

#if notyet
/* variables per Sbus */
struct sbus_softc {
	struct	device sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;
	int	sc_clockfreq;		/* clock frequency (in Hz) */
	struct	sbusdev *sc_sbdev;	/* list of all children */
	struct	sbus_range *sc_range;
	int	sc_nrange;
	int	sc_burst;		/* burst transfer sizes supported */
	/* machdep stuff follows here */
	int	*sc_intr2ipl;		/* Interrupt level translation */
	int	*sc_intr_compat;	/* `intr' property to sbus compat */
};
#endif


/*
 * PROM-reported DMA burst sizes for the SBus
 */
#define SBUS_BURST_1	0x1
#define SBUS_BURST_2	0x2
#define SBUS_BURST_4	0x4
#define SBUS_BURST_8	0x8
#define SBUS_BURST_16	0x10
#define SBUS_BURST_32	0x20
#define SBUS_BURST_64	0x40

/* We use #defined(SUN4*) here while the ports are in flux */
#if defined(SUN4) || defined(SUN4C) || defined(SUN4M)
#include <sparc/dev/sbusvar.h>
#elif defined(SUN4U)
#include <sparc64/dev/sbusvar.h>
#endif

#endif /* _SBUS_VAR_H */
