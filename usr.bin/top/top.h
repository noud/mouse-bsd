/*	$NetBSD: top.h,v 1.3 1999/04/12 06:02:26 ross Exp $	*/

/*
 *  Top - a top users display for Berkeley Unix
 *
 *  General (global) definitions
 */

/* Current major version number */
#define VERSION		3

/* Number of lines of header information on the standard screen */
#define Header_lines	6

/* Maximum number of columns allowed for display */
#define MAX_COLS	128

/* Log base 2 of 1024 is 10 (2^10 == 1024) */
#define LOG1024		10

/* Special atoi routine returns either a non-negative number or one of: */
#define Infinity	-1
#define Invalid		-2

/* maximum number we can have */
#define Largest		0x7fffffff

/*
 * The entire display is based on these next numbers being defined as is.
 */

#define NUM_AVERAGES    3

void reset_display __P((void));
sigret_t leave __P((int));
sigret_t tstop __P((int));
sigret_t winch __P((int));
sigret_t onalrm __P((int));
void quit __P((int));

char *version_string __P((void));
