#ifndef _SFB_H_3c03197c_
#define _SFB_H_3c03197c_

#define SFB_ASIC_OFFSET   0x0100000
#define SFB_RAMDAC_OFFSET 0x01c0000
#define SFB_FB_OFFSET     0x0200000
#define SFB_OSBM_OFFSET   0x0600000

struct sfb_asic {
  unsigned int copybuf[8];
  unsigned int fg;
  unsigned int bg;
  unsigned int planemask;
  unsigned int pixelmask;
  unsigned int mode;
#define SFB_MODE_SIMPLE         0
#define SFB_MODE_STIPPLE_OPAQUE 1
#define SFB_MODE_LINE_OPAQUE    2
/* 3 and 4 are documented as UNDEFINED */
#define SFB_MODE_STIPPLE_TRANSP 5
#define SFB_MODE_LINE_TRANSP    6
#define SFB_MODE_COPY           7
  unsigned int rop;
#define SFB_ROP_SRC  3
#define SFB_ROP_DST  5
#define SFB_ROP_0    0
#define SFB_ROP_1   15
#define SFB_ROP_AND(x,y) ((x)&(y))
#define SFB_ROP_OR(x,y) ((x)|(y))
#define SFB_ROP_XOR(x,y) ((x)^(y))
#define SFB_ROP_NOT(x) (15&~(x))
  unsigned int pixelshift;
  unsigned int addr;
  unsigned int bres[3];
  unsigned int bcont;
  const unsigned int deep;
  unsigned int start;
  const unsigned int intclear;
  const unsigned int hole1;
  const unsigned int vidrefresh;
  const unsigned int vidhsetup;
  const unsigned int vidvsetup;
  const unsigned int vidbase;
  const unsigned int vidvalid;
  const unsigned int intenb;
  const unsigned int tcclk;
  const unsigned int vidclk;
  } ;

struct sfb_ramdac {
  unsigned int alow;
  unsigned int ahigh;
  unsigned int reg;
  unsigned int cmap;
  } ;

#endif
