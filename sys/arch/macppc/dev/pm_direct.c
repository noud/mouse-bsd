/*	$NetBSD: pm_direct.c,v 1.8 1999/09/05 05:30:30 tsubai Exp $	*/

/*
 * Copyright (C) 1997 Takashi Hamada
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Takashi Hamada
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* From: pm_direct.c 1.3 03/18/98 Takashi Hamada */

#ifdef DEBUG
#ifndef ADB_DEBUG
#define ADB_DEBUG
#endif
#endif

/* #define	PM_GRAB_SI	1 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/adbsys.h>
#include <machine/cpu.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/pm_direct.h>
#include <macppc/dev/viareg.h>

extern int adb_polling;		/* Are we polling?  (Debugger mode) */

/* hardware dependent values */
#define ADBDelay 100		/* XXX */
#define HwCfgFlags3 0x20000	/* XXX */

/* define the types of the Power Manager */
#define PM_HW_UNKNOWN		0x00	/* don't know */
#define PM_HW_PB1XX		0x01	/* PowerBook 1XX series */
#define	PM_HW_PB5XX		0x02	/* PowerBook Duo and 5XX series */

/* useful macros */
#define PM_SR()			read_via_reg(VIA1, vSR)
#define PM_VIA_INTR_ENABLE()	write_via_reg(VIA1, vIER, 0x90)
#define PM_VIA_INTR_DISABLE()	write_via_reg(VIA1, vIER, 0x10)
#define PM_VIA_CLR_INTR()	write_via_reg(VIA1, vIFR, 0x90)
#if 0
#define PM_SET_STATE_ACKON()	via_reg_or(VIA2, vBufB, 0x04)
#define PM_SET_STATE_ACKOFF()	via_reg_and(VIA2, vBufB, ~0x04)
#define PM_IS_ON		(0x02 == (read_via_reg(VIA2, vBufB) & 0x02))
#define PM_IS_OFF		(0x00 == (read_via_reg(VIA2, vBufB) & 0x02))
#else
#define PM_SET_STATE_ACKON()	via_reg_or(VIA2, vBufB, 0x10)
#define PM_SET_STATE_ACKOFF()	via_reg_and(VIA2, vBufB, ~0x10)
#define PM_IS_ON		(0x08 == (read_via_reg(VIA2, vBufB) & 0x08))
#define PM_IS_OFF		(0x00 == (read_via_reg(VIA2, vBufB) & 0x08))
#endif

/*
 * Variables for internal use
 */
int	pmHardware = PM_HW_UNKNOWN;
u_short	pm_existent_ADB_devices = 0x0;	/* each bit expresses the existent ADB device */
u_int	pm_LCD_brightness = 0x0;
u_int	pm_LCD_contrast = 0x0;
u_int	pm_counter = 0;			/* clock count */

/* these values shows that number of data returned after 'send' cmd is sent */
signed char pm_send_cmd_type[] = {
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1, 0x00,
	  -1, 0x00, 0x02, 0x01, 0x01,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x04, 0x14,   -1, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x02, 0x02,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1, 0x01,   -1,   -1,   -1,
	0x01, 0x00, 0x02, 0x02,   -1, 0x01, 0x03, 0x01,
	0x00, 0x01, 0x00, 0x00, 0x00,   -1,   -1,   -1,
	0x02,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   -1,   -1,
	0x01, 0x01, 0x01,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1, 0x04, 0x04,
	0x04,   -1, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00,   -1,   -1,   -1,   -1,   -1,   -1,
	0x02, 0x02, 0x02, 0x04,   -1, 0x00,   -1,   -1,
	0x01, 0x01, 0x03, 0x02,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x01, 0x01,   -1,   -1, 0x00, 0x00,   -1,   -1,
	  -1, 0x04, 0x00,   -1,   -1,   -1,   -1,   -1,
	0x03,   -1, 0x00,   -1, 0x00,   -1,   -1, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

/* these values shows that number of data returned after 'receive' cmd is sent */
signed char pm_receive_cmd_type[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x05, 0x15,   -1, 0x02,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x03, 0x03,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x04, 0x04, 0x03, 0x09,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x06,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x02,   -1,   -1, 0x02,   -1,   -1,   -1,
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1, 0x02,   -1,   -1,   -1,   -1, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
};


/*
 * Define the private functions
 */

/* for debugging */
#ifdef ADB_DEBUG
void	pm_printerr __P((char *, int, int, char *));
#endif

int	pm_wait_busy __P((int));
int	pm_wait_free __P((int));

/* these functions are for the PB1XX series */
int	pm_receive_pm1 __P((u_char *));
int	pm_send_pm1 __P((u_char,int));
int	pm_pmgrop_pm1 __P((PMData *));
void	pm_intr_pm1 __P((void));

/* these functions are for the PB Duo series and the PB 5XX series */
int	pm_receive_pm2 __P((u_char *));
int	pm_send_pm2 __P((u_char));
int	pm_pmgrop_pm2 __P((PMData *));
void	pm_intr_pm2 __P((void));

/* this function is MRG-Based (for testing) */
int	pm_pmgrop_mrg __P((PMData *));

/* these functions are called from adb_direct.c */
void	pm_setup_adb __P((void));
void	pm_check_adb_devices __P((int));
void	pm_intr __P((void));
int	pm_adb_op __P((u_char *, void *, void *, int));

/* these functions also use the variables of adb_direct.c */
void	pm_adb_get_TALK_result __P((PMData *));
void	pm_adb_get_ADB_data __P((PMData *));
void	pm_adb_poll_next_device_pm1 __P((PMData *));


/*
 * These variables are in adb_direct.c.
 */
extern u_char	*adbBuffer;	/* pointer to user data area */
extern void	*adbCompRout;	/* pointer to the completion routine */
extern void	*adbCompData;	/* pointer to the completion routine data */
extern int	adbWaiting;	/* waiting for return data from the device */
extern int	adbWaitingCmd;	/* ADB command we are waiting for */
extern int	adbStarting;	/* doing ADB reinit, so do "polling" differently */

#define	ADB_MAX_MSG_LENGTH	16
#define	ADB_MAX_HDR_LENGTH	8
struct adbCommand {
	u_char	header[ADB_MAX_HDR_LENGTH];	/* not used yet */
	u_char	data[ADB_MAX_MSG_LENGTH];	/* packet data only */
	u_char	*saveBuf;	/* where to save result */
	u_char	*compRout;	/* completion routine pointer */
	u_char	*compData;	/* completion routine data pointer */
	u_int	cmd;		/* the original command for this data */
	u_int	unsol;		/* 1 if packet was unsolicited */
	u_int	ack_only;	/* 1 for no special processing */
};
extern	void	adb_pass_up __P((struct adbCommand *));


#if 0
/*
 * Define the external functions
 */
extern int	zshard __P((int));		/* from zs.c */
#endif

#ifdef ADB_DEBUG
/*
 * This function dumps contents of the PMData
 */
void
pm_printerr(ttl, rval, num, data)
	char *ttl;
	int rval;
	int num;
	char *data;
{
	int i;

	printf("pm: %s:%04x %02x ", ttl, rval, num);
	for (i = 0; i < num; i++)
		printf("%02x ", data[i]);
	printf("\n");
}
#endif



/*
 * Check the hardware type of the Power Manager
 */
void
pm_setup_adb()
{
	pmHardware = PM_HW_PB5XX;	/* XXX */
}


/*
 * Check the existent ADB devices
 */
void
pm_check_adb_devices(id)
	int id;
{
	u_short ed = 0x1;

	ed <<= id;
	pm_existent_ADB_devices |= ed;
}


/*
 * Wait until PM IC is busy
 */
int
pm_wait_busy(delay)
	int delay;
{
	while (PM_IS_ON) {
#ifdef PM_GRAB_SI
#if 0
		zshard(0);		/* grab any serial interrupts */
#else
		(void)intr_dispatch(0x70);
#endif
#endif
		if ((--delay) < 0)
			return 1;	/* timeout */
	}
	return 0;
}


/*
 * Wait until PM IC is free
 */
int
pm_wait_free(delay)
	int delay;
{
	while (PM_IS_OFF) {
#ifdef PM_GRAB_SI
#if 0
		zshard(0);		/* grab any serial interrupts */
#else
		(void)intr_dispatch(0x70);
#endif
#endif
		if ((--delay) < 0)
			return 0;	/* timeout */
	}
	return 1;
}



/*
 * Functions for the PB1XX series
 */

/*
 * Receive data from PM for the PB1XX series
 */
int
pm_receive_pm1(data)
	u_char *data;
{
#if 0
	int rval = 0xffffcd34;

	via_reg(VIA2, vDirA) = 0x00;

	switch (1) {
		default:
			if (pm_wait_busy(0x40) != 0)
				break;			/* timeout */

			PM_SET_STATE_ACKOFF();
			*data = via_reg(VIA2, 0x200);

			rval = 0xffffcd33;
			if (pm_wait_free(0x40) == 0)
				break;			/* timeout */

			rval = 0x00;
			break;
	}

	PM_SET_STATE_ACKON();
	via_reg(VIA2, vDirA) = 0x00;

	return rval;
#else
	panic("pm_receive_pm1");
#endif
}



/*
 * Send data to PM for the PB1XX series
 */
int
pm_send_pm1(data, delay)
	u_char data;
	int delay;
{
#if 0
	int rval;

	via_reg(VIA2, vDirA) = 0xff;
	via_reg(VIA2, 0x200) = data;

	PM_SET_STATE_ACKOFF();
	if (pm_wait_busy(0x400) != 0) {
		PM_SET_STATE_ACKON();
		via_reg(VIA2, vDirA) = 0x00;

		return 0xffffcd36;
	}

	rval = 0x0;
	PM_SET_STATE_ACKON();
	if (pm_wait_free(0x40) == 0)
		rval = 0xffffcd35;

	PM_SET_STATE_ACKON();
	via_reg(VIA2, vDirA) = 0x00;

	return rval;
#else
	panic("pm_send_pm1");
#endif
}


/*
 * My PMgrOp routine for the PB1XX series
 */
int
pm_pmgrop_pm1(pmdata)
	PMData *pmdata;
{
#if 0
	int i;
	int s = 0x81815963;
	u_char via1_vIER, via1_vDirA;
	int rval = 0;
	int num_pm_data = 0;
	u_char pm_cmd;
	u_char pm_data;
	u_char *pm_buf;

	/* disable all inetrrupts but PM */
	via1_vIER = via_reg(VIA1, vIER);
	PM_VIA_INTR_DISABLE();

	via1_vDirA = via_reg(VIA1, vDirA);

	switch (pmdata->command) {
		default:
			for (i = 0; i < 7; i++) {
				via_reg(VIA2, vDirA) = 0x00;

				/* wait until PM is free */
				if (pm_wait_free(ADBDelay) == 0) {	/* timeout */
					via_reg(VIA2, vDirA) = 0x00;
					/* restore formar value */
					via_reg(VIA1, vDirA) = via1_vDirA;
					via_reg(VIA1, vIER) = via1_vIER;
					return 0xffffcd38;
				}

				switch (mac68k_machine.machineid) {
					case MACH_MACPB160:
					case MACH_MACPB165:
					case MACH_MACPB165C:
					case MACH_MACPB180:
					case MACH_MACPB180C:
						{
							int delay = ADBDelay * 16;

							via_reg(VIA2, vDirA) = 0x00;
							while ((via_reg(VIA2, 0x200) == 0x7f) && (delay >= 0))
								delay--;

							if (delay < 0) {	/* timeout */
								via_reg(VIA2, vDirA) = 0x00;
								/* restore formar value */
								via_reg(VIA1, vIER) = via1_vIER;
								return 0xffffcd38;
							}
						}
				} /* end switch */

				s = splhigh();

				via1_vDirA = via_reg(VIA1, vDirA);
				via_reg(VIA1, vDirA) &= 0x7f;

				pm_cmd = (u_char)(pmdata->command & 0xff);
				if ((rval = pm_send_pm1(pm_cmd, ADBDelay * 8)) == 0)
					break;	/* send command succeeded */

				via_reg(VIA1, vDirA) = via1_vDirA;
				splx(s);
			} /* end for */

			/* failed to send a command */
			if (i == 7) {
				via_reg(VIA2, vDirA) = 0x00;
				/* restore formar value */
				via_reg(VIA1, vDirA) = via1_vDirA;
				via_reg(VIA1, vIER) = via1_vIER;
					return 0xffffcd38;
			}

			/* send # of PM data */
			num_pm_data = pmdata->num_data;
			if ((rval = pm_send_pm1((u_char)(num_pm_data & 0xff), ADBDelay * 8)) != 0)
				break;			/* timeout */

			/* send PM data */
			pm_buf = (u_char *)pmdata->s_buf;
			for (i = 0; i < num_pm_data; i++)
				if ((rval = pm_send_pm1(pm_buf[i], ADBDelay * 8)) != 0)
					break;		/* timeout */
			if ((i != num_pm_data) && (num_pm_data != 0))
				break;			/* timeout */

			/* Will PM IC return data? */
			if ((pm_cmd & 0x08) == 0) {
				rval = 0;
				break;			/* no returned data */
			}

			rval = 0xffffcd37;
			if (pm_wait_busy(ADBDelay) != 0)
				break;			/* timeout */

			/* receive PM command */
			if ((rval = pm_receive_pm1(&pm_data)) != 0)
				break;

			pmdata->command = pm_data;

			/* receive number of PM data */
			if ((rval = pm_receive_pm1(&pm_data)) != 0)
				break;			/* timeout */
			num_pm_data = pm_data;
			pmdata->num_data = num_pm_data;

			/* receive PM data */
			pm_buf = (u_char *)pmdata->r_buf;
			for (i = 0; i < num_pm_data; i++) {
				if ((rval = pm_receive_pm1(&pm_data)) != 0)
					break;				/* timeout */
				pm_buf[i] = pm_data;
			}

			rval = 0;
	}

	via_reg(VIA2, vDirA) = 0x00;

	/* restore formar value */
	via_reg(VIA1, vDirA) = via1_vDirA;
	via_reg(VIA1, vIER) = via1_vIER;
	if (s != 0x81815963)
		splx(s);

	return rval;
#else
	panic("pm_pmgrop_pm1");
#endif
}


/*
 * My PM interrupt routine for PB1XX series
 */
void
pm_intr_pm1()
{
#if 0
	int s;
	int rval;
	PMData pmdata;

	s = splhigh();

	PM_VIA_CLR_INTR();				/* clear VIA1 interrupt */

	/* ask PM what happend */
	pmdata.command = 0x78;
	pmdata.num_data = 0;
	pmdata.data[0] = pmdata.data[1] = 0;
	pmdata.s_buf = &pmdata.data[2];
	pmdata.r_buf = &pmdata.data[2];
	rval = pm_pmgrop_pm1(&pmdata);
	if (rval != 0) {
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("pm: PM is not ready. error code=%08x\n", rval);
#endif
		splx(s);
	}

	if ((pmdata.data[2] & 0x10) == 0x10) {
		if ((pmdata.data[2] & 0x0f) == 0) {
			/* ADB data that were requested by TALK command */
			pm_adb_get_TALK_result(&pmdata);
		} else if ((pmdata.data[2] & 0x08) == 0x8) {
			/* PM is requesting to poll  */
			pm_adb_poll_next_device_pm1(&pmdata);
		} else if ((pmdata.data[2] & 0x04) == 0x4) {
			/* ADB device event */
			pm_adb_get_ADB_data(&pmdata);
		}
	} else {
#ifdef ADB_DEBUG
		if (adb_debug)
			pm_printerr("driver does not supported this event.",
			    rval, pmdata.num_data, pmdata.data);
#endif
	}

	splx(s);
#else
	panic("pm_intr_pm1");
#endif
}



/*
 * Functions for the PB Duo series and the PB 5XX series
 */

/*
 * Receive data from PM for the PB Duo series and the PB 5XX series
 */
int
pm_receive_pm2(data)
	u_char *data;
{
	int i;
	int rval;

	rval = 0xffffcd34;

	switch (1) {
		default:
			/* set VIA SR to input mode */
			via_reg_or(VIA1, vACR, 0x0c);
			via_reg_and(VIA1, vACR, ~0x10);
			i = PM_SR();

			PM_SET_STATE_ACKOFF();
			if (pm_wait_busy((int)ADBDelay*32) != 0)
				break;		/* timeout */

			PM_SET_STATE_ACKON();
			rval = 0xffffcd33;
			if (pm_wait_free((int)ADBDelay*32) == 0)
				break;		/* timeout */

			*data = PM_SR();
			rval = 0;

			break;
	}

	PM_SET_STATE_ACKON();
	via_reg_or(VIA1, vACR, 0x1c);

	return rval;
}



/*
 * Send data to PM for the PB Duo series and the PB 5XX series
 */
int
pm_send_pm2(data)
	u_char data;
{
	int rval;

	via_reg_or(VIA1, vACR, 0x1c);
	write_via_reg(VIA1, vSR, data);	/* PM_SR() = data; */

	PM_SET_STATE_ACKOFF();
	rval = 0xffffcd36;
	if (pm_wait_busy((int)ADBDelay*32) != 0) {
		PM_SET_STATE_ACKON();

		via_reg_or(VIA1, vACR, 0x1c);

		return rval;
	}

	PM_SET_STATE_ACKON();
	rval = 0xffffcd35;
	if (pm_wait_free((int)ADBDelay*32) != 0)
		rval = 0;

	PM_SET_STATE_ACKON();
	via_reg_or(VIA1, vACR, 0x1c);

	return rval;
}



/*
 * My PMgrOp routine for the PB Duo series and the PB 5XX series
 */
int
pm_pmgrop_pm2(pmdata)
	PMData *pmdata;
{
	int i;
	int s;
	u_char via1_vIER;
	int rval = 0;
	int num_pm_data = 0;
	u_char pm_cmd;
	short pm_num_rx_data;
	u_char pm_data;
	u_char *pm_buf;

	s = splhigh();

	/* disable all inetrrupts but PM */
	via1_vIER = 0x10;
	via1_vIER &= read_via_reg(VIA1, vIER);
	write_via_reg(VIA1, vIER, via1_vIER);
	if (via1_vIER != 0x0)
		via1_vIER |= 0x80;

	switch (pmdata->command) {
		default:
			/* wait until PM is free */
			pm_cmd = (u_char)(pmdata->command & 0xff);
			rval = 0xcd38;
			if (pm_wait_free(ADBDelay * 4) == 0)
				break;			/* timeout */

			if (HwCfgFlags3 & 0x00200000) {
				/* PB 160, PB 165(c), PB 180(c)? */
				int delay = ADBDelay * 16;

				write_via_reg(VIA2, vDirA, 0x00);
				while ((read_via_reg(VIA2, 0x200) == 0x07) &&
				    (delay >= 0))
					delay--;

				if (delay < 0) {
					rval = 0xffffcd38;
					break;		/* timeout */
				}
			}

			/* send PM command */
			if ((rval = pm_send_pm2((u_char)(pm_cmd & 0xff))))
				break;				/* timeout */

			/* send number of PM data */
			num_pm_data = pmdata->num_data;
			if (HwCfgFlags3 & 0x00020000) {		/* PB Duo, PB 5XX */
				if (pm_send_cmd_type[pm_cmd] < 0) {
					if ((rval = pm_send_pm2((u_char)(num_pm_data & 0xff))) != 0)
						break;		/* timeout */
					pmdata->command = 0;
				}
			} else {				/* PB 1XX series ? */
				if ((rval = pm_send_pm2((u_char)(num_pm_data & 0xff))) != 0)
					break;			/* timeout */
			}
			/* send PM data */
			pm_buf = (u_char *)pmdata->s_buf;
			for (i = 0 ; i < num_pm_data; i++)
				if ((rval = pm_send_pm2(pm_buf[i])) != 0)
					break;			/* timeout */
			if (i != num_pm_data)
				break;				/* timeout */


			/* check if PM will send me data  */
			pm_num_rx_data = pm_receive_cmd_type[pm_cmd];
			pmdata->num_data = pm_num_rx_data;
			if (pm_num_rx_data == 0) {
				rval = 0;
				break;				/* no return data */
			}

			/* receive PM command */
			pm_data = pmdata->command;
			if (HwCfgFlags3 & 0x00020000) {		/* PB Duo, PB 5XX */
				pm_num_rx_data--;
				if (pm_num_rx_data == 0)
					if ((rval = pm_receive_pm2(&pm_data)) != 0) {
						rval = 0xffffcd37;
						break;
					}
				pmdata->command = pm_data;
			} else {				/* PB 1XX series ? */
				if ((rval = pm_receive_pm2(&pm_data)) != 0) {
					rval = 0xffffcd37;
					break;
				}
				pmdata->command = pm_data;
			}

			/* receive number of PM data */
			if (HwCfgFlags3 & 0x00020000) {		/* PB Duo, PB 5XX */
				if (pm_num_rx_data < 0) {
					if ((rval = pm_receive_pm2(&pm_data)) != 0)
						break;		/* timeout */
					num_pm_data = pm_data;
				} else
					num_pm_data = pm_num_rx_data;
				pmdata->num_data = num_pm_data;
			} else {				/* PB 1XX serias ? */
				if ((rval = pm_receive_pm2(&pm_data)) != 0)
					break;			/* timeout */
				num_pm_data = pm_data;
				pmdata->num_data = num_pm_data;
			}

			/* receive PM data */
			pm_buf = (u_char *)pmdata->r_buf;
			for (i = 0; i < num_pm_data; i++) {
				if ((rval = pm_receive_pm2(&pm_data)) != 0)
					break;			/* timeout */
				pm_buf[i] = pm_data;
			}

			rval = 0;
	}

	/* restore former value */
	write_via_reg(VIA1, vIER, via1_vIER);
	splx(s);

	return rval;
}


/*
 * My PM interrupt routine for the PB Duo series and the PB 5XX series
 */
void
pm_intr_pm2()
{
	int s;
	int rval;
	PMData pmdata;

	s = splhigh();

	PM_VIA_CLR_INTR();			/* clear VIA1 interrupt */
						/* ask PM what happend */
	pmdata.command = 0x78;
	pmdata.num_data = 0;
	pmdata.s_buf = &pmdata.data[2];
	pmdata.r_buf = &pmdata.data[2];
	rval = pm_pmgrop_pm2(&pmdata);
	if (rval != 0) {
#ifdef ADB_DEBUG
		if (adb_debug)
			printf("pm: PM is not ready. error code: %08x\n", rval);
#endif
		splx(s);
	}

	switch ((u_int)(pmdata.data[2] & 0xff)) {
		case 0x00:			/* 1 sec interrupt? */
			break;
		case 0x80:			/* 1 sec interrupt? */
			pm_counter++;
			break;
		case 0x08:			/* Brightness/Contrast button on LCD panel */
			/* get brightness and contrast of the LCD */
			pm_LCD_brightness = (u_int)pmdata.data[3] & 0xff;
			pm_LCD_contrast = (u_int)pmdata.data[4] & 0xff;
/*
			pm_printerr("#08", rval, pmdata.num_data, pmdata.data);
			pmdata.command = 0x33;
			pmdata.num_data = 1;
			pmdata.s_buf = pmdata.data;
			pmdata.r_buf = pmdata.data;
			pmdata.data[0] = pm_LCD_contrast;
			rval = pm_pmgrop_pm2(&pmdata);
			pm_printerr("#33", rval, pmdata.num_data, pmdata.data);
*/
			/* this is an experimental code */
			pmdata.command = 0x41;
			pmdata.num_data = 1;
			pmdata.s_buf = pmdata.data;
			pmdata.r_buf = pmdata.data;
			pm_LCD_brightness = 0x7f - pm_LCD_brightness / 2;
			if (pm_LCD_brightness < 0x08)
				pm_LCD_brightness = 0x08;
			if (pm_LCD_brightness > 0x78)
				pm_LCD_brightness = 0x78;
			pmdata.data[0] = pm_LCD_brightness;
			rval = pm_pmgrop_pm2(&pmdata);
			break;
		case 0x10:			/* ADB data that were requested by TALK command */
		case 0x14:
			pm_adb_get_TALK_result(&pmdata);
			break;
		case 0x16:			/* ADB device event */
		case 0x18:
		case 0x1e:
			pm_adb_get_ADB_data(&pmdata);
			break;
		default:
#ifdef ADB_DEBUG
			if (adb_debug)
				pm_printerr("driver does not supported this event.",
				    pmdata.data[2], pmdata.num_data,
				    pmdata.data);
#endif
			break;
	}

	splx(s);
}


#if 0
/*
 * MRG-based PMgrOp routine
 */
int
pm_pmgrop_mrg(pmdata)
	PMData *pmdata;
{
	u_int32_t rval=0;

	asm("
		movl	%1, a0
		.word	0xa085
		movl	d0, %0"
		: "=g" (rval)
		: "g" (pmdata)
		: "a0", "d0" );

	return rval;
}
#endif


/*
 * My PMgrOp routine
 */
int
pmgrop(pmdata)
	PMData *pmdata;
{
	switch (pmHardware) {
		case PM_HW_PB1XX:
			return (pm_pmgrop_pm1(pmdata));
			break;
		case PM_HW_PB5XX:
			return (pm_pmgrop_pm2(pmdata));
			break;
		default:
			/* return (pmgrop_mrg(pmdata)); */
			return 1;
	}
}


/*
 * My PM interrupt routine
 */
void
pm_intr()
{
	switch (pmHardware) {
		case PM_HW_PB1XX:
			pm_intr_pm1();
			break;
		case PM_HW_PB5XX:
			pm_intr_pm2();
			break;
		default:
			break;
	}
}



/*
 * Synchronous ADBOp routine for the Power Manager
 */
int
pm_adb_op(buffer, compRout, data, command)
	u_char *buffer;
	void *compRout;
	void *data;
	int command;
{
	int i;
	int s;
	int rval;
	int delay;
	PMData pmdata;
	struct adbCommand packet;

	if (adbWaiting == 1)
		return 1;

	s = splhigh();
	write_via_reg(VIA1, vIER, 0x10);

 	adbBuffer = buffer;
	adbCompRout = compRout;
	adbCompData = data;

	pmdata.command = 0x20;
	pmdata.s_buf = pmdata.data;
	pmdata.r_buf = pmdata.data;

	/* if the command is LISTEN, add number of ADB data to number of PM data */
	if ((command & 0xc) == 0x8) {
		if (buffer != (u_char *)0)
			pmdata.num_data = buffer[0] + 3;
	} else {
		pmdata.num_data = 3;
	}

	pmdata.data[0] = (u_char)(command & 0xff);
	pmdata.data[1] = 0;
	if ((command & 0xc) == 0x8) {		/* if the command is LISTEN, copy ADB data to PM buffer */
		if ((buffer != (u_char *)0) && (buffer[0] <= 24)) {
			pmdata.data[2] = buffer[0];		/* number of data */
			for (i = 0; i < buffer[0]; i++)
				pmdata.data[3 + i] = buffer[1 + i];
		} else
			pmdata.data[2] = 0;
	} else
		pmdata.data[2] = 0;

	if ((command & 0xc) != 0xc) {		/* if the command is not TALK */
		/* set up stuff for adb_pass_up */
		packet.data[0] = 1 + pmdata.data[2];
		packet.data[1] = command;
		for (i = 0; i < pmdata.data[2]; i++)
			packet.data[i+2] = pmdata.data[i+3];
		packet.saveBuf = adbBuffer;
		packet.compRout = adbCompRout;
		packet.compData = adbCompData;
		packet.cmd = command;
		packet.unsol = 0;
		packet.ack_only = 1;
		adb_polling = 1;
		adb_pass_up(&packet);
		adb_polling = 0;
	}

	rval = pmgrop(&pmdata);
	if (rval != 0)
		return 1;

	adbWaiting = 1;
	adbWaitingCmd = command;

	PM_VIA_INTR_ENABLE();

	/* wait until the PM interrupt is occured */
	delay = 0x80000;
	while (adbWaiting == 1) {
		if (read_via_reg(VIA1, vIFR) & 0x14)
			pm_intr();
#ifdef PM_GRAB_SI
#if 0
			zshard(0);		/* grab any serial interrupts */
#else
			(void)intr_dispatch(0x70);
#endif
#endif
		if ((--delay) < 0)
			return 1;
	}

	/* this command enables the interrupt by operating ADB devices */
	if (HwCfgFlags3 & 0x00020000) {		/* PB Duo series, PB 5XX series */
		pmdata.command = 0x20;
		pmdata.num_data = 4;
		pmdata.s_buf = pmdata.data;
		pmdata.r_buf = pmdata.data;
		pmdata.data[0] = 0x00;
		pmdata.data[1] = 0x86;	/* magic spell for awaking the PM */
		pmdata.data[2] = 0x00;
		pmdata.data[3] = 0x0c;	/* each bit may express the existent ADB device */
	} else {				/* PB 1XX series */
		pmdata.command = 0x20;
		pmdata.num_data = 3;
		pmdata.s_buf = pmdata.data;
		pmdata.r_buf = pmdata.data;
		pmdata.data[0] = (u_char)(command & 0xf0) | 0xc;
		pmdata.data[1] = 0x04;
		pmdata.data[2] = 0x00;
	}
	rval = pmgrop(&pmdata);

	splx(s);
	return rval;
}


void
pm_adb_get_TALK_result(pmdata)
	PMData *pmdata;
{
	int i;
	struct adbCommand packet;

	/* set up data for adb_pass_up */
	packet.data[0] = pmdata->num_data-1;
	packet.data[1] = pmdata->data[3];
	for (i = 0; i <packet.data[0]-1; i++)
		packet.data[i+2] = pmdata->data[i+4];

	packet.saveBuf = adbBuffer;
	packet.compRout = adbCompRout;
	packet.compData = adbCompData;
	packet.unsol = 0;
	packet.ack_only = 0;
	adb_polling = 1;
	adb_pass_up(&packet);
	adb_polling = 0;

	adbWaiting = 0;
	adbBuffer = (long)0;
	adbCompRout = (long)0;
	adbCompData = (long)0;
}


void
pm_adb_get_ADB_data(pmdata)
	PMData *pmdata;
{
	int i;
	struct adbCommand packet;

	/* set up data for adb_pass_up */
	packet.data[0] = pmdata->num_data-1;	/* number of raw data */
	packet.data[1] = pmdata->data[3];	/* ADB command */
	for (i = 0; i <packet.data[0]-1; i++)
		packet.data[i+2] = pmdata->data[i+4];
	packet.unsol = 1;
	packet.ack_only = 0;
	adb_pass_up(&packet);
}


void
pm_adb_poll_next_device_pm1(pmdata)
	PMData *pmdata;
{
	int i;
	int ndid;
	u_short bendid = 0x1;
	int rval;
	PMData tmp_pmdata;

	/* find another existent ADB device to poll */
	for (i = 1; i < 16; i++) {
		ndid = (((pmdata->data[3] & 0xf0) >> 4) + i) & 0xf;
		bendid <<= ndid;
		if ((pm_existent_ADB_devices & bendid) != 0)
			break;
	}

	/* poll the other device */
	tmp_pmdata.command = 0x20;
	tmp_pmdata.num_data = 3;
	tmp_pmdata.s_buf = tmp_pmdata.data;
	tmp_pmdata.r_buf = tmp_pmdata.data;
	tmp_pmdata.data[0] = (u_char)(ndid << 4) | 0xc;
	tmp_pmdata.data[1] = 0x04;	/* magic spell for awaking the PM */
	tmp_pmdata.data[2] = 0x00;
	rval = pmgrop(&tmp_pmdata);
}

void
pm_adb_restart()
{
	PMData p;

	p.command = PMU_RESET_CPU;
	p.num_data = 0;
	p.s_buf = p.data;
	p.r_buf = p.data;
	pmgrop(&p);
}

void
pm_adb_poweroff()
{
	PMData p;

	p.command = PMU_POWER_OFF;
	p.num_data = 4;
	p.s_buf = p.data;
	p.r_buf = p.data;
	strcpy(p.data, "MATT");
	pmgrop(&p);
}

void
pm_read_date_time(time)
	u_long *time;
{
	PMData p;

	p.command = PMU_READ_RTC;
	p.num_data = 0;
	p.s_buf = p.data;
	p.r_buf = p.data;
	pmgrop(&p);

	bcopy(p.data, time, 4);
}

void
pm_set_date_time(time)
	u_long time;
{
	PMData p;

	p.command = PMU_SET_RTC;
	p.num_data = 4;
	p.s_buf = p.r_buf = p.data;
	bcopy(&time, p.data, 4);
	pmgrop(&p);
}

int
pm_read_brightness()
{
	PMData p;

	p.command = PMU_READ_BRIGHTNESS;
	p.num_data = 1;		/* XXX why 1? */
	p.s_buf = p.r_buf = p.data;
	p.data[0] = 0;
	pmgrop(&p);

	return p.data[0];
}

void
pm_set_brightness(val)
	int val;
{
	PMData p;

	val = 0x7f - val / 2;
	if (val < 0x08)
		val = 0x08;
	if (val > 0x78)
		val = 0x78;

	p.command = PMU_SET_BRIGHTNESS;
	p.num_data = 1;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = val;
	pmgrop(&p);
}

void
pm_init_brightness()
{
	int val;

	val = pm_read_brightness();
	pm_set_brightness(val);
}

void
pm_eject_pcmcia(slot)
	int slot;
{
	PMData p;

	if (slot != 0 && slot != 1)
		return;

	p.command = PMU_EJECT_PCMCIA;
	p.num_data = 1;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = 5 + slot;	/* XXX */
	pmgrop(&p);
}

int
pm_read_nvram(addr)
	int addr;
{
	PMData p;

	p.command = PMU_READ_NVRAM;
	p.num_data = 2;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = addr >> 8;
	p.data[1] = addr;
	pmgrop(&p);

	return p.data[0];
}

void
pm_write_nvram(addr, val)
	int addr, val;
{
	PMData p;

	p.command = PMU_WRITE_NVRAM;
	p.num_data = 3;
	p.s_buf = p.r_buf = p.data;
	p.data[0] = addr >> 8;
	p.data[1] = addr;
	p.data[2] = val;
	pmgrop(&p);
}
