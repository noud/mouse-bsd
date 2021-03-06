/*	$NetBSD: scsipi_debug.h,v 1.12 1998/02/13 08:28:53 enami Exp $	*/

/*
 * Written by Julian Elischer (julian@tfs.com)
 */

/*
 * These are the new debug bits.  (Sat Oct  2 12:46:46 WST 1993)
 * the following DEBUG bits are defined to exist in the flags word of
 * the scsi_link structure.
 */
#define	SDEV_DB1		0x10	/* scsi commands, errors, data */
#define	SDEV_DB2		0x20	/* routine flow tracking */
#define	SDEV_DB3		0x40	/* internal to routine flows */
#define	SDEV_DB4		0x80	/* level 4 debugging for this dev */

/* type, target and LUN we want to debug */
#define DEBUGTYPE	BUS_ATAPI
#define	DEBUGTARGET	-1		/* -1 = disable. This is the drive
					   number for ATAPI */
#define	DEBUGLUN	0
#define	DEBUGLEVEL	(SDEV_DB1|SDEV_DB2|SDEV_DB3)

/*
 * This is the usual debug macro for use with the above bits
 */
#ifdef SCSIDEBUG
#define	SC_DEBUG(sc_link,Level,Printstuff)				\
do {									\
	if ((sc_link)->flags & (Level)) {				\
		sc_link->sc_print_addr(sc_link);			\
 		printf Printstuff;					\
	}								\
} while (0)

#define	SC_DEBUGN(sc_link,Level,Printstuff) 				\
do {									\
	if ((sc_link)->flags & (Level)) {				\
 		printf Printstuff;					\
	}								\
} while (0)
#else
#define SC_DEBUG(A,B,C)
#define SC_DEBUGN(A,B,C)
#endif
