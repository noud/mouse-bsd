/*	$NetBSD: curses.h,v 1.48 2000/05/11 22:50:23 jdc Exp $	*/

/*
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)curses.h	8.5 (Berkeley) 4/29/95
 */

#ifndef _CURSES_H_
#define	_CURSES_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <wchar.h>

#include <stdio.h>
#include <termcap.h>

/*
 * attr_t must be the same size as wchar_t (see <wchar.h>) to avoid padding
 * in __LDATA.
 */
typedef wchar_t	chtype;
typedef wchar_t	attr_t;

/* C++ already defines bool. */
#ifndef __cplusplus
typedef	char	bool;
#endif

#ifndef TRUE
#define	TRUE	(/*CONSTCOND*/1)
#endif
#ifndef FALSE
#define	FALSE	(/*CONSTCOND*/0)
#endif

/*
 * The following #defines and #includes are present for backward
 * compatibility only.  They should not be used in future code.
 *
 * START BACKWARD COMPATIBILITY ONLY.
 */
#ifndef _CURSES_PRIVATE

#define	_puts(s)	tputs(s, 0, __cputchar)
#define	_putchar(c)	__cputchar(c)

/* Old-style terminal modes access. */
#define	baudrate()	(cfgetospeed(&__baset))
#define	crmode()	cbreak()
#define	nocrmode()	nocbreak()
#define	ospeed		(cfgetospeed(&__baset))
#endif /* _CURSES_PRIVATE */

extern char	 GT;			/* Gtty indicates tabs. */
extern char	 NONL;			/* Term can't hack LF doing a CR. */
extern char	 UPPERCASE;		/* Terminal is uppercase only. */

extern int	 My_term;		/* Use Def_term regardless. */
extern const char	*Def_term;		/* Default terminal type. */

/* Termcap capabilities. */
extern char	AM, UT, BS, CA, CC, DA, EO, HC, HL, IN, MI, MS, NC, NS,
		OS, PC, UL, XB, XN, XT, XS, XX;
extern int	PA, cO, nc;
extern char	*AC, *AE, *AL, *AS, *BC, *BL, *BT, *CD, *CE, *CL, *CM,
		*CR, *CS, *DC, *DL, *DM, *DO, *Ea, *ED, *EI, *K0, *K1,
		*K2, *K3, *K4, *K5, *K6, *K7, *K8, *K9, *HO, *IC, *IM,
		*IP, *KD, *KE, *KH, *KL, *KR, *KS, *KU, *LL, *MA, *MB,
		*MD, *ME, *MH, *MK, *MM, *MO, *MP, *MR, *ND, *NL, *OC,
		*OP, *RC, *SC, *SE, *SF, *SO, *SP, *SR, *TA, *TE, *TI,
		*UC, *UE, *UP, *US, *VB, *VI, *VS, *VE, *ab, *af, *al, *dl,
		*iC, *iP, *sB, *sF, *sf, *sr, *AL_PARM, *DL_PARM, *UP_PARM,
		*DOWN_PARM, *LEFT_PARM, *RIGHT_PARM;

/* END BACKWARD COMPATIBILITY ONLY. */

/* symbols for values returned by getch in keypad mode */
#define    KEY_MIN        0x101    /* minimum extended key value */
#define    KEY_BREAK      0x101    /* break key */
#define    KEY_DOWN       0x102    /* down arrow */
#define    KEY_UP         0x103    /* up arrow */
#define    KEY_LEFT       0x104    /* left arrow */
#define    KEY_RIGHT      0x105    /* right arrow*/
#define    KEY_HOME       0x106    /* home key */
#define    KEY_BACKSPACE  0x107    /* Backspace */

/* First function key (block of 64 follow) */
#define    KEY_F0         0x108
/* Function defining other function key values*/
#define    KEY_F(n)       (KEY_F0+(n))

#define    KEY_DL         0x148    /* Delete Line */
#define    KEY_IL         0x149    /* Insert Line*/
#define    KEY_DC         0x14A    /* Delete Character */
#define    KEY_IC         0x14B    /* Insert Character */
#define    KEY_EIC        0x14C    /* Exit Insert Char mode */
#define    KEY_CLEAR      0x14D    /* Clear screen */
#define    KEY_EOS        0x14E    /* Clear to end of screen */
#define    KEY_EOL        0x14F    /* Clear to end of line */
#define    KEY_SF         0x150    /* Scroll one line forward */
#define    KEY_SR         0x151    /* Scroll one line back */
#define    KEY_NPAGE      0x152    /* Next Page */
#define    KEY_PPAGE      0x153    /* Prev Page */
#define    KEY_STAB       0x154    /* Set Tab */
#define    KEY_CTAB       0x155    /* Clear Tab */
#define    KEY_CATAB      0x156    /* Clear All Tabs */
#define    KEY_ENTER      0x157    /* Enter or Send */
#define    KEY_SRESET     0x158    /* Soft Reset */
#define    KEY_RESET      0x159    /* Hard Reset */
#define    KEY_PRINT      0x15A    /* Print */
#define    KEY_LL         0x15B    /* Home Down */

/*
 * "Keypad" keys arranged like this:
 *
 *  A1   up  A3
 * left  B2 right
 *  C1  down C3
 *
 */
#define    KEY_A1         0x15C    /* Keypad upper left */
#define    KEY_A3         0x15D    /* Keypad upper right */
#define    KEY_B2         0x15E    /* Keypad centre key */
#define    KEY_C1         0x15F    /* Keypad lower left */
#define    KEY_C3         0x160    /* Keypad lower right */

#define    KEY_BTAB       0x161    /* Back Tab */
#define    KEY_BEG        0x162    /* Begin key */
#define    KEY_CANCEL     0x163    /* Cancel key */
#define    KEY_CLOSE      0x164    /* Close Key */
#define    KEY_COMMAND    0x165    /* Command Key */
#define    KEY_COPY       0x166    /* Copy key */
#define    KEY_CREATE     0x167    /* Create key */
#define    KEY_END        0x168    /* End key */
#define    KEY_EXIT       0x169    /* Exit key */
#define    KEY_FIND       0x16A    /* Find key */
#define    KEY_HELP       0x16B    /* Help key */
#define    KEY_MARK       0x16C    /* Mark key */
#define    KEY_MESSAGE    0x16D    /* Message key */
#define    KEY_MOVE       0x16E    /* Move key */
#define    KEY_NEXT       0x16F    /* Next Object key */
#define    KEY_OPEN       0x170    /* Open key */
#define    KEY_OPTIONS    0x171    /* Options key */
#define    KEY_PREVIOUS   0x172    /* Previous Object key */
#define    KEY_REDO       0x173    /* Redo key */
#define    KEY_REFERENCE  0x174    /* Ref Key */
#define    KEY_REFRESH    0x175    /* Refresh key */
#define    KEY_REPLACE    0x176    /* Replace key */
#define    KEY_RESTART    0x177    /* Restart key */
#define    KEY_RESUME     0x178    /* Resume key */
#define    KEY_SAVE       0x179    /* Save key */
#define    KEY_SBEG       0x17A    /* Shift begin key */
#define    KEY_SCANCEL    0x17B    /* Shift Cancel key */
#define    KEY_SCOMMAND   0x17C    /* Shift Command key */
#define    KEY_SCOPY      0x17D    /* Shift Copy key */
#define    KEY_SCREATE    0x17E    /* Shift Create key */
#define    KEY_SDC        0x17F    /* Shift Delete Character */
#define    KEY_SDL        0x180    /* Shift Delete Line */
#define    KEY_SELECT     0x181    /* Select key */
#define    KEY_SEND       0x182    /* Send key */
#define    KEY_SEOL       0x183    /* Shift Clear Line key */
#define    KEY_SEXIT      0x184    /* Shift Exit key */
#define    KEY_SFIND      0x185    /* Shift Find key */
#define    KEY_SHELP      0x186    /* Shift Help key */
#define    KEY_SHOME      0x187    /* Shift Home key */
#define    KEY_SIC        0x188    /* Shift Input key */
#define    KEY_SLEFT      0x189    /* Shift Left Arrow key */
#define    KEY_SMESSAGE   0x18A    /* Shift Message key */
#define    KEY_SMOVE      0x18B    /* Shift Move key */
#define    KEY_SNEXT      0x18C    /* Shift Next key */
#define    KEY_SOPTIONS   0x18D    /* Shift Options key */
#define    KEY_SPREVIOUS  0x18E    /* Shift Previous key */
#define    KEY_SPRINT     0x18F    /* Shift Print key */
#define    KEY_SREDO      0x190    /* Shift Redo key */
#define    KEY_SREPLACE   0x191    /* Shift Replace key */
#define    KEY_SRIGHT     0x192    /* Shift Right Arrow key */
#define    KEY_SRSUME     0x193    /* Shift Resume key */
#define    KEY_SSAVE      0x194    /* Shift Save key */
#define    KEY_SSUSPEND   0x195    /* Shift Suspend key */
#define    KEY_SUNDO      0x196    /* Shift Undo key */
#define    KEY_SUSPEND    0x197    /* Suspend key */
#define    KEY_UNDO       0x198    /* Undo key */
#define    KEY_MOUSE      0x199    /* Mouse event has occured */
#define    KEY_MAX        0x199    /* maximum extended key value */

/* 8-bit ASCII characters. */
#define	unctrl(c)		__unctrl[((unsigned)c) & 0xff]
#define	unctrllen(ch)		__unctrllen[((unsigned)ch) & 0xff]

extern char	 *__unctrl[256];	/* Control strings. */
extern char	 __unctrllen[256];	/* Control strings length. */

/*
 * A window an array of __LINE structures pointed to by the 'lines' pointer.
 * A line is an array of __LDATA structures pointed to by the 'line' pointer.
 */

/*
 * Definitions for characters and attributes in __LDATA
 */
#define __CHARTEXT	0x000000ff	/* bits for 8-bit characters */
#define __NORMAL	0x00000000	/* Added characters are normal. */
#define __STANDOUT	0x00010000	/* Added characters are standout. */
#define __UNDERSCORE	0x00020000	/* Added characters are underscored. */
#define __REVERSE	0x00040000	/* Added characters are reverse
					   video. */
#define __BLINK		0x00080000	/* Added characters are blinking. */
#define __DIM		0x00100000	/* Added characters are dim. */
#define __BOLD		0x00200000	/* Added characters are bold. */
#define __BLANK		0x00400000	/* Added characters are blanked. */
#define __PROTECT	0x00800000	/* Added characters are protected. */
#define __ALTCHARSET	0x01000000	/* Added characters are ACS */
#define __COLOR		0x7e000000	/* Color bits */
#define __ATTRIBUTES	0x7fff0000	/* All 8-bit attribute bits */

typedef struct __ldata __LDATA;
typedef struct __line  __LINE;
typedef struct __window  WINDOW;

/*
 * Attribute definitions
 */
#define	A_NORMAL	__NORMAL
#define	A_STANDOUT	__STANDOUT
#define	A_UNDERLINE	__UNDERSCORE
#define	A_REVERSE	__REVERSE
#define	A_BLINK		__BLINK
#define	A_DIM		__DIM
#define	A_BOLD		__BOLD
#define	A_BLANK		__BLANK
#define	A_PROTECT	__PROTECT
#define	A_ALTCHARSET	__ALTCHARSET
#define	A_ATTRIBUTES	__ATTRIBUTES
#define	A_CHARTEXT	__CHARTEXT
#define	A_COLOR		__COLOR

/*
 * Alternate character set definitions
 */

#define	NUM_ACS	128

extern chtype _acs_char[NUM_ACS];

#define	ACS_RARROW	_acs_char['+']
#define	ACS_LARROW	_acs_char[',']
#define	ACS_UARROW	_acs_char['-']
#define	ACS_DARROW	_acs_char['.']
#define	ACS_BLOCK	_acs_char['0']
#define	ACS_DIAMOND	_acs_char['`']
#define	ACS_CKBOARD	_acs_char['a']
#define	ACS_DEGREE	_acs_char['f']
#define	ACS_PLMINUS	_acs_char['g']
#define	ACS_BOARD	_acs_char['h']
#define	ACS_LANTERN	_acs_char['i']
#define	ACS_LRCORNER	_acs_char['j']
#define	ACS_URCORNER	_acs_char['k']
#define	ACS_ULCORNER	_acs_char['l']
#define	ACS_LLCORNER	_acs_char['m']
#define	ACS_PLUS	_acs_char['n']
#define	ACS_HLINE	_acs_char['q']
#define	ACS_S1		_acs_char['o']
#define	ACS_S9		_acs_char['s']
#define	ACS_LTEE	_acs_char['t']
#define	ACS_RTEE	_acs_char['u']
#define	ACS_BTEE	_acs_char['v']
#define	ACS_TTEE	_acs_char['w']
#define	ACS_VLINE	_acs_char['x']
#define	ACS_BULLET	_acs_char['~']

/* System V compatability */
#define	ACS_SBBS	ACS_LRCORNER
#define	ACS_BBSS	ACS_URCORNER
#define	ACS_BSSB	ACS_ULCORNER
#define	ACS_SSBB	ACS_LLCORNER
#define	ACS_SSSS	ACS_PLUS
#define	ACS_BSBS	ACS_HLINE
#define	ACS_SSSB	ACS_LTEE
#define	ACS_SBSS	ACS_RTEE
#define	ACS_SSBS	ACS_BTEE
#define	ACS_BSSS	ACS_TTEE
#define	ACS_SBSB	ACS_VLINE
#define	_acs_map	_acs_char

/*
 * Colour definitions (ANSI colour numbers)
 */

#define	COLOR_BLACK	0x00
#define	COLOR_RED	0x01
#define	COLOR_GREEN	0x02
#define	COLOR_YELLOW	0x03
#define	COLOR_BLUE	0x04
#define	COLOR_MAGENTA	0x05
#define	COLOR_CYAN	0x06
#define	COLOR_WHITE	0x07

#define	COLOR_PAIR(n)	(((n) << 25) & A_COLOR)
#define	PAIR_NUMBER(n)	(((n) & A_COLOR) >> 25)

/* Curses external declarations. */
extern WINDOW	*curscr;		/* Current screen. */
extern WINDOW	*stdscr;		/* Standard screen. */

extern struct termios __orig_termios;	/* Terminal state before curses */
extern struct termios __baset;		/* Our base terminal state */
extern int	__tcaction;		/* If terminal hardware set. */

extern int	 COLS;			/* Columns on the screen. */
extern int	 LINES;			/* Lines on the screen. */
extern int	 COLORS;		/* Max colours on the screen. */
extern int	 COLOR_PAIRS;		/* Max colour pairs on the screen. */

extern char	*ttytype;		/* Full name of current terminal. */

#define	ERR	(0)			/* Error return. */
#define	OK	(1)			/* Success return. */

/*
 * The following have, traditionally, been macros but X/Open say they
 * need to be functions.  Keep the old macros for debugging.
 */
#ifdef _CURSES_USE_MACROS
/* Standard screen pseudo functions. */
#define	addbytes(s, n)			__waddbytes(stdscr, s, n, 0)
#define	addch(ch)			waddch(stdscr, ch)
#define	addnstr(s, n)			waddnstr(stdscr, s, n)
#define	addstr(s)			waddnstr(stdscr, s, -1)
#define bkgd(ch)			wbkgd(stdscr, ch)
#define bkgdset(ch)			wbkgdset(stdscr, ch)
#define	border(l, r, t, b, tl, tr, bl, br) \
	wborder(stdscr, l, r, t, b, tl, tr, bl, br)
#define	clear()				wclear(stdscr)
#define	clrtobot()			wclrtobot(stdscr)
#define	clrtoeol()			wclrtoeol(stdscr)
#define	delch()				wdelch(stdscr)
#define	deleteln()			wdeleteln(stdscr)
#define	erase()				werase(stdscr)
#define	getch()				wgetch(stdscr)
#define	getstr(s)			wgetstr(stdscr, s)
#define	inch()				winch(stdscr)
#define	insch(ch)			winsch(stdscr, ch)
#define	insdelln(n)			winsdelln(stdscr, n)
#define	insertln()			winsertln(stdscr)
#define	move(y, x)			wmove(stdscr, y, x)
#define	refresh()			wrefresh(stdscr)
#define	scrl(n)				wscrl(stdscr, n)
#define	standend()			wstandend(stdscr)
#define	standout()			wstandout(stdscr)
#define	timeout(delay)			wtimeout(stdscr, delay)
#define	underscore()			wunderscore(stdscr)
#define	underend()			wunderend(stdscr)
#define	attron(attr)			wattron(stdscr, attr)
#define	attroff(attr)			wattroff(stdscr, attr)
#define	attrset(attr)			wattrset(stdscr, attr)
#define	waddbytes(w, s, n)		__waddbytes(w, s, n, 0)
#define	waddstr(w, s)			waddnstr(w, s, -1)

/* Standard screen plus movement pseudo functions. */
#define	mvaddbytes(y, x, s, n)		mvwaddbytes(stdscr, y, x, s, n)
#define	mvaddch(y, x, ch)		mvwaddch(stdscr, y, x, ch)
#define	mvaddnstr(y, x, s, n)		mvwaddnstr(stdscr, y, x, s, n)
#define	mvaddstr(y, x, s)		mvwaddstr(stdscr, y, x, s)
#define	mvdelch(y, x)			mvwdelch(stdscr, y, x)
#define	mvgetch(y, x)			mvwgetch(stdscr, y, x)
#define	mvgetstr(y, x, s)		mvwgetstr(stdscr, y, x, s)
#define	mvinch(y, x)			mvwinch(stdscr, y, x)
#define	mvinsch(y, x, c)		mvwinsch(stdscr, y, x, c)
#define	mvwaddbytes(w, y, x, s, n) \
	(wmove(w, y, x) == ERR ? ERR : __waddbytes(w, s, n, 0))
#define	mvwaddch(w, y, x, ch) \
	(wmove(w, y, x) == ERR ? ERR : waddch(w, ch))
#define	mvwaddnstr(w, y, x, s, n) \
	(wmove(w, y, x) == ERR ? ERR : waddnstr(w, s, n))
#define	mvwaddstr(w, y, x, s) \
	(wmove(w, y, x) == ERR ? ERR : waddnstr(w, s, -1))
#define	mvwdelch(w, y, x) \
	(wmove(w, y, x) == ERR ? ERR : wdelch(w))
#define	mvwgetch(w, y, x) \
	(wmove(w, y, x) == ERR ? ERR : wgetch(w))
#define	mvwgetstr(w, y, x, s) \
	(wmove(w, y, x) == ERR ? ERR : wgetstr(w, s))
#define	mvwinch(w, y, x) \
	(wmove(w, y, x) == ERR ? ERR : winch(w))
#define	mvwinsch(w, y, x, c) \
	(wmove(w, y, x) == ERR ? ERR : winsch(w, c))

#define	getyx(w, y, x)		(y) = getcury(w), (x) = getcurx(w)
#define getbegyx(w, y, x)	(y) = getbegy(w), (x) = getbegx(w)
#define getmaxyx(w, y, x)	(y) = getmaxy(w), (x) = getmaxx(w)

#else
/* Use functions not macros... */
__BEGIN_DECLS
int	 addbytes(const char *, int);
int	 addch(chtype);
int	 addnstr(const char *, int);
int	 addstr(const char *);
int	 bkgd(chtype);
void	 bkgdset(chtype);
int	 border(chtype, chtype, chtype, chtype,
	   chtype, chtype, chtype, chtype);
int	 clear(void);
int	 clrtobot(void);
int	 clrtoeol(void);
int	 delch(void);
int	 deleteln(void);
int	 erase(void);
int	 getch(void);
int	 getstr(char *);
chtype	 inch(void);
int	 insch(chtype);
int	 insdelln(int);
int	 insertln(void);
int	 move(int, int);
int	 refresh(void);
int	 scrl(int);
int	 standend(void);
int	 standout(void);
void	 timeout(int);
int	 underscore(void);
int	 underend(void);
int	 attron(int);
int	 attroff(int);
int	 attrset(int);
int	 waddbytes(WINDOW *, const char *, int);
int	 waddstr(WINDOW *, const char *);

/* Standard screen plus movement functions. */
int	 mvaddbytes(int, int, const char *, int);
int	 mvaddch(int, int, chtype);
int	 mvaddnstr(int, int, const char *, int);
int	 mvaddstr(int, int, const char *);
int	 mvdelch(int, int);
int	 mvgetch(int, int);
int	 mvgetstr(int, int, char *);
chtype	 mvinch(int, int);
int	 mvinsch(int, int, chtype);

int	 mvwaddbytes(WINDOW *, int, int, const char *, int);
int	 mvwaddch(WINDOW *, int, int, chtype);
int	 mvwaddnstr(WINDOW *, int, int, const char *, int);
int	 mvwaddstr(WINDOW *, int, int, const char *);
int	 mvwdelch(WINDOW *, int, int);
int	 mvwgetch(WINDOW *, int, int);
int	 mvwgetstr(WINDOW *, int, int, char *);
chtype	 mvwinch(WINDOW *, int, int);
int	 mvwinsch(WINDOW *, int, int, chtype);
__END_DECLS
#endif /* _CURSES_USE_MACROS */

#define	getyx(w, y, x)		(y) = getcury(w), (x) = getcurx(w)
#define	getbegyx(w, y, x)	(y) = getbegy(w), (x) = getbegx(w)
#define	getmaxyx(w, y, x)	(y) = getmaxy(w), (x) = getmaxx(w)

/* Public function prototypes. */
__BEGIN_DECLS
int	 beep(void);
int	 box(WINDOW *, chtype, chtype);
bool	 can_change_colors(void);
int	 cbreak(void);
int	 clearok(WINDOW *, bool);
int	 color_content(short, short *, short *, short *);
int	 copywin(const WINDOW *, WINDOW *, int, int, int, int, int, int, int);
int	 curs_set(int);
int	 delay_output(int);
int	 def_prog_mode(void);
int	 def_shell_mode(void);
int	 delwin(WINDOW *);
WINDOW	*derwin(WINDOW *, int, int, int, int);
WINDOW	*dupwin(WINDOW *);
int	 doupdate(void);
int	 echo(void);
int	 endwin(void);
char     erasechar(void);
int	 flash(void);
int	 flushinp(void);
int	 flushok(WINDOW *, bool);
char	*fullname(const char *, char *);
chtype	 getattrs(WINDOW *);
chtype	 getbkgd(WINDOW *);
char	*getcap(char *);
int	 getcury(WINDOW *);
int	 getcurx(WINDOW *);
int	 getbegy(WINDOW *);
int	 getbegx(WINDOW *);
int	 getmaxy(WINDOW *);
int	 getmaxx(WINDOW *);
int	 gettmode(void);
bool	 has_colors(void);
int	 hline(chtype, int);
int	 idlok(WINDOW *, bool);
int	 init_color(short, short, short, short);
int	 init_pair(short, short, short);
WINDOW	*initscr(void);
int	 intrflush(WINDOW *, bool);
bool	 isendwin(void);
bool	 is_linetouched(WINDOW *, int);
bool	 is_wintouched(WINDOW *);
void	 keypad(WINDOW *, bool);
char     killchar(void);
int	 leaveok(WINDOW *, bool);
char	*longname(void);
int	 meta(WINDOW *, bool);
int	 mvcur(int, int, int, int);
int	 mvhline(int, int, chtype, int);
int	 mvprintw(int, int, const char *, ...)
		__attribute__((__format__(__printf__, 3, 4)));
int	 mvscanw(int, int, const char *, ...)
		__attribute__((__format__(__scanf__, 3, 4)));
int	 mvvline(int, int, chtype, int);
int	 mvwhline(WINDOW *, int, int, chtype, int);
int	 mvwvline(WINDOW *, int, int, chtype, int);
int	 mvwin(WINDOW *, int, int);
int	 mvwprintw(WINDOW *, int, int, const char *, ...)
		__attribute__((__format__(__printf__, 4, 5)));
int	 mvwscanw(WINDOW *, int, int, const char *, ...)
		__attribute__((__format__(__scanf__, 4, 5)));
int	 napms(int);
WINDOW	*newwin(int, int, int, int);
int	 nl(void);
int	 nocbreak(void);
void	 nodelay(WINDOW *, bool);
int	 noecho(void);
int	 nonl(void);
int	 noraw(void);
int	 notimeout(WINDOW *, bool);
int	 overlay(const WINDOW *, WINDOW *);
int	 overwrite(const WINDOW *, WINDOW *);
int	 pair_content(short, short *, short *);
int	 printw(const char *, ...)
		__attribute__((__format__(__printf__, 1, 2)));
int	 raw(void);
int	 reset_prog_mode(void);
int	 reset_shell_mode(void);
int	 resetty(void);
int	 savetty(void);
int	 scanw(const char *, ...)
		__attribute__((__format__(__scanf__, 1, 2)));
int	 scroll(WINDOW *);
int	 scrollok(WINDOW *, bool);
int	 setterm(char *);
int	 start_color(void);
WINDOW	*subwin(WINDOW *, int, int, int, int);
int	 touchline(WINDOW *, int, int);
int	 touchoverlap(WINDOW *, WINDOW *);
int	 touchwin(WINDOW *);
int	 ungetch(int);
int	 untouchwin(WINDOW *);
int	 vline(chtype, int);
int	 vwprintw(WINDOW *, const char *, _BSD_VA_LIST_)
		__attribute__((__format__(__printf__, 2, 0)));
int	 vwscanw(WINDOW *, const char *, _BSD_VA_LIST_)
		__attribute__((__format__(__scanf__, 2, 0)));
int	 waddch(WINDOW *, chtype);
int	 waddnstr(WINDOW *, const char *, int);
int	 wattron(WINDOW *, int);
int	 wattroff(WINDOW *, int);
int	 wattrset(WINDOW *, int);
int	 wbkgd(WINDOW *, chtype);
void	 wbkgdset(WINDOW *, chtype);
int	 wborder(WINDOW *, chtype, chtype, chtype, chtype, chtype, chtype,
		chtype, chtype);
int	 wclear(WINDOW *);
int	 wclrtobot(WINDOW *);
int	 wclrtoeol(WINDOW *);
int	 wdelch(WINDOW *);
int	 wdeleteln(WINDOW *);
int	 werase(WINDOW *);
int	 wgetch(WINDOW *);
int	 wgetstr(WINDOW *, char *);
int	 whline(WINDOW *, chtype, int);
chtype	 winch(WINDOW *);
int	 winsch(WINDOW *, chtype);
int	 winsdelln(WINDOW *, int);
int	 winsertln(WINDOW *);
int	 wmove(WINDOW *, int, int);
int	 wnoutrefresh(WINDOW *);
int	 wprintw(WINDOW *, const char *, ...)
		__attribute__((__format__(__printf__, 2, 3)));
int	 wrefresh(WINDOW *);
int	 wscanw(WINDOW *, const char *, ...)
		__attribute__((__format__(__scanf__, 2, 3)));
int	 wscrl(WINDOW *, int);
int	 wstandend(WINDOW *);
int	 wstandout(WINDOW *);
void	 wtimeout(WINDOW *, int);
int	 wtouchln(WINDOW *, int, int, int);
int	 wunderend(WINDOW *);
int	 wunderscore(WINDOW *);
int	 wvline(WINDOW *, chtype, int);

/* Private functions that are needed for user programs prototypes. */
int	 __cputchar(int);
int	 __waddbytes(WINDOW *, const char *, int, attr_t);
__END_DECLS

#endif /* !_CURSES_H_ */
