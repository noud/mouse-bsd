/*	$NetBSD: if_wivar.h,v 1.4 2000/02/13 06:17:58 itojun Exp $	*/

/*
 * Copyright (c) 1997, 1998, 1999
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: if_wivar.h,v 1.4 2000/02/13 06:17:58 itojun Exp $
 */


/*
 * FreeBSD driver ported to NetBSD by Bill Sommerfeld in the back of the
 * Oslo IETF plenary meeting.
 */

struct wi_softc	{
	struct device sc_dev;
	struct ethercom sc_ethercom;

	struct pcmcia_function *sc_pf;
	struct pcmcia_io_handle sc_pcioh;
	int sc_iowin;

	bus_space_tag_t		wi_btag;
	bus_space_handle_t	wi_bhandle;

  	void *sc_sdhook;	/* saved shutdown hook for card */
	void *sc_ih;
	u_int8_t		sc_macaddr[6];

	struct ifmedia		ifmedia;
	int			wi_tx_data_id;
	int			wi_tx_mgmt_id;
	int			wi_gone;
	int			wi_if_flags;
	u_int16_t		wi_ptype;
	u_int16_t		wi_portnum;
	u_int16_t		wi_max_data_len;
	u_int16_t		wi_rts_thresh;
	u_int16_t		wi_ap_density;
	u_int16_t		wi_tx_rate;
	u_int16_t		wi_create_ibss;
	u_int16_t		wi_channel;
	u_int16_t		wi_pm_enabled;
	u_int16_t		wi_max_sleep;
	char			wi_node_name[32];
	char			wi_net_name[32];
	char			wi_ibss_name[32];
	u_int8_t		wi_txbuf[1596];
	int                     wi_has_wep;
	int                     wi_use_wep;
	int                     wi_tx_key;
	struct wi_ltv_keys      wi_keys;
	struct wi_counters	wi_stats;

	int			wi_resource; /* Resources alloc'ed on attach */
#define WI_RES_IO	1
#define WI_RES_NET	2
};
