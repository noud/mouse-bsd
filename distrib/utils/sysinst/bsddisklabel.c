/*	$NetBSD: bsddisklabel.c,v 1.1 1999/08/16 08:29:04 abs Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/* bsddisklabel.c -- generate standard BSD disklabel */
/* Included by appropriate arch/XXXX/md.c */

/* For the current state of this file blame abs@netbsd.org */

int	make_bsd_partitions __P((void));

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
make_bsd_partitions(void)
{
	int i, part;
	int remain;
	char isize[20];
	int maxpart = getmaxpartitions();

	/*
	 * Initialize global variables that track space used on this disk.
	 * Standard 4.3BSD 8-partition labels always cover whole disk.
	 */
	ptsize = dlsize - ptstart;
	fsdsize = dlsize;		/* actually means `whole disk' */
	fsptsize = dlsize - ptstart;	/* netbsd partition -- same as above */
	fsdmb = fsdsize / MEG;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
		    (1.0*fsptsize*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu(MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult();
	} else {
		sizemult = MEG / sectorsize;
		multname = msg_string(MSG_megname);
	}


	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		bsdlabel[part].pi_fstype = FS_UNUSED;

	/* We _always_ have a root partition */
	bsdlabel[PART_ROOT].pi_fstype = FS_BSDFFS;

	/* Whole disk partition */
	bsdlabel[PART_RAW].pi_offset = 0;
	bsdlabel[PART_RAW].pi_size = dlsize;

#ifdef PART_BSDRAW	/* BSD area of the disk - only on some (i386) ports */
	bsdlabel[PART_BSDRAW].pi_offset = ptstart;
	bsdlabel[PART_BSDRAW].pi_size = fsptsize;
#endif

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c "unused", PART_USR /usr */
	case 2: /* standard X: as above, but with larger swap */
		partstart = ptstart;

		/* Root */
		partsize = NUMSEC(DEFROOTSIZE, MEG/sectorsize, dlcylsize);
		bsdlabel[PART_ROOT].pi_offset = partstart;
		bsdlabel[PART_ROOT].pi_size = partsize;
		bsdlabel[PART_ROOT].pi_bsize = 8192;
		bsdlabel[PART_ROOT].pi_fsize = 1024;
		strcpy(fsmount[PART_ROOT], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 *
			   (rammb < DEFSWAPRAM ? DEFSWAPRAM : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		bsdlabel[PART_SWAP].pi_fstype = FS_SWAP;
		bsdlabel[PART_SWAP].pi_offset = partstart;
		bsdlabel[PART_SWAP].pi_size = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsdsize - partstart;
		bsdlabel[PART_USR].pi_fstype = FS_BSDFFS;
		bsdlabel[PART_USR].pi_offset = partstart;
		bsdlabel[PART_USR].pi_size = partsize;
		bsdlabel[PART_USR].pi_bsize = 8192;
		bsdlabel[PART_USR].pi_fsize = 1024;
		strcpy(fsmount[PART_USR], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		/* root */
		partstart = ptstart;
		remain = fsdsize - partstart;
		partsize = NUMSEC(DEFROOTSIZE, MEG/sectorsize, dlcylsize);
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt(MSG_askfsroot, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		/* If less than a 'unit' left, also use it */
		if (remain - partsize < sizemult)
			partsize = remain;
		bsdlabel[PART_ROOT].pi_offset = partstart;
		bsdlabel[PART_ROOT].pi_size = partsize;
		bsdlabel[PART_ROOT].pi_bsize = 8192;
		bsdlabel[PART_ROOT].pi_fsize = 1024;
		strcpy(fsmount[PART_ROOT], "/");
		partstart += partsize;

		/* swap */
		remain = fsdsize - partstart;
		if (remain > 0) {
			i = NUMSEC(2 *
				   (rammb < DEFSWAPRAM ? DEFSWAPRAM : rammb),
				   MEG/sectorsize, dlcylsize) + partstart;
			partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
			if (partsize > remain)
				partsize = remain;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfsswap, isize, isize, 20,
				    remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			if (partsize) {
				/* If less than a 'unit' left, also use it */
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[PART_SWAP].pi_fstype = FS_SWAP;
				bsdlabel[PART_SWAP].pi_offset = partstart;
				bsdlabel[PART_SWAP].pi_size = partsize;
				partstart += partsize;
			}
		}

		/* /usr */
		remain = fsdsize - partstart;
		if (remain > 0) {
			partsize = remain;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfsusr, isize, isize, 20,
				    remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			if (partsize) {
				/* If less than a 'unit' left, also use it */
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[PART_USR].pi_fstype = FS_BSDFFS;
				bsdlabel[PART_USR].pi_offset = partstart;
				bsdlabel[PART_USR].pi_size = partsize;
				bsdlabel[PART_USR].pi_bsize = 8192;
				bsdlabel[PART_USR].pi_fsize = 1024;
				strcpy(fsmount[PART_USR], "/usr");
				partstart += partsize;
			}
		}

		/* Others ... */
		remain = fsdsize - partstart;
		if (remain > 0)
			msg_display(MSG_otherparts);
		part = PART_FIRST_FREE;
		for (; remain > 0 && part < maxpart; ++part ) {
			if (bsdlabel[part].pi_fstype != FS_UNUSED)
				continue;
			partsize = fsdsize - partstart;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfspart, isize, isize, 20,
					diskdev, partition_name(part),
					remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			/* If less than a 'unit' left, also use it */
			if (remain - partsize < sizemult)
				partsize = remain;
			bsdlabel[part].pi_fstype = FS_BSDFFS;
			bsdlabel[part].pi_offset = partstart;
			bsdlabel[part].pi_size = partsize;
			bsdlabel[part].pi_bsize = 8192;
			bsdlabel[part].pi_fsize = 1024;
			msg_prompt_add(MSG_mountpoint, NULL, fsmount[part],
				    20);
			partstart += partsize;
			remain = fsdsize - partstart;
		}

		break;
	}

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 edit_check:
	if (edit_and_check_label(bsdlabel, maxpart, RAW_PART, RAW_PART) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (md_check_partitions() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, maxpart);

	/* Everything looks OK. */
	return (1);
}

