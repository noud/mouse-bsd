/*	$NetBSD: isapnpdevs.c,v 1.37 2000/02/08 06:36:46 erh Exp $	*/

/*
 * THIS FILE IS AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: isapnpdevs,v 1.32 1999/12/15 05:49:33 explorer Exp
 */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
#include <sys/param.h>
#include <dev/isapnp/isapnpdevs.h>


/* Adaptec SCSI */
static const struct isapnp_matchinfo isapnp_aha_devlogic[] = {
	{"ADP1542", 0},	/* Adaptec AHA-1542CP */
};
static const struct isapnp_matchinfo isapnp_aha_devcompat[] = {
	{"PNP00A0", 0},	/* Adaptec AHA-1542CP */
};
const struct isapnp_devinfo isapnp_aha_devinfo = {
	isapnp_aha_devlogic, 1,
	isapnp_aha_devcompat, 1,
};

/* Adaptec SCSI */
static const struct isapnp_matchinfo isapnp_aic_devlogic[] = {
	{"ADP1520", 0},	/* Adaptec AHA-1520B */
	{"ADP1502", 0},	/* Adaptec AHA-1502P */
	{"ADP1505", 0},	/* Adaptec AVA-1505A */
};
static const struct isapnp_matchinfo isapnp_aic_devcompat[] = {
	{"ADP1530", 0},	/* (Adaptec AVA-1505A) */
};
const struct isapnp_devinfo isapnp_aic_devinfo = {
	isapnp_aic_devlogic, 3,
	isapnp_aic_devcompat, 1,
};

/* National Semiconductor Serial */
static const struct isapnp_matchinfo isapnp_com_devlogic[] = {
	{"BDP3336", 0},	/* Best Data Prods. 336F */
	{"OZO8039", 0},	/* Zoom 56k flex */
	{"BRI1400", 0},	/* Boca 33.6 PnP */
	{"BRIB400", 0},	/* Boca 56k PnP */
	{"ROK0010", 0},	/* Rockwell ? */
	{"USR0004", 0},	/* USR Sportster 14.4k */
	{"USR0006", 0},	/* USR Sportster 33.6k */
	{"USR2070", 0},	/* USR Sportster 56k */
	{"USR3031", 0},	/* USR 56k Faxmodem */
	{"USR9190", 0},	/* USR 56k Voice INT */
	{"ZTIF761", 0},	/* Zoom ComStar 33.6 */
	{"CIR3000", 0},	/* Cirrus Logic V43 */
	{"MOT0000", 0},	/* Motorola ModemSurfr */
	{"SMM00C1", 0},	/* Leopard 56k PnP */
	{"SUP1650", 0},	/* Supra 336i Sp Intl */
	{"GVC0505", 0},	/* GVC 56k Faxmodem */
};
static const struct isapnp_matchinfo isapnp_com_devcompat[] = {
	{"PNP0500", 0},	/* Generic 8250/16450 */
	{"PNP0501", 0},	/* Generic 16550A */
};
const struct isapnp_devinfo isapnp_com_devinfo = {
	isapnp_com_devlogic, 16,
	isapnp_com_devcompat, 2,
};

/* 3Com 3CXXX Ethernet */
static const struct isapnp_matchinfo isapnp_ep_devlogic[] = {
	{"TCM5090", 0},	/* 3Com 3c509B */
	{"TCM5091", 0},	/* 3Com 3c509B-1 */
	{"TCM5094", 0},	/* 3Com 3c509B-4 */
	{"TCM5095", 0},	/* 3Com 3c509B-5 */
	{"TCM5098", 0},	/* 3Com 3c509B-8 */
};
const struct isapnp_devinfo isapnp_ep_devinfo = {
	isapnp_ep_devlogic, 5,
	NULL, 0,
};

/* ESS Audio Drive */
static const struct isapnp_matchinfo isapnp_ess_devlogic[] = {
	{"ESS1868", 0},	/* ESS1868 */
	{"ESS1869", 0},	/* ESS1869 */
};
const struct isapnp_devinfo isapnp_ess_devinfo = {
	isapnp_ess_devlogic, 2,
	NULL, 0,
};

/* Generic Joystick */
static const struct isapnp_matchinfo isapnp_joy_devlogic[] = {
	{"AZT0003", 0},	/* Aztech AZT2320 GAME PORT */
	{"CSC0001", 0},	/* CS4235 */
	{"CSCA801", 0},	/* Terratec EWS64XL */
	{"CTL7001", 0},	/* Creative Awe64 */
	{"CTL7002", 0},	/* Creative Vibra16CL */
	{"ESS0001", 0},	/* ESS1868 */
	{"OPT0001", 0},	/* OPTi Audio 16 */
	{"PNPB02F", 0},	/* XXX broken GUS PnP */
	{"ASB16FD", 0},	/* AdLib NSC 16 PNP */
};
static const struct isapnp_matchinfo isapnp_joy_devcompat[] = {
	{"PNPB02F", 0},	/* generic */
};
const struct isapnp_devinfo isapnp_joy_devinfo = {
	isapnp_joy_devlogic, 9,
	isapnp_joy_devcompat, 1,
};

/* Gravis Ultrasound */
static const struct isapnp_matchinfo isapnp_gus_devlogic[] = {
	{"GRV0000", 0},	/* Gravis Ultrasound */
};
const struct isapnp_devinfo isapnp_gus_devinfo = {
	isapnp_gus_devlogic, 1,
	NULL, 0,
};

/* Lance Ethernet */
static const struct isapnp_matchinfo isapnp_le_devlogic[] = {
	{"TKN0010", 0},	/* Lance Ethernet on TEKNOR board */
	{"ATK1500", 0},	/* Lance Ethernet on Allied Telesyn board */
};
const struct isapnp_devinfo isapnp_le_devinfo = {
	isapnp_le_devlogic, 2,
	NULL, 0,
};

/* MPU-401 MIDI UART */
static const struct isapnp_matchinfo isapnp_mpu_devlogic[] = {
	{"AZT0002", 0},	/* Aztech AZT2320 MPU401 MIDI */
};
const struct isapnp_devinfo isapnp_mpu_devinfo = {
	isapnp_mpu_devlogic, 1,
	NULL, 0,
};

/* NE2000 Ethernet */
static const struct isapnp_matchinfo isapnp_ne_devlogic[] = {
	{"@@@1980", 0},	/* OvisLink LE-8019R */
};
static const struct isapnp_matchinfo isapnp_ne_devcompat[] = {
	{"PNP80D6", 0},	/* Digital DE305 ISAPnP */
};
const struct isapnp_devinfo isapnp_ne_devinfo = {
	isapnp_ne_devlogic, 1,
	isapnp_ne_devcompat, 1,
};

/* PCMCIA bridge */
static const struct isapnp_matchinfo isapnp_pcic_devlogic[] = {
	{"SCM0469", 0},	/* SCM SwapBox Plug and Play */
	{"AEI0218", 0},	/* Actiontec PnP PCMCIA Adapter */
};
static const struct isapnp_matchinfo isapnp_pcic_devcompat[] = {
	{"PNP0E00", 0},	/* PCIC Compatible PCMCIA Bridge */
};
const struct isapnp_devinfo isapnp_pcic_devinfo = {
	isapnp_pcic_devlogic, 2,
	isapnp_pcic_devcompat, 1,
};

/* Creative Soundblaster */
static const struct isapnp_matchinfo isapnp_sb_devlogic[] = {
	{"ADS7150", 0},	/* AD1815 */
	{"ADS7180", 0},	/* AD1816 */
	{"CTL0001", 0},	/* SB */
	{"CTL0031", 0},	/* SB AWE32 */
	{"CTL0041", 0},	/* SB16 PnP (CT4131) */
	{"CTL0043", 0},	/* SB16 PnP (CT4170) */
	{"CTL0042", 0},	/* SB AWE64 Value */
	{"CTL0044", 0},	/* SB AWE64 Gold */
	{"CTL0045", 0},	/* SB AWE64 Value */
	{"OPT9250", 0},	/* Televideo card, Opti */
	{"@X@0001", 0},	/* CMI8330. Audio Adapter */
};
static const struct isapnp_matchinfo isapnp_sb_devcompat[] = {
	{"PNPB000", 0},	/* Generic SB 1.5 */
	{"PNPB001", 0},	/* Generic SB 2.0 */
	{"PNPB002", 0},	/* Generic SB Pro */
	{"PNPB003", 0},	/* Generic SB 16 */
};
const struct isapnp_devinfo isapnp_sb_devinfo = {
	isapnp_sb_devlogic, 11,
	isapnp_sb_devcompat, 4,
};

/* TROPIC Token-Ring */
static const struct isapnp_matchinfo isapnp_tr_devlogic[] = {
	{"IBM0000", 0},	/* IBM TROPIC Token-Ring */
	{"TCM3190", 0},	/* 3Com TokenLink Velocity ISA */
};
const struct isapnp_devinfo isapnp_tr_devinfo = {
	isapnp_tr_devlogic, 2,
	NULL, 0,
};

/* Western Digital Disk Controller */
static const struct isapnp_matchinfo isapnp_wdc_devlogic[] = {
	{"AZT0000", 0},	/* Aztech AZT2320 IDE CDROM */
	{"OPT0007", 0},	/* OPTi Audio 16 IDE controller */
};
static const struct isapnp_matchinfo isapnp_wdc_devcompat[] = {
	{"PNP0600", 0},	/* Western Digital Compatible Controller */
};
const struct isapnp_devinfo isapnp_wdc_devinfo = {
	isapnp_wdc_devlogic, 2,
	isapnp_wdc_devcompat, 1,
};

/* Microsoft Sound System */
static const struct isapnp_matchinfo isapnp_wss_devlogic[] = {
	{"AZT0001", 1},	/* Aztech AZT2320 AUDIO */
	{"CSC0000", 0},	/* Windows Sound System */
	{"CSC0100", 0},	/* CS4235 CODEC */
	{"CSCA800", 0},	/* Terratec EWS64 CoDec */
	{"ASB1611", 0},	/* AdLib NSC 16 PNP */
	{"ASB1622", 0},	/* AdLib MSC 32 Wave PnP V3SB */
};
const struct isapnp_devinfo isapnp_wss_devinfo = {
	isapnp_wss_devlogic, 6,
	NULL, 0,
};

/* Yamaha Sound */
static const struct isapnp_matchinfo isapnp_ym_devlogic[] = {
	{"YMH0021", 0},	/* OPL3-SA2, OPL3-SA3 */
};
const struct isapnp_devinfo isapnp_ym_devinfo = {
	isapnp_ym_devlogic, 1,
	NULL, 0,
};
