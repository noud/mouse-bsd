#ifndef _LPVIIO_H_5f3adef3_
#define _LPVIIO_H_5f3adef3_

/* This file is in the public domain. */

#include <sys/ioccom.h>

/* LPVIIOC_GSIZE argument structure */
struct lpvi_pagesize {
  int lp_wid;
  int lp_hgt;
  } ;

/* LPVIIOC_{G,S}SETUP argument structure */
/*
 * For _SSETUP, LPVI_NOCHANGE means "no change", except for the
 *  lp_default field, for which only zero/nonzero matters.
 *
 * Setting a structure with lp_default nonzero ignores all other values
 *  and resets to the default state, where lp_wid and lp_hgt track the
 *  paper size (as can be fetched with LPVIIOC_GSIZE) and xoff and yoff
 *  are zero.  Setting a structure with lp_default zero and lp_dpi set
 *  to 300 or 400 will also have the effect of setting the resolution,
 *  as if with LPVIIOC_SRES.  Setting the resolution with LPVIIOC_SRES
 *  after setting a non-default page setup will cause the non-dpi
 *  members to be converted so as to keep the positions on the page
 *  within a pixel of their previous values.  (The original structure
 *  is remembered, and setting the resolution back will resurrect the
 *  original values rather than re-converting the converted values.)
 *  Changing the resolution with LPVIIOC_SSETUP while leaving the other
 *  four geometry values alone (by using LPVI_NOCHANGE) is identical in
 *  effect to LPVIIOC_SRES; however, if any of the four geometry values
 *  is not LPVI_NOCHANGE, the whole "original structure" will be loaded
 *  with the converted values, as modified by the overridden values
 *  from the structure.
 *
 * Trying to set a page setup with lp_xoff or lp_yoff negative (except
 *  for LPVI_NOCHANGE values, if LPVI_NOCHANGE is negative), or with
 *  lp_xoff+lp_wid or lp_yoff+lp_hgt too great for the relevant
 *  resolution and paper size, returns an error and no changes are
 *  made.
 *
 * User code should not depend on anything about LPVI_NOCHANGE except
 *  that (a) it can be stored in an int and (b) it is a value that
 *  cannot otherwise be valid for any of the fields.  (For example, it
 *  is not safe to assume it is negative; while it presently is, that
 *  may change in a future version, such as to permit doing something
 *  sensible with small negative lp_xoff and lp_yoff values.)
 */
struct lpvi_pagesetup {
  int lp_dpi;
  int lp_wid;
  int lp_hgt;
  int lp_xoff;
  int lp_yoff;
  int lp_default;
  } ;
#define LPVI_NOCHANGE (-1)

/*
 * Argument bits for LPVIIOC_SDISP.  Argument is an unsigned int; it
 *  breaks down thus:
 *
 *	iiiiiiiiiiiiiiiii ddddddd ddddddd c
 *
 * The iii bits are ignored; for future compatability, they should be
 *  zero.  If c is zero, the other 14 bits are ignored, and the display
 *  is set to be driven by the printer.  If c is one, each "dddddddd"
 *  set of seven bits drives one display digit.
 */
#define LPVID_CUSTOM 0x00000001 /* c */
#define LPVID_DATAMASK 0x7f /* ddddddd */
#define LPVID_A_SHF 8 /* shift for digit A data bits */
#define LPVID_B_SHF 1 /* shift for digit B data bits */

/* Argument bits for LPVIIOC_{G,S}MSGS */
#define LPVIMSG_EPNT 0x00000001 /* major driver entry points */
#define LPVIMSG_CMD  0x00000002 /* individual commands to printer */
#define LPVIMSG_INTR 0x00000004 /* interrupt-time details */

/* Argument bits for LPVIIOC_{G,S}FLAGS */
/*
 * By default, the margin (the area outside the wid/hgt/xoff/yoff
 *  rectangle) is white (unprinted); if LPVIF_MARGIN_INV is set, it's
 *  black (printed).  By default, 0 data bits print white and 1 data
 *  bits print black; if LPVIF_IMAGE_INV is set, these are inverted.
 */
#define LPVIF_IMAGE_INV  0x00000001
#define LPVIF_MARGIN_INV 0x00000002

/* Check that the printer is connected and alive */
#define LPVIIOC_PING _IO('V',0)

/* Reset everything */
#define LPVIIOC_RESET _IO('V',1)

/* Get the current resolution setting (300 or 400) */
#define LPVIIOC_GRES _IOR('V',2,int)

/* Set the resolution (300 or 400) */
#define LPVIIOC_SRES _IOW('V',3,int)

/* Get the current page size (depends on resolution and loaded paper) */
#define LPVIIOC_GSIZE _IOR('V',4,struct lpvi_pagesize)

/* Get driver verbosity bits */
#define LPVIIOC_GMSGS _IOR('V',5,unsigned int)

/* Set driver verbosity bits */
#define LPVIIOC_SMSGS _IOW('V',6,unsigned int)

/* Get page setup */
#define LPVIIOC_GSETUP _IOR('V',7,struct lpvi_pagesetup)

/* Set page setup */
#define LPVIIOC_SSETUP _IOW('V',8,struct lpvi_pagesetup)

/* Get flag bits */
#define LPVIIOC_GFLAGS _IOR('V',9,unsigned int)

/* Set flag bits */
#define LPVIIOC_SFLAGS _IOW('V',10,unsigned int)

/* Set display */
#define LPVIIOC_SDISP _IOW('V',11,unsigned int)

#endif
