/*	$NetBSD: cyvar.h,v 1.5 1999/09/09 21:52:11 tron Exp $	*/

/*
 * cy_var.h
 *
 * Driver for Cyclades Cyclom-8/16/32 multiport serial cards
 * (currently not tested with Cyclom-32 cards)
 *
 * Timo Rossi, 1996
 *
 * Supports both ISA and PCI Cyclom cards
 */

/* #define CY_DEBUG */
#define CY_DEBUG1

/*
 * Maximum number of ports per card
 */
#define	CY_MAX_PORTS		(CD1400_NO_OF_CHANNELS * CY_MAX_CD1400s)

#define CY_RX_FIFO_THRESHOLD  6

/*
 * Automatic RTS (or actually DTR, the RTS and DTR lines need to be
 * exchanged) handshake threshold used if CY_HW_RTS is defined
 */
#define CY_RX_DTR_THRESHOLD   9

/*
 * Port number on card encoded in low 5 bits
 * card number in next 2 bits (only space for 4 cards)
 * high bit reserved for dialout flag
 */
#define CY_PORT(x) (minor(x) & 0x1f)
#define CY_CARD(x) ((minor(x) >> 5) & 3)
#define CY_DIALOUT(x) ((minor(x) & 0x80) != 0)
#define CY_DIALIN(x) (!CY_DIALOUT(x))

/*
 * read/write cd1400 registers (when sc_softc-structure is available)
 */
#define cd_read_reg(sc,chip,reg) bus_space_read_1(sc->sc_memt, \
    sc->sc_bsh, sc->sc_cd1400_offs[chip] + \
    (((reg << 1)) << sc->sc_bustype))

#define cd_write_reg(sc,chip,reg,val) bus_space_write_1(sc->sc_memt, \
    sc->sc_bsh, sc->sc_cd1400_offs[chip] + \
    (((reg << 1))<< sc->sc_bustype), (val))

/*
 * ibuf is a simple ring buffer. It is always used two
 * bytes at a time (status and data)
 */
#define CY_IBUF_SIZE (2*512)

/* software state for one port */
struct cy_port {
	int             cy_port_num;
	int             cy_chip;
	int             cy_clock;
	struct tty     *cy_tty;
	int             cy_openflags;
	int             cy_fifo_overruns;
	int             cy_ibuf_overruns;
	u_char          cy_channel_control;	/* last CCR channel control
						 * command bits */
	u_char          cy_carrier_stat;	/* copied from MSVR2 */
	u_char          cy_flags;
	u_char         *cy_ibuf, *cy_ibuf_end;
	u_char         *cy_ibuf_rd_ptr, *cy_ibuf_wr_ptr;
#ifdef CY_DEBUG1
	int             cy_rx_int_count;
	int             cy_tx_int_count;
	int             cy_modem_int_count;
	int             cy_start_count;
#endif /* CY_DEBUG1 */
};

#define CY_F_CARRIER_CHANGED  0x01
#define CY_F_START_BREAK      0x02
#define CY_F_END_BREAK        0x04
#define CY_F_STOP             0x08
#define CY_F_SEND_NUL         0x10
#define CY_F_START            0x20

/* software state for one card */
struct cy_softc {
	struct device   sc_dev;
	void           *sc_ih;
	bus_space_tag_t sc_memt;
	bus_space_handle_t sc_bsh;
	int             sc_bustype;
	int		sc_nchips;	/* Number of cd1400's on this card */
	int             sc_cd1400_offs[CY_MAX_CD1400s];
	struct cy_port  sc_ports[CY_MAX_PORTS];
#ifdef CY_DEBUG1
	int             sc_poll_count1;
	int             sc_poll_count2;
#endif
};

int cy_find __P((struct cy_softc *));
void cy_attach __P((struct device *, struct device *, void *));
int cy_intr __P((void *));
