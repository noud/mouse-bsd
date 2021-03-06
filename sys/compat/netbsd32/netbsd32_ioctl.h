/*	$NetBSD: netbsd32_ioctl.h,v 1.3 1999/03/25 16:22:49 mrg Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* from arch/sparc/include/fbio.h */
#if 0
/* unused */
#define	FBIOGINFO	_IOR('F', 2, struct fbinfo)
#endif

struct netbsd32_fbcmap {
	int	index;		/* first element (0 origin) */
	int	count;		/* number of elements */
	netbsd32_u_charp	red;		/* red color map elements */
	netbsd32_u_charp	green;		/* green color map elements */
	netbsd32_u_charp	blue;		/* blue color map elements */
};
#if 0
#define	FBIOPUTCMAP	_IOW('F', 3, struct fbcmap)
#define	FBIOGETCMAP	_IOW('F', 4, struct fbcmap)
#endif

struct netbsd32_fbcursor {
	short set;		/* what to set */
	short enable;		/* enable/disable cursor */
	struct fbcurpos pos;	/* cursor's position */
	struct fbcurpos hot;	/* cursor's hot spot */
	struct netbsd32_fbcmap cmap;	/* color map info */
	struct fbcurpos size;	/* cursor's bit map size */
	netbsd32_charp image;	/* cursor's image bits */
	netbsd32_charp mask;	/* cursor's mask bits */
};
#if 0
#define FBIOSCURSOR	_IOW('F', 24, struct fbcursor)
#define FBIOGCURSOR	_IOWR('F', 25, struct fbcursor)
#endif

/* from arch/sparc/include/openpromio.h */
struct netbsd32_opiocdesc {
	int	op_nodeid;		/* passed or returned node id */
	int	op_namelen;		/* length of op_name */
	netbsd32_charp op_name;		/* pointer to field name */
	int	op_buflen;		/* length of op_buf (value-result) */
	netbsd32_charp op_buf;		/* pointer to field value */
};
#if 0
#define	OPIOCGET	_IOWR('O', 1, struct opiocdesc) /* get openprom field */
#define	OPIOCSET	_IOW('O', 2, struct opiocdesc) /* set openprom field */
#define	OPIOCNEXTPROP	_IOWR('O', 3, struct opiocdesc) /* get next property */
#endif

/* from <sys/audioio.h> */
#if 0
#define AUDIO_WSEEK	_IOR('A', 25, u_long)
#endif

/* from <sys/dkio.h> */
typedef int32_t netbsd32_disklabel_tp_t;
typedef int32_t netbsd32_partition_tp_t;
struct netbsd32_partinfo {
	netbsd32_disklabel_tp_t disklab;
	netbsd32_partition_tp_t part;
};
#if 0
#define DIOCGPART	_IOW('d', 104, struct partinfo)	/* get partition */
#endif

struct netbsd32_format_op {
	netbsd32_charp df_buf;
	int	 df_count;		/* value-result */
	daddr_t	 df_startblk;
	int	 df_reg[8];		/* result */
};
#if 0
#define DIOCRFORMAT	_IOWR('d', 105, struct format_op)
#define DIOCWFORMAT	_IOWR('d', 106, struct format_op)
#endif

/* can wait! */
#if 0
dev/ccdvar.h:219:#define CCDIOCSET	_IOWR('F', 16, struct ccd_ioctl)   /* enable ccd */
dev/ccdvar.h:220:#define CCDIOCCLR	_IOW('F', 17, struct ccd_ioctl)    /* disable ccd */

dev/md.h:45:#define MD_GETCONF	_IOR('r', 0, struct md_conf)	/* get unit config */
dev/md.h:46:#define MD_SETCONF	_IOW('r', 1, struct md_conf)	/* set unit config */

dev/wscons/wsconsio.h:133:#define WSKBDIO_GETMAP		_IOWR('W', 13, struct wskbd_map_data)
dev/wscons/wsconsio.h:134:#define WSKBDIO_SETMAP		_IOW('W', 14, struct wskbd_map_data)

dev/wscons/wsconsio.h:188:#define WSDISPLAYIO_GETCMAP	_IOW('W', 66, struct wsdisplay_cmap)
dev/wscons/wsconsio.h:189:#define WSDISPLAYIO_PUTCMAP	_IOW('W', 67, struct wsdisplay_cmap)

dev/wscons/wsconsio.h:227:#define	WSDISPLAYIO_GCURSOR	_IOWR('W', 73, struct wsdisplay_cursor)
dev/wscons/wsconsio.h:228:#define	WSDISPLAYIO_SCURSOR	_IOW('W', 74, struct wsdisplay_cursor)

dev/wscons/wsconsio.h:241:#define WSDISPLAYIO_SFONT	_IOW('W', 77, struct wsdisplay_font)

net/bpf.h:127:#define	BIOCSETF	_IOW('B',103, struct bpf_program)
net/bpf.h:138:#define BIOCSTCPF	_IOW('B',114, struct bpf_program)
net/bpf.h:139:#define BIOCSUDPF	_IOW('B',115, struct bpf_program)
net/if_ppp.h:110:#define PPPIOCSPASS	_IOW('t', 71, struct bpf_program) /* set pass filter */
net/if_ppp.h:111:#define PPPIOCSACTIVE	_IOW('t', 70, struct bpf_program) /* set active filt */

net/if_atm.h:88:#define SIOCATMENA	_IOWR('a', 123, struct atm_pseudoioctl) /* enable */
net/if_atm.h:89:#define SIOCATMDIS	_IOWR('a', 124, struct atm_pseudoioctl) /* disable */

net/if_ppp.h:105:#define PPPIOCSCOMPRESS	_IOW('t', 77, struct ppp_option_data)

netccitt/x25.h:157:#define	SIOCSIFCONF_X25	_IOW('i', 12, struct ifreq_x25)	/* set ifnet config */
netccitt/x25.h:158:#define	SIOCGIFCONF_X25	_IOWR('i',13, struct ifreq_x25)	/* get ifnet config */

netinet/ip_fil.h:46:#define	SIOCGETFS	_IOR('r', 64, struct friostat)
netinet/ip_fil.h:56:#define	SIOCFRZST	_IOWR('r', 74, struct friostat)

netinet/ip_fil.h:42:#define	SIOCADAFR	_IOW('r', 60, struct frentry)
netinet/ip_fil.h:43:#define	SIOCRMAFR	_IOW('r', 61, struct frentry)
netinet/ip_fil.h:49:#define	SIOCADIFR	_IOW('r', 67, struct frentry)
netinet/ip_fil.h:50:#define	SIOCRMIFR	_IOW('r', 68, struct frentry)
netinet/ip_fil.h:52:#define	SIOCINAFR	_IOW('r', 70, struct frentry)
netinet/ip_fil.h:53:#define	SIOCINIFR	_IOW('r', 71, struct frentry)
netinet/ip_fil.h:57:#define	SIOCZRLST	_IOWR('r', 75, struct frentry)

netinet/ip_fil.h:78:#define	SIOCAUTHW	_IOWR(r, 76, struct fr_info)
netinet/ip_fil.h:79:#define	SIOCAUTHR	_IOWR(r, 77, struct fr_info)

netinet/ip_fil.h:60:#define	SIOCATHST	_IOWR('r', 78, struct fr_authstat)

netinet/ip_nat.h:22:#define	SIOCADNAT	_IOW('r', 80, struct ipnat)
netinet/ip_nat.h:23:#define	SIOCRMNAT	_IOW('r', 81, struct ipnat)

netinet/ip_nat.h:24:#define	SIOCGNATS	_IOR('r', 82, struct natstat)

netinet/ip_nat.h:25:#define	SIOCGNATL	_IOWR('r', 83, struct natlookup)

netinet/ip_nat.h:26:#define SIOCGFRST	_IOR('r', 84, struct ipfrstat)

netinet/ip_nat.h:27:#define SIOCGIPST	_IOR('r', 85, struct ips_stat)

sys/lkm.h:286:#define	LMRESERV	_IOWR('K', 0, struct lmc_resrv)

sys/lkm.h:287:#define	LMLOADBUF	_IOW('K', 1, struct lmc_loadbuf)

sys/lkm.h:291:#define	LMLOAD		_IOW('K', 9, struct lmc_load)

sys/lkm.h:292:#define	LMUNLOAD	_IOWR('K', 10, struct lmc_unload)

sys/lkm.h:293:#define	LMSTAT		_IOWR('K', 11, struct lmc_stat)

sys/rnd.h:186:#define RNDGETPOOL      _IOR('R',  103, u_char *)  /* get whole pool */

sys/scanio.h:86:#define SCIOCGET	_IOR('S', 1, struct scan_io) /* retrieve parameters */
sys/scanio.h:87:#define SCIOCSET	_IOW('S', 2, struct scan_io) /* set parameters */

sys/scsiio.h:43:#define SCIOCCOMMAND	_IOWR('Q', 1, scsireq_t)
#endif

/* from <net/if.h> */

typedef int32_t netbsd32_ifreq_tp_t;
/*
 * note that ifr_data is the only one that needs to be changed
 */
struct	netbsd32_ifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		int	ifru_mtu;
		netbsd32_caddr_t	ifru_data;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu */
#define	ifr_media	ifr_ifru.ifru_metric	/* media options (overload) */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
};
#if 0
/* from <dev/pci/if_devar.h> */
#define	SIOCGADDRROM		_IOW('i', 240, struct ifreq)	/* get 128 bytes of ROM */
#define	SIOCGCHIPID		_IOWR('i', 241, struct ifreq)	/* get chipid */
/* from <sys/sockio.h> */
#define	SIOCSIFADDR	 _IOW('i', 12, struct ifreq)	/* set ifnet address */
#define	OSIOCGIFADDR	_IOWR('i', 13, struct ifreq)	/* get ifnet address */
#define	SIOCGIFADDR	_IOWR('i', 33, struct ifreq)	/* get ifnet address */
#define	SIOCSIFDSTADDR	 _IOW('i', 14, struct ifreq)	/* set p-p address */
#define	OSIOCGIFDSTADDR	_IOWR('i', 15, struct ifreq)	/* get p-p address */
#define	SIOCGIFDSTADDR	_IOWR('i', 34, struct ifreq)	/* get p-p address */
#define	SIOCSIFFLAGS	 _IOW('i', 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR('i', 17, struct ifreq)	/* get ifnet flags */
#define	OSIOCGIFBRDADDR	_IOWR('i', 18, struct ifreq)	/* get broadcast addr */
#define	SIOCGIFBRDADDR	_IOWR('i', 35, struct ifreq)	/* get broadcast addr */
#define	SIOCSIFBRDADDR	 _IOW('i', 19, struct ifreq)	/* set broadcast addr */
#define	OSIOCGIFNETMASK	_IOWR('i', 21, struct ifreq)	/* get net addr mask */
#define	SIOCGIFNETMASK	_IOWR('i', 37, struct ifreq)	/* get net addr mask */
#define	SIOCSIFNETMASK	 _IOW('i', 22, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR('i', 23, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	 _IOW('i', 24, struct ifreq)	/* set IF metric */
#define	SIOCDIFADDR	 _IOW('i', 25, struct ifreq)	/* delete IF addr */
#define	SIOCADDMULTI	 _IOW('i', 49, struct ifreq)	/* add m'cast addr */
#define	SIOCDELMULTI	 _IOW('i', 50, struct ifreq)	/* del m'cast addr */
#define	SIOCSIFMEDIA	_IOWR('i', 53, struct ifreq)	/* set net media */
#define	SIOCSIFMTU	 _IOW('i', 127, struct ifreq)	/* set ifnet mtu */
#define	SIOCGIFMTU	_IOWR('i', 126, struct ifreq)	/* get ifnet mtu */
#define	SIOCSIFASYNCMAP  _IOW('i', 125, struct ifreq)	/* set ppp asyncmap */
#define	SIOCGIFASYNCMAP _IOWR('i', 124, struct ifreq)	/* get ppp asyncmap */
/* from <net/bpf.h> */
#define BIOCGETIF	_IOR('B',107, struct ifreq)
#define BIOCSETIF	_IOW('B',108, struct ifreq)
/* from <netatalk/phase2.h> */
#define SIOCPHASE1	_IOW('i', 100, struct ifreq)	/* AppleTalk phase 1 */
#define SIOCPHASE2	_IOW('i', 101, struct ifreq)	/* AppleTalk phase 2 */
#endif

/* from <net/if.h> */
struct	netbsd32_ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		netbsd32_caddr_t	ifcu_buf;
		netbsd32_ifreq_tp_t ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};
#if 0
/* from <sys/sockio.h> */
#define	OSIOCGIFCONF	_IOWR('i', 20, struct ifconf)	/* get ifnet list */
#define	SIOCGIFCONF	_IOWR('i', 36, struct ifconf)	/* get ifnet list */
#endif

/* from <net/if.h> */
struct netbsd32_ifmediareq {
	char	ifm_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	int	ifm_current;			/* current media options */
	int	ifm_mask;			/* don't care mask */
	int	ifm_status;			/* media status */
	int	ifm_active;			/* active options */
	int	ifm_count;			/* # entries in ifm_ulist
						   array */
	netbsd32_intp	ifm_ulist;		/* media words */
};
#if 0
/* from <sys/sockio.h> */
#define	SIOCGIFMEDIA	_IOWR('i', 54, struct ifmediareq) /* get net media */
#endif

/* from <net/if.h> */
struct  netbsd32_ifdrv {
	char		ifd_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	unsigned long	ifd_cmd;
	size_t		ifd_len;
	void		*ifd_data;
};
#if 0
/* from <sys/sockio.h> */
#define SIOCSDRVSPEC     _IOW('i', 123, struct ifdrv)   /* set driver-specific */
#endif

/* from <netinet/ip_mroute.h> */
struct netbsd32_sioc_vif_req {
	vifi_t	vifi;			/* vif number */
	netbsd32_u_long	icount;		/* input packet count on vif */
	netbsd32_u_long	ocount;		/* output packet count on vif */
	netbsd32_u_long	ibytes;		/* input byte count on vif */
	netbsd32_u_long	obytes;		/* output byte count on vif */
};
#if 0
/* from <sys/sockio.h> */
#define	SIOCGETVIFCNT	_IOWR('u', 51, struct sioc_vif_req)/* vif pkt cnt */
#endif

struct netbsd32_sioc_sg_req {
	struct	in_addr src;
	struct	in_addr grp;
	u_long	pktcnt;
	u_long	bytecnt;
	u_long	wrong_if;
};
#if 0
/* from <sys/sockio.h> */
#define	SIOCGETSGCNT	_IOWR('u', 52, struct sioc_sg_req) /* sg pkt cnt */
#endif
