/*	$NetBSD: autoconf.h,v 1.32 2000/01/11 13:00:36 pk Exp $ */

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)autoconf.h	8.2 (Berkeley) 9/30/93
 */

/*
 * Autoconfiguration information.
 */

#include <machine/bus.h>
#include <machine/bsd_openprom.h>
#include <machine/promlib.h>
#include <dev/sbus/sbusvar.h>

/*
 * Most devices are configured according to information kept in
 * the FORTH PROMs.  In particular, we extract the `name', `reg',
 * and `address' properties of each device attached to the mainbus;
 * other drives may also use this information.  The mainbus itself
 * (which `is' the CPU, in some sense) gets just the node, with a
 * fake name ("mainbus").
 */

/* Device register space description */
struct rom_reg {
	u_int32_t	rr_iospace;	/* register space (obio, etc) */
	u_int32_t	rr_paddr;	/* register physical address */
	u_int32_t	rr_len;		/* register length */
};

/* Interrupt information */
struct rom_intr {
	u_int32_t	int_pri;	/* priority (IPL) */
	u_int32_t	int_vec;	/* vector (always 0?) */
};

/* Address translation accross busses */
struct rom_range {		/* Only used on v3 PROMs */
	u_int32_t	cspace;		/* Client space */
	u_int32_t	coffset;	/* Client offset */
	u_int32_t	pspace;		/* Parent space */
	u_int32_t	poffset;	/* Parent offset */
	u_int32_t	size;		/* Size in bytes of this range */
};

/* Attach arguments presented by mainbus_attach() */
struct mainbus_attach_args {
	bus_space_tag_t	ma_bustag;	/* parent bus tag */
	bus_dma_tag_t	ma_dmatag;
	char		*ma_name;	/* PROM node name */
	int		ma_node;	/* PROM handle */
	bus_type_t	ma_iospace;	/* device I/O space */
	bus_addr_t	ma_paddr;	/* register physical address */
	int		ma_size;	/* register physical size */
	int		ma_pri;		/* priority (IPL) */
	void		*ma_promvaddr;	/* PROM virtual address, if any */
};

/* Attach arguments presented to devices by obio_attach() (sun4 only) */
struct obio4_attach_args {
	int		oba_placeholder;/* obio/sbus attach args sharing */
	bus_space_tag_t	oba_bustag;	/* parent bus tag */
	bus_dma_tag_t	oba_dmatag;
	bus_addr_t	oba_paddr;	/* register physical address */
	int		oba_pri;	/* interrupt priority (IPL) */
};

union obio_attach_args {
	/* sun4m obio space is treated like an sbus slot */
	int				uoba_isobio4;
	struct sbus_attach_args		uoba_sbus;	/* Sbus view */
	struct obio4_attach_args	uoba_oba4;	/* sun4 on-board view */
};

#define obio_bus_map(t, a, o, s, f, v, hp)		\
	bus_space_map2(t, 0, (long)(a) + o, s, f, (vaddr_t)v, hp)

/* obio specific bus flag */
#define OBIO_BUS_MAP_USE_ROM	BUS_SPACE_MAP_BUS1

/* obio bus helper that finds ROM mappings; exported for autoconf.c */
int	obio_find_rom_map __P((bus_addr_t, bus_type_t, int,
				bus_space_handle_t *));


/*
 * The matchbyname function is useful in drivers that are matched
 * by romaux name, i.e., all `mainbus attached' devices.  It expects
 * its aux pointer to point to a pointer to the name (the address of
 * a romaux structure suffices, for instance). (OBSOLETE)
 */
struct device;
struct cfdata;
int	matchbyname __P((struct device *, struct cfdata *cf, void *aux));

/*
 * `clockfreq' produces a printable representation of a clock frequency
 * (this is just a frill).
 */
char	*clockfreq __P((int freq));

/*
 * Memory description arrays.  Shared between pmap.c and autoconf.c; no
 * one else should use this (except maybe mem.c, e.g., if we fix the VM to
 * handle discontiguous physical memory).
 */
struct memarr {
	paddr_t	addr;
	psize_t	len;
};
int	makememarr(struct memarr *, int max, int which);
#define	MEMARR_AVAILPHYS	0
#define	MEMARR_TOTALPHYS	1

/* Openprom V2 style boot path */
struct bootpath {
	char	name[16];	/* name of this node */
	int	val[3];		/* up to three optional values */
	struct device *dev;	/* device that recognised this component */
};

#if 0
struct bootpath	*bootpath_store __P((int, struct bootpath *));
#endif
int		sd_crazymap __P((int));

/* Parse a disk string into a dev_t, return device struct pointer */
struct	device *parsedisk __P((char *, int, int, dev_t *));

/* Establish a mountroot_hook, for benefit of floppy drive, mostly. */
void	mountroot_hook_establish __P((void (*) __P((struct device *)),
				      struct device *));

void	bootstrap __P((void));
struct device *getdevunit __P((char *, int));
void	*findzs __P((int));
int	romgetcursoraddr __P((int **, int **));
