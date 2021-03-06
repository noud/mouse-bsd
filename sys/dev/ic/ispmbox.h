/* $NetBSD: ispmbox.h,v 1.21 2000/02/12 02:26:26 mjacob Exp $ */
/*
 * Copyright (C) 1997, 1998, 1999 National Aeronautics & Space Administration
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
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Mailbox and Queue Entry Definitions for for Qlogic ISP SCSI adapters.
 * <mjacob@nas.nasa.gov>
 */
#ifndef	_ISPMBOX_H
#define	_ISPMBOX_H

/*
 * Mailbox Command Opcodes
 */

#define MBOX_NO_OP			0x0000
#define MBOX_LOAD_RAM			0x0001
#define MBOX_EXEC_FIRMWARE		0x0002
#define MBOX_DUMP_RAM			0x0003
#define MBOX_WRITE_RAM_WORD		0x0004
#define MBOX_READ_RAM_WORD		0x0005
#define MBOX_MAILBOX_REG_TEST		0x0006
#define MBOX_VERIFY_CHECKSUM		0x0007
#define MBOX_ABOUT_FIRMWARE		0x0008
					/*   9 */
					/*   a */
					/*   b */
					/*   c */
					/*   d */
#define MBOX_CHECK_FIRMWARE		0x000e
					/*   f */
#define MBOX_INIT_REQ_QUEUE		0x0010
#define MBOX_INIT_RES_QUEUE		0x0011
#define MBOX_EXECUTE_IOCB		0x0012
#define MBOX_WAKE_UP			0x0013
#define MBOX_STOP_FIRMWARE		0x0014
#define MBOX_ABORT			0x0015
#define MBOX_ABORT_DEVICE		0x0016
#define MBOX_ABORT_TARGET		0x0017
#define MBOX_BUS_RESET			0x0018
#define MBOX_STOP_QUEUE			0x0019
#define MBOX_START_QUEUE		0x001a
#define MBOX_SINGLE_STEP_QUEUE		0x001b
#define MBOX_ABORT_QUEUE		0x001c
#define MBOX_GET_DEV_QUEUE_STATUS	0x001d
					/*  1e */
#define MBOX_GET_FIRMWARE_STATUS	0x001f
#define MBOX_GET_INIT_SCSI_ID		0x0020
#define MBOX_GET_SELECT_TIMEOUT		0x0021
#define MBOX_GET_RETRY_COUNT		0x0022
#define MBOX_GET_TAG_AGE_LIMIT		0x0023
#define MBOX_GET_CLOCK_RATE		0x0024
#define MBOX_GET_ACT_NEG_STATE		0x0025
#define MBOX_GET_ASYNC_DATA_SETUP_TIME	0x0026
#define MBOX_GET_SBUS_PARAMS		0x0027
#define MBOX_GET_TARGET_PARAMS		0x0028
#define MBOX_GET_DEV_QUEUE_PARAMS	0x0029
#define	MBOX_GET_RESET_DELAY_PARAMS	0x002a
					/*  2b */
					/*  2c */
					/*  2d */
					/*  2e */
					/*  2f */
#define MBOX_SET_INIT_SCSI_ID		0x0030
#define MBOX_SET_SELECT_TIMEOUT		0x0031
#define MBOX_SET_RETRY_COUNT		0x0032
#define MBOX_SET_TAG_AGE_LIMIT		0x0033
#define MBOX_SET_CLOCK_RATE		0x0034
#define MBOX_SET_ACT_NEG_STATE		0x0035
#define MBOX_SET_ASYNC_DATA_SETUP_TIME	0x0036
#define MBOX_SET_SBUS_CONTROL_PARAMS	0x0037
#define		MBOX_SET_PCI_PARAMETERS	0x0037
#define MBOX_SET_TARGET_PARAMS		0x0038
#define MBOX_SET_DEV_QUEUE_PARAMS	0x0039
#define	MBOX_SET_RESET_DELAY_PARAMS	0x003a
					/*  3b */
					/*  3c */
					/*  3d */
					/*  3e */
					/*  3f */
#define	MBOX_RETURN_BIOS_BLOCK_ADDR	0x0040
#define	MBOX_WRITE_FOUR_RAM_WORDS	0x0041
#define	MBOX_EXEC_BIOS_IOCB		0x0042
#define	MBOX_SET_FW_FEATURES		0x004a
#define	MBOX_GET_FW_FEATURES		0x004b
#define		FW_FEATURE_LVD_NOTIFY	0x2
#define		FW_FEATURE_FAST_POST	0x1

#define	MBOX_ENABLE_TARGET_MODE		0x55
#define		ENABLE_TARGET_FLAG	0x8000

/* These are for the ISP2100 FC cards */
#define	MBOX_GET_LOOP_ID		0x20
#define	MBOX_EXEC_COMMAND_IOCB_A64	0x54
#define	MBOX_INIT_FIRMWARE		0x60
#define	MBOX_GET_INIT_CONTROL_BLOCK	0x61
#define	MBOX_INIT_LIP			0x62
#define	MBOX_GET_FC_AL_POSITION_MAP	0x63
#define	MBOX_GET_PORT_DB		0x64
#define	MBOX_CLEAR_ACA			0x65
#define	MBOX_TARGET_RESET		0x66
#define	MBOX_CLEAR_TASK_SET		0x67
#define	MBOX_ABORT_TASK_SET		0x68
#define	MBOX_GET_FW_STATE		0x69
#define	MBOX_GET_PORT_NAME		0x6a
#define	MBOX_GET_LINK_STATUS		0x6b
#define	MBOX_INIT_LIP_RESET		0x6c
#define	MBOX_SEND_SNS			0x6e
#define	MBOX_FABRIC_LOGIN		0x6f
#define	MBOX_SEND_CHANGE_REQUEST	0x70
#define	MBOX_FABRIC_LOGOUT		0x71
#define	MBOX_INIT_LIP_LOGIN		0x72

#define	ISP2100_SET_PCI_PARAM		0x00ff

#define	MBOX_BUSY			0x04

typedef struct {
	u_int16_t param[8];
} mbreg_t;

/*
 * Mailbox Command Complete Status Codes
 */
#define	MBOX_COMMAND_COMPLETE		0x4000
#define	MBOX_INVALID_COMMAND		0x4001
#define	MBOX_HOST_INTERFACE_ERROR	0x4002
#define	MBOX_TEST_FAILED		0x4003
#define	MBOX_COMMAND_ERROR		0x4005
#define	MBOX_COMMAND_PARAM_ERROR	0x4006

/*
 * Asynchronous event status codes
 */
#define	ASYNC_BUS_RESET			0x8001
#define	ASYNC_SYSTEM_ERROR		0x8002
#define	ASYNC_RQS_XFER_ERR		0x8003
#define	ASYNC_RSP_XFER_ERR		0x8004
#define	ASYNC_QWAKEUP			0x8005
#define	ASYNC_TIMEOUT_RESET		0x8006
#define	ASYNC_DEVICE_RESET		0x8007
#define	ASYNC_EXTMSG_UNDERRUN		0x800A
#define	ASYNC_SCAM_INT			0x800B
#define	ASYNC_HUNG_SCSI			0x800C
#define	ASYNC_KILLED_BUS		0x800D
#define	ASYNC_BUS_TRANSIT		0x800E	/* LVD -> HVD, eg. */
#define	ASYNC_CMD_CMPLT			0x8020
#define	ASYNC_CTIO_DONE			0x8021

/* for ISP2100 only */
#define	ASYNC_LIP_OCCURRED		0x8010
#define	ASYNC_LOOP_UP			0x8011
#define	ASYNC_LOOP_DOWN			0x8012
#define	ASYNC_LOOP_RESET		0x8013
#define	ASYNC_PDB_CHANGED		0x8014
#define	ASYNC_CHANGE_NOTIFY		0x8015

/* for ISP2200 only */
#define	ASYNC_PTPMODE			0x8030
#define	ASYNC_CONNMODE			0x8036
#define		ISP_CONN_LOOP		1
#define		ISP_CONN_PTP		2
#define		ISP_CONN_BADLIP		3
#define		ISP_CONN_FATAL		4
#define		ISP_CONN_LOOPBACK	5

/*
 * Command Structure Definitions
 */

typedef struct {
	u_int32_t	ds_base;
	u_int32_t	ds_count;
} ispds_t;

#define	_ISP_SWAP8(a, b)	{	\
	u_int8_t tmp;			\
	tmp = a;			\
	a = b;				\
	b = tmp;			\
}

/*
 * These elements get swizzled around for SBus instances.
 */
typedef struct {
	u_int8_t	rqs_entry_type;
	u_int8_t	rqs_entry_count;
	u_int8_t	rqs_seqno;
	u_int8_t	rqs_flags;
} isphdr_t;
/*
 * There are no (for all intents and purposes) non-sparc SBus machines
 */
#ifdef	__sparc__
#define	ISP_SBUSIFY_ISPHDR(isp, hdrp)					\
    if ((isp)->isp_bustype == ISP_BT_SBUS) {				\
	_ISP_SWAP8((hdrp)->rqs_entry_count, (hdrp)->rqs_entry_type);	\
	_ISP_SWAP8((hdrp)->rqs_flags, (hdrp)->rqs_seqno);		\
    }
#else
#define	ISP_SBUSIFY_ISPHDR(a, b)
#endif

/* RQS Flag definitions */
#define	RQSFLAG_CONTINUATION	0x01
#define	RQSFLAG_FULL		0x02
#define	RQSFLAG_BADHEADER	0x04
#define	RQSFLAG_BADPACKET	0x08

/* RQS entry_type definitions */
#define	RQSTYPE_REQUEST		0x01
#define	RQSTYPE_DATASEG		0x02
#define	RQSTYPE_RESPONSE	0x03
#define	RQSTYPE_MARKER		0x04
#define	RQSTYPE_CMDONLY		0x05
#define	RQSTYPE_ATIO		0x06	/* Target Mode */
#define	RQSTYPE_CTIO		0x07	/* Target Mode */
#define	RQSTYPE_SCAM		0x08
#define	RQSTYPE_A64		0x09
#define	RQSTYPE_A64_CONT	0x0a
#define	RQSTYPE_ENABLE_LUN	0x0b	/* Target Mode */
#define	RQSTYPE_MODIFY_LUN	0x0c	/* Target Mode */
#define	RQSTYPE_NOTIFY		0x0d	/* Target Mode */
#define	RQSTYPE_NOTIFY_ACK	0x0e	/* Target Mode */
#define	RQSTYPE_CTIO1		0x0f	/* Target Mode */
#define	RQSTYPE_STATUS_CONT	0x10
#define	RQSTYPE_T2RQS		0x11

#define	RQSTYPE_T4RQS		0x15
#define	RQSTYPE_ATIO2		0x16
#define	RQSTYPE_CTIO2		0x17
#define	RQSTYPE_CSET0		0x18
#define	RQSTYPE_T3RQS		0x19

#define	RQSTYPE_CTIO3		0x1f


#define	ISP_RQDSEG	4
typedef struct {
	isphdr_t	req_header;
	u_int32_t	req_handle;
	u_int8_t	req_lun_trn;
	u_int8_t	req_target;
	u_int16_t	req_cdblen;
#define	req_modifier	req_cdblen	/* marker packet */
	u_int16_t	req_flags;
	u_int16_t	req_reserved;
	u_int16_t	req_time;
	u_int16_t	req_seg_count;
	u_int8_t	req_cdb[12];
	ispds_t		req_dataseg[ISP_RQDSEG];
} ispreq_t;

/*
 * A request packet can also be a marker packet.
 */
#define SYNC_DEVICE	0
#define SYNC_TARGET	1
#define SYNC_ALL	2

/*
 * There are no (for all intents and purposes) non-sparc SBus machines
 */
#ifdef	__sparc__
#define	ISP_SBUSIFY_ISPREQ(isp, rqp)					\
    if ((isp)->isp_bustype == ISP_BT_SBUS) {				\
	_ISP_SWAP8((rqp)->req_target, (rqp)->req_lun_trn);		\
    }
#else
#define	ISP_SBUSIFY_ISPREQ(a, b)
#endif

#define	ISP_RQDSEG_T2	3
typedef struct {
	isphdr_t	req_header;
	u_int32_t	req_handle;
	u_int8_t	req_lun_trn;
	u_int8_t	req_target;
	u_int16_t	req_scclun;
	u_int16_t	req_flags;
	u_int16_t	_res2;
	u_int16_t	req_time;
	u_int16_t	req_seg_count;
	u_int32_t	req_cdb[4];
	u_int32_t	req_totalcnt;
	ispds_t		req_dataseg[ISP_RQDSEG_T2];
} ispreqt2_t;

/* req_flag values */
#define	REQFLAG_NODISCON	0x0001
#define	REQFLAG_HTAG		0x0002
#define	REQFLAG_OTAG		0x0004
#define	REQFLAG_STAG		0x0008
#define	REQFLAG_TARGET_RTN	0x0010

#define	REQFLAG_NODATA		0x0000
#define	REQFLAG_DATA_IN		0x0020
#define	REQFLAG_DATA_OUT	0x0040
#define	REQFLAG_DATA_UNKNOWN	0x0060

#define	REQFLAG_DISARQ		0x0100
#define	REQFLAG_FRC_ASYNC	0x0200
#define	REQFLAG_FRC_SYNC	0x0400
#define	REQFLAG_FRC_WIDE	0x0800
#define	REQFLAG_NOPARITY	0x1000
#define	REQFLAG_STOPQ		0x2000
#define	REQFLAG_XTRASNS		0x4000
#define	REQFLAG_PRIORITY	0x8000

typedef struct {
	isphdr_t	req_header;
	u_int32_t	req_handle;
	u_int8_t	req_lun_trn;
	u_int8_t	req_target;
	u_int16_t	req_cdblen;
	u_int16_t	req_flags;
	u_int16_t	_res1;
	u_int16_t	req_time;
	u_int16_t	req_seg_count;
	u_int8_t	req_cdb[44];
} ispextreq_t;

#define	ISP_CDSEG	7
typedef struct {
	isphdr_t	req_header;
	u_int32_t	_res1;
	ispds_t		req_dataseg[ISP_CDSEG];
} ispcontreq_t;

typedef struct {
	isphdr_t	req_header;
	u_int32_t	req_handle;
	u_int16_t	req_scsi_status;
	u_int16_t	req_completion_status;
	u_int16_t	req_state_flags;
	u_int16_t	req_status_flags;
	u_int16_t	req_time;
#define	req_response_len	req_time	/* FC only */
	u_int16_t	req_sense_len;
	u_int32_t	req_resid;
	u_int8_t	_res1[8];
	u_int8_t	req_sense_data[32];
} ispstatusreq_t;

/*
 * For Qlogic 2100, the high order byte of SCSI status has
 * additional meaning.
 */
#define	RQCS_RU	0x800	/* Residual Under */
#define	RQCS_RO	0x400	/* Residual Over */
#define	RQCS_SV	0x200	/* Sense Length Valid */
#define	RQCS_RV	0x100	/* Residual Valid */

/*
 * Completion Status Codes.
 */
#define RQCS_COMPLETE			0x0000
#define RQCS_INCOMPLETE			0x0001
#define RQCS_DMA_ERROR			0x0002
#define RQCS_TRANSPORT_ERROR		0x0003
#define RQCS_RESET_OCCURRED		0x0004
#define RQCS_ABORTED			0x0005
#define RQCS_TIMEOUT			0x0006
#define RQCS_DATA_OVERRUN		0x0007
#define RQCS_COMMAND_OVERRUN		0x0008
#define RQCS_STATUS_OVERRUN		0x0009
#define RQCS_BAD_MESSAGE		0x000a
#define RQCS_NO_MESSAGE_OUT		0x000b
#define RQCS_EXT_ID_FAILED		0x000c
#define RQCS_IDE_MSG_FAILED		0x000d
#define RQCS_ABORT_MSG_FAILED		0x000e
#define RQCS_REJECT_MSG_FAILED		0x000f
#define RQCS_NOP_MSG_FAILED		0x0010
#define RQCS_PARITY_ERROR_MSG_FAILED	0x0011
#define RQCS_DEVICE_RESET_MSG_FAILED	0x0012
#define RQCS_ID_MSG_FAILED		0x0013
#define RQCS_UNEXP_BUS_FREE		0x0014
#define RQCS_DATA_UNDERRUN		0x0015
#define	RQCS_XACT_ERR1			0x0018
#define	RQCS_XACT_ERR2			0x0019
#define	RQCS_XACT_ERR3			0x001A
#define	RQCS_BAD_ENTRY			0x001B
#define	RQCS_QUEUE_FULL			0x001C
#define	RQCS_PHASE_SKIPPED		0x001D
#define	RQCS_ARQS_FAILED		0x001E
#define	RQCS_WIDE_FAILED		0x001F
#define	RQCS_SYNCXFER_FAILED		0x0020
#define	RQCS_LVD_BUSERR			0x0021

/* 2100 Only Completion Codes */
#define	RQCS_PORT_UNAVAILABLE		0x0028
#define	RQCS_PORT_LOGGED_OUT		0x0029
#define	RQCS_PORT_CHANGED		0x002A
#define	RQCS_PORT_BUSY			0x002B

/*
 * State Flags (not applicable to 2100)
 */
#define RQSF_GOT_BUS			0x0100
#define RQSF_GOT_TARGET			0x0200
#define RQSF_SENT_CDB			0x0400
#define RQSF_XFRD_DATA			0x0800
#define RQSF_GOT_STATUS			0x1000
#define RQSF_GOT_SENSE			0x2000
#define	RQSF_XFER_COMPLETE		0x4000

/*
 * Status Flags (not applicable to 2100)
 */
#define RQSTF_DISCONNECT		0x0001
#define RQSTF_SYNCHRONOUS		0x0002
#define RQSTF_PARITY_ERROR		0x0004
#define RQSTF_BUS_RESET			0x0008
#define RQSTF_DEVICE_RESET		0x0010
#define RQSTF_ABORTED			0x0020
#define RQSTF_TIMEOUT			0x0040
#define RQSTF_NEGOTIATION		0x0080

/*
 * FC (ISP2100) specific data structures
 */

/*
 * Initialization Control Block
 *
 * Version One (prime) format.
 */
typedef struct isp_icb {
	u_int8_t	icb_version;
	u_int8_t	_reserved0;
	u_int16_t	icb_fwoptions;
	u_int16_t	icb_maxfrmlen;
	u_int16_t	icb_maxalloc;
	u_int16_t	icb_execthrottle;
	u_int8_t	icb_retry_count;
	u_int8_t	icb_retry_delay;
	u_int8_t	icb_portname[8];
	u_int16_t	icb_hardaddr;
	u_int8_t	icb_iqdevtype;
	u_int8_t	icb_logintime;
	u_int8_t	icb_nodename[8];
	u_int16_t	icb_rqstout;
	u_int16_t	icb_rspnsin;
	u_int16_t	icb_rqstqlen;
	u_int16_t	icb_rsltqlen;
	u_int16_t	icb_rqstaddr[4];
	u_int16_t	icb_respaddr[4];
	u_int16_t	icb_lunenables;
	u_int8_t	icb_ccnt;
	u_int8_t	icb_icnt;
	u_int16_t	icb_lunetimeout;
	u_int16_t	_reserved1;
	u_int16_t	icb_xfwoptions;
	u_int8_t	icb_racctimer;
	u_int8_t	icb_idelaytimer;
	u_int16_t	icb_zfwoptions;
	u_int16_t	_reserved2[13];
} isp_icb_t;
#define	ICB_VERSION1	1

#define	ICBOPT_HARD_ADDRESS	0x0001
#define	ICBOPT_FAIRNESS		0x0002
#define	ICBOPT_FULL_DUPLEX	0x0004
#define	ICBOPT_FAST_POST	0x0008
#define	ICBOPT_TGT_ENABLE	0x0010
#define	ICBOPT_INI_DISABLE	0x0020
#define	ICBOPT_INI_ADISC	0x0040
#define	ICBOPT_INI_TGTTYPE	0x0080
#define	ICBOPT_PDBCHANGE_AE	0x0100
#define	ICBOPT_NOLIP		0x0200
#define	ICBOPT_SRCHDOWN		0x0400
#define	ICBOPT_PREVLOOP		0x0800
#define	ICBOPT_STOP_ON_QFULL	0x1000
#define	ICBOPT_FULL_LOGIN	0x2000
#define	ICBOPT_USE_PORTNAME	0x4000
#define	ICBOPT_EXTENDED		0x8000

#define	ICBXOPT_CLASS2_ACK0	0x0200
#define	ICBXOPT_CLASS2		0x0100
#define	ICBXOPT_LOOP_ONLY	(0 << 4)
#define	ICBXOPT_PTP_ONLY	(1 << 4)
#define	ICBXOPT_LOOP_2_PTP	(2 << 4)
#define	ICBXOPT_PTP_2_LOOP	(3 << 4)

#define	ICBXOPT_RIO_OFF		0
#define	ICBXOPT_RIO_16BIT	1
#define	ICBXOPT_RIO_32BIT	2
#define	ICBXOPT_RIO_16BIT_DELAY	3
#define	ICBXOPT_RIO_32BIT_DELAY	4



#define	ICB_MIN_FRMLEN		256
#define	ICB_MAX_FRMLEN		2112
#define	ICB_DFLT_FRMLEN		1024
#define	ICB_DFLT_ALLOC		256
#define	ICB_DFLT_THROTTLE	16
#define	ICB_DFLT_RDELAY		5
#define	ICB_DFLT_RCOUNT		3


#define	RQRSP_ADDR0015	0
#define	RQRSP_ADDR1631	1
#define	RQRSP_ADDR3247	2
#define	RQRSP_ADDR4863	3


#define	ICB_NNM0	7
#define	ICB_NNM1	6
#define	ICB_NNM2	5
#define	ICB_NNM3	4
#define	ICB_NNM4	3
#define	ICB_NNM5	2
#define	ICB_NNM6	1
#define	ICB_NNM7	0

#define	MAKE_NODE_NAME_FROM_WWN(array, wwn)	\
	array[ICB_NNM0] = (u_int8_t) ((wwn >>  0) & 0xff), \
	array[ICB_NNM1] = (u_int8_t) ((wwn >>  8) & 0xff), \
	array[ICB_NNM2] = (u_int8_t) ((wwn >> 16) & 0xff), \
	array[ICB_NNM3] = (u_int8_t) ((wwn >> 24) & 0xff), \
	array[ICB_NNM4] = (u_int8_t) ((wwn >> 32) & 0xff), \
	array[ICB_NNM5] = (u_int8_t) ((wwn >> 40) & 0xff), \
	array[ICB_NNM6] = (u_int8_t) ((wwn >> 48) & 0xff), \
	array[ICB_NNM7] = (u_int8_t) ((wwn >> 56) & 0xff)

/*
 * Port Data Base Element
 */

typedef struct {
	u_int16_t	pdb_options;
	u_int8_t	pdb_mstate;
	u_int8_t	pdb_sstate;
#define	BITS2WORD(x)	(x)[0] << 16 | (x)[3] << 8 | (x)[2]
	u_int8_t	pdb_hardaddr_bits[4];
	u_int8_t	pdb_portid_bits[4];
	u_int8_t	pdb_nodename[8];
	u_int8_t	pdb_portname[8];
	u_int16_t	pdb_execthrottle;
	u_int16_t	pdb_exec_count;
	u_int8_t	pdb_retry_count;
	u_int8_t	pdb_retry_delay;
	u_int16_t	pdb_resalloc;
	u_int16_t	pdb_curalloc;
	u_int16_t	pdb_qhead;
	u_int16_t	pdb_qtail;
	u_int16_t	pdb_tl_next;
	u_int16_t	pdb_tl_last;
	u_int16_t	pdb_features;	/* PLOGI, Common Service */
	u_int16_t	pdb_pconcurrnt;	/* PLOGI, Common Service */
	u_int16_t	pdb_roi;	/* PLOGI, Common Service */
	u_int8_t	pdb_target;
	u_int8_t	pdb_initiator;	/* PLOGI, Class 3 Control Flags */
	u_int16_t	pdb_rdsiz;	/* PLOGI, Class 3 */
	u_int16_t	pdb_ncseq;	/* PLOGI, Class 3 */
	u_int16_t	pdb_noseq;	/* PLOGI, Class 3 */
	u_int16_t	pdb_labrtflg;
	u_int16_t	pdb_lstopflg;
	u_int16_t	pdb_sqhead;
	u_int16_t	pdb_sqtail;
	u_int16_t	pdb_ptimer;
	u_int16_t	pdb_nxt_seqid;
	u_int16_t	pdb_fcount;
	u_int16_t	pdb_prli_len;
	u_int16_t	pdb_prli_svc0;
	u_int16_t	pdb_prli_svc3;
	u_int16_t	pdb_loopid;
	u_int16_t	pdb_il_ptr;
	u_int16_t	pdb_sl_ptr;
} isp_pdb_t;

#define	PDB_OPTIONS_XMITTING	(1<<11)
#define	PDB_OPTIONS_LNKXMIT	(1<<10)
#define	PDB_OPTIONS_ABORTED	(1<<9)
#define	PDB_OPTIONS_ADISC	(1<<1)

#define	PDB_STATE_DISCOVERY	0
#define	PDB_STATE_WDISC_ACK	1
#define	PDB_STATE_PLOGI		2
#define	PDB_STATE_PLOGI_ACK	3
#define	PDB_STATE_PRLI		4
#define	PDB_STATE_PRLI_ACK	5
#define	PDB_STATE_LOGGED_IN	6
#define	PDB_STATE_PORT_UNAVAIL	7
#define	PDB_STATE_PRLO		8
#define	PDB_STATE_PRLO_ACK	9
#define	PDB_STATE_PLOGO		10
#define	PDB_STATE_PLOG_ACK	11

#define		SVC3_TGT_ROLE		0x10
#define 	SVC3_INI_ROLE		0x20
#define			SVC3_ROLE_MASK	0x30
#define			SVC3_ROLE_SHIFT	4

#define	SNS_GAN	0x100
#define	SNS_GP3	0x171
typedef struct {
	u_int16_t	snscb_rblen;	/* response buffer length (words) */
	u_int16_t	snscb_res0;
	u_int16_t	snscb_addr[4];	/* response buffer address */
	u_int16_t	snscb_sblen;	/* subcommand buffer length (words) */
	u_int16_t	snscb_res1;
	u_int16_t	snscb_data[1];	/* variable data */
} sns_screq_t;	/* Subcommand Request Structure */
#define	SNS_GAN_REQ_SIZE	(sizeof (sns_screq_t)+(5*(sizeof (u_int16_t))))
#define	SNS_GP3_REQ_SIZE	(sizeof (sns_screq_t)+(5*(sizeof (u_int16_t))))

typedef struct {
	u_int8_t	snscb_cthdr[16];
	u_int8_t	snscb_port_type;
	u_int8_t	snscb_port_id[3];
	u_int8_t	snscb_portname[8];
	u_int16_t	snscb_data[1];	/* variable data */
} sns_scrsp_t;	/* Subcommand Response Structure */
#define	SNS_GAN_RESP_SIZE	608	/* Maximum response size (bytes) */
#define	SNS_GP3_RESP_SIZE	532	/* XXX: For 128 ports */

#endif	/* _ISPMBOX_H */
