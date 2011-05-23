/*	$NetBSD: rf_kintf.h,v 1.8 2000/02/13 04:53:58 oster Exp $	*/
/*
 * rf_kintf.h
 *
 * RAIDframe exported kernel interface
 */
/*
 * Copyright (c) 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Jim Zelenka
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _RF__RF_KINTF_H_
#define _RF__RF_KINTF_H_

#include "rf_types.h"

int     rf_GetSpareTableFromDaemon(RF_SparetWait_t * req);

void    raidstart(RF_Raid_t * raidPtr);
int     rf_DispatchKernelIO(RF_DiskQueue_t * queue, RF_DiskQueueData_t * req);

int raidwrite_component_label(dev_t, struct vnode *, RF_ComponentLabel_t *);
int raidread_component_label(dev_t, struct vnode *, RF_ComponentLabel_t *);
void rf_update_component_labels( RF_Raid_t *);
int raidlookup __P((char *, struct proc *, struct vnode **));
int raidmarkclean(dev_t dev, struct vnode *b_vp, int);
int raidmarkdirty(dev_t dev, struct vnode *b_vp, int);
void raid_init_component_label __P((RF_Raid_t *, RF_ComponentLabel_t *));

#endif				/* _RF__RF_KINTF_H_ */
