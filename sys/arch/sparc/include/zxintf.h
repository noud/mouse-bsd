#ifndef _ZXINTF_H_17a1e157_
#define _ZXINTF_H_17a1e157_

/*
 * mmap offsets.  These are supposedly compatible with Sun OSes.
 */
#define ZX_FB0_VOFF        0x00000000
#define ZX_LC0_VOFF        0x00800000
#define ZX_LD0_VOFF        0x00801000
#define ZX_LX0_CURSOR_VOFF 0x00802000
#define ZX_FB1_VOFF        0x00803000
#define ZX_LC1_VOFF        0x01003000
#define ZX_LD1_VOFF        0x01004000
#define ZX_LX0_VERT_VOFF   0x01005000
#define ZX_LX_KRN_VOFF     0x01006000
#define ZX_LC0_KRN_VOFF    0x01007000
#define ZX_LC1_KRN_VOFF    0x01008000
#define ZX_LD_GBL_VOFF     0x01009000

/*
 * More mmap offsets, this time for us.
 */
#define ZX_CMD0_VOFF  0x02000000
#define ZX_CMD1_VOFF  0x02002000
#define ZX_DRAW0_VOFF 0x02004000
#define ZX_DRAW1_VOFF 0x02006000
#define ZX_CROSS_VOFF 0x02007000

/* Chip code registers all have this format:
	0xf0000000 version
	0x0ffff000 device ID
	0x00000ffe JED manufacturer code
	0x00000001 always set */
#define ZX_CC_VERS_SHIFT 28
#define ZX_CC_VERS_MASK 0xf
#define ZX_CC_DEVID_SHIFT 12
#define ZX_CC_DEVID_MASK 0xffff
#define ZX_CC_JEDMFGR_SHIFT 1
#define ZX_CC_JEDMFGR_MASK 0x7ff
#define ZX_CC_MBO_SHIFT 0
#define ZX_CC_MBO_MASK 1

/* Framebuffer addresses exist in two versions.  The LeoCommand version
   puts the Y bits in the 0x003ff800 bits (sometimes the 0x001ff800 bits)
   and the X bits in 0x000007ff; the LeoDraw version shifts the Y bits over
   five more bits. */
#define ZXC_ADDR_Y_SHIFT 11
#define ZXC_ADDR_Y_MASK 0x7ff
#define ZXC_ADDR_X_SHIFT 0
#define ZXC_ADDR_X_MASK 0x7ff
#define ZXD_ADDR_Y_SHIFT 16
#define ZXD_ADDR_Y_MASK 0x7ff
#define ZXD_ADDR_X_SHIFT 0
#define ZXD_ADDR_X_MASK 0x7ff

/* Window ID lookup table contents are ten-bit values giving pixel
   treatment thus: */
#define ZXWID_CMODE 0x3c0 /* Colour mode and data source */
#define ZXWID_CMODE_TRUE24_A   0x000 /* 24bpp True, buffer A */
#define ZXWID_CMODE_TRUE12_A_H 0x040 /* 12bpp True, buffer A MSBs */
#define ZXWID_CMODE_TRUE12_A_L 0x080 /* 12bpp True, buffer A LSBs */
#define ZXWID_CMODE_TRUE24_B   0x100 /* 24bpp True, buffer B */
#define ZXWID_CMODE_TRUE12_B_H 0x140 /* 12bpp True, buffer B MSBs */
#define ZXWID_CMODE_TRUE12_B_L 0x180 /* 12bpp True, buffer B LSBs */
#define ZXWID_CMODE_PSEUDO_A_R 0x200 /* 8bpp Pseudo, buffer A red */
#define ZXWID_CMODE_PSEUDO_A_G 0x240 /* 8bpp Pseudo, buffer A green */
#define ZXWID_CMODE_PSEUDO_A_B 0x280 /* 8bpp Pseudo, buffer A blue */
#define ZXWID_CMODE_PSEUDO_O   0x2c0 /* 8bpp Pseudo, overlay */
#define ZXWID_CMODE_PSEUDO_B_R 0x300 /* 8bpp Pseudo, buffer B red */
#define ZXWID_CMODE_PSEUDO_B_G 0x340 /* 8bpp Pseudo, buffer B green */
#define ZXWID_CMODE_PSEUDO_B_B 0x380 /* 8bpp Pseudo, buffer B blue */
#define ZXWID_CLUT_SHIFT 4
#define ZXWID_CLUT_MASK 3
#define ZXWID_CLUT_BYPASS 3
#define ZXWID_FC_ENB 0x008
#define ZXWID_FC_SHIFT 0
#define ZXWID_FC_MASK 7

struct zx_bootprom { /* 0x00000000 */
  unsigned char rom[65536];
  /* 31 more copies follow - 0x001f0000 address bits are ignored */
  } ;

/* The LeoCommand registers are rather sparsely scattered.  We collapse
   everything in the same 64K block into a single structure, so we end
   up with two structs instead of seven:
   ss0  0x00200000-0x00200008	State Set 0, Leo Clock Domain
   ss0  0x00200800-0x00200848	State Set 0, SBus Clock Domain
   ss0  0x00201000-0x00201020	State Set 0, Leo Clock Domain
   ss1  0x01200000-0x01200008	State Set 1, Leo Clock Domain
   ss1  0x01200800-0x0120082c	State Set 1, SBus Clock Domain
   ss1  0x01201000-0x0120146c	State Set 1, Leo Clock Domain
   ss1  0x01201800-0x01201814	State Set 1, SBus Clock Domain
   There are probably privilege reasons why these should be kept separate,
   and we may do so at some point. */

struct zx_command_ss0 { /* 0x00200000 */
  unsigned int ie;		/* Leo Domain Interrupt Enable */
#define ZXC_IE_LDSEMA  0x0010 /* LeoDraw Semaphore */
#define ZXC_IE_BLTDONE 0x0008 /* BLT Done */
#define ZXC_IE_SBEMPTY 0x0004 /* Scoreboard Empty */
#define ZXC_IE_CXVERTI 0x0002 /* CXBus Vert Interrupt */
#define ZXC_IE_CDDRAWI 0x0001 /* CDBus Draw Interrupt */
  unsigned int clrbltdone;	/* Clear Blt Done */
  unsigned int clrldsema;	/* Clear LD Semaphore */
  unsigned int hole0[509];
  unsigned int chipcode;	/* LC Chip Code */
  unsigned int sbusstatus;	/* SBus Status */
#define ZXC_SBS_RESET     0x00000200 /* Leo Reset Status */
#define ZXC_SBS_DMARP_ACT 0x00000100 /* DMA Read Pause Active */
#define ZXC_SBS_DMARP     0x00000080 /* DMA Read Pause */
#define ZXC_SBS_ACCRESET  0x00000040 /* Accelerator Port Reset Status */
/* The following six match the low six sbusie bits */
#define ZXC_SBS_RERUN_TMO 0x00000020 /* Slave Rerun Timeout */
#define ZXC_SBS_ILLADDR   0x00000010 /* Slave Illegal Addr */
#define ZXC_SBS_DMAERR    0x00000008 /* DMA Error Acknowledge */
#define ZXC_SBS_INVPT     0x00000004 /* Invalid PTE/PTD */
#define ZXC_SBS_WDMADONE  0x00000002 /* Write DMA Done */
#define ZXC_SBS_RDMADONE  0x00000001 /* Read DMA Done */
  unsigned int sbusie;		/* SBus Interrupt ENable */
#define ZXC_SBIE_PICKFNE   0x00000040 /* Pick FIFO Not Empty */
/* The following six match the low six sbusstatus bits */
#define ZXC_SBIE_RERUN_TMO 0x00000020 /* Slave Rerun Timeout */
#define ZXC_SBIE_ILLADDR   0x00000010 /* Slave Illegal Addr */
#define ZXC_SBIE_DMAERR    0x00000008 /* DMA Error Acknowledge */
#define ZXC_SBIE_INVPT     0x00000004 /* Invalid PTE/PTD */
#define ZXC_SBIE_WDMADONE  0x00000002 /* Write DMA Done */
#define ZXC_SBIE_RDMADONE  0x00000001 /* Read DMA Done */
  unsigned int readtimeout;	/* First Read Timeout Counter */
  unsigned int rerunlimit;	/* Rerun Counter */
  unsigned int hole1[3];
  unsigned int clr_rdmadone;	/* Clear Read DMA Done */
  unsigned int clr_wdmadone;	/* Clear Write DMA Done */
  unsigned int clr_invpt;	/* Clear Invalid PTE/PTD */
  unsigned int clr_dmaerr;	/* Clear DMA Error Acknowledge */
  unsigned int clr_illaddr;	/* Clear Slave Illegal Address */
  unsigned int clr_rerun_tmo;	/* Clear Slave Rerun Timeout */
  unsigned int hole2[2];
  unsigned int set_reset;	/* Leo Reset */
  unsigned int clr_reset;	/* Clear Leo Reset */
  unsigned int rdmapause;	/* DMA Read Pause */
  unsigned int hole3[493];
  unsigned int status;		/* Leo System Status */
#define ZXC_STAT_LDSEMA 0x80000000 /* LeoDraw Semaphore */
#define ZXC_STAT_BLTDONE 0x40000000 /* BLT Done */
#define ZXC_STAT_BLTBUSY 0x20000000 /* BLT Busy */
#define ZXC_STAT_SBEMPTY 0x10000000 /* Scoreboard Empty */
#define ZXC_STAT_CXVERTI 0x08000000 /* CXBus Vert Interrupt */
/* 0x07ff8000 are LeoFloat status signals */
/* 0x00007c00 are LeoDraw interrupt signals */
/* 0x000003e0 are LeoDraw Direct Port buffer available signals */
/* 0x0000001f are LeoDraw Accelerator Port buffer available signals */
  unsigned int fbaddr;		/* Frame Buffer Address Space */
#define ZXC_FB_PIX_OBGR     0
#define ZXC_FB_PIX_Z        1
#define ZXC_FB_PIX_WID      2
#define ZXC_FB_STENCIL_IMG  4
#define ZXC_FB_STENCIL_Z    5
#define ZXC_FB_STENCIL_WID  6
#define ZXC_FB_BYTE_O       8
#define ZXC_FB_BYTE_B       9
#define ZXC_FB_BYTE_G      10
#define ZXC_FB_BYTE_R      11
  unsigned int stencil_mask;	/* Stencil Mask */
  unsigned int stencil_transp;	/* Stencil Transparent Enable */
  unsigned int size_dir;	/* Block Copy/Fill Direction/Size */
#define ZXC_DIR_ULtoLR 0x00000000
#define ZXC_DIR_LRtoUL 0x80000000
/* low bits are X and Y sizes in LeoCommand framebuffer address format */
  unsigned int src;		/* Block Copy Source Address */
#define ZXC_SRC_IMG 0x00000000 /* if ZXC_DST_BYTEMODE clear in dst */
#define ZXC_SRC_Z   0x40000000
#define ZXC_SRC_WID 0x80000000
#define ZXC_SRC_O 0x00000000 /* if ZXC_DST_BYTEMODE set in dst */
#define ZXC_SRC_B 0x40000000
#define ZXC_SRC_G 0x80000000
#define ZXC_SRC_R 0xc0000000
/* low bits are a LeoCommand framebuffer address */
  unsigned int dst;		/* Block Copy/Fill Destination Address */
#define ZXC_DST_BYTEMODE 0x80000000
/* if ZXC_DST_BYTEMODE clear, destination plane given by... */
#define ZXC_DST_IMG 0x00000000
#define ZXC_DST_Z   0x20000000
#define ZXC_DST_WID 0x40000000
#define ZXC_DST_I_Z 0x60000000
/* if ZXC_DST_BYTEMODE set, destination plane(s) given by LeoDraw imgwmask */
/* low bits are a LeoCommand framebuffer address */
  unsigned int dst_copy;	/* just like dst except starts copy */
  unsigned int dst_fill;	/* just like dst except starts fill */
  } ;

struct zx_command_ss1 { /* 0x01200000 */
  unsigned int lf_enable;	/* LeoFloat Enable */
  unsigned int lf_intrun;	/* Trigger LeoFloat Interrupt/Run */
  unsigned int wpickfifo;	/* Write Pick FIFO */
  unsigned int hole0[509];
  unsigned int ptroot;		/* Table Walk Root Pointer */
  unsigned int rdmapt;		/* DMA Read PTE/PTD */
  unsigned int wdmabufstart;	/* DMA Write Buffer Start Address */
  unsigned int wdmabufsize;	/* DMA Write Buffer Size */
  unsigned int wdmaaddr;	/* DMA Write Current Buffer Address */
  unsigned int wdmasize_go;	/* DMA Write Current Buffer Size (start) */
  unsigned int wdmasize;	/* DMA Write Current Buffer Size (no start) */
  unsigned int wdmacount;	/* DMA Write Word Count */
  unsigned int dmaconfig;	/* DMA Configuration */
/* The 0x0e000000 bits are documented as being part of this field too,
   but all the documented combinations show them as being don't-cares. */
#define ZXC_DMACONF_SIZE_BURST_SHIFT 28
#define ZXC_DMACONF_SIZE_BURST_MASK 0x0f
#define ZXC_DMACONF_SIZE_BURST_64_1  0x0 /* 64-bit, 1-doubleword */
#define ZXC_DMACONF_SIZE_BURST_64_2  0x4 /* 64-bit, 2-doubleword */
#define ZXC_DMACONF_SIZE_BURST_64_4  0x5 /* 64-bit, 4-doubleword */
#define ZXC_DMACONF_SIZE_BURST_64_8  0x6 /* 64-bit, 8-doubleword */
#define ZXC_DMACONF_SIZE_BURST_32_1  0x8 /* 32-bit, 1-word */
#define ZXC_DMACONF_SIZE_BURST_32_4  0xc /* 32-bit, 4-word */
#define ZXC_DMACONF_SIZE_BURST_32_8  0xd /* 32-bit, 8-word */
#define ZXC_DMACONF_SIZE_BURST_32_16 0xe /* 32-bit, 16-word */
#define ZXC_DMACONF_CIRCULAR 0x01000000
  unsigned int wdma_run;	/* DMA Write On/Off */
  unsigned int acc_reset;	/* Reset Accelerator Port */
  unsigned int clr_acc_reset;	/* Clear Accelerator Port Reset */
  unsigned int hole1[500];
  unsigned int vertexbuf[48];	/* Vertex Buffer */
  unsigned int altv[3][3];	/* Alternative Vertex Tuples */
  unsigned int ld_dispatch;	/* LeoFloat Dispatch */
  unsigned int hole2[6];
  unsigned int abs_bucket[32];	/* Absolute Bucket Buffer */
  unsigned int hole3[32];
  unsigned int rel_bucket[32];	/* Relative Bucket Buffer */
  unsigned int hole4[32];
  unsigned int go_bucket[32];	/* Launch Relative Bucket Buffer */
  unsigned int hole5[32];
  unsigned int acc_status;	/* Accelerator Port Status */
#define ZXC_ACCSTAT_VBBUSY   0x00000800 /* Vertex Buffer Busy */
#define ZXC_ACCSTAT_BBBUSY   0x00000400 /* Bucket Buffer Busy */
#define ZXC_ACCSTAT_BBAVAIL  0x00000200 /* Bucket Buffer Space Available */
#define ZXC_ACCSTAT_CSWITCH  0x00000100 /* Context Switch Mode */
#define ZXC_ACCSTAT_PASSTHRU 0x00000080 /* Passthrough Mode */
#define ZXC_ACCSTAT_VERTEX   0x00000040 /* Vertex Mode */
#define ZXC_ACCSTAT_DMA      0x00000020 /* DMA Mode */
#define ZXC_ACCSTAT_BBWORDS_SHIFT 0 /* Bucket Buffer Words Available */
#define ZXC_ACCSTAT_BBWORDS_MASK 0x1f
  unsigned int start_vertex;	/* Start Vertex Mode */
  unsigned int start_passthru;	/* Start Passthrough Mode */
  unsigned int start_cswitch;	/* Start Context Switch Mode */
  unsigned int start_idle;	/* Exit other modes */
  /* BEGIN same as like-named ss0 registers */
  unsigned int fbaddr;
  unsigned int stencil_mask;
  unsigned int stencil_transp;
  /* END same as like-named ss0 registers */
  unsigned int vertex_mode;	/* Vertex Mode Control */
#define ZXC_VXMODE_SHREDGE  0x00080000 /* Triangle Shared Edge Mode */
#define ZXC_VXMODE_PICKCNT  0x00040000 /* Subelement PID Count with Header */
#define ZXC_VXMODE_HDRSRC   0x00020000 /* Header Source */
#define ZXC_VXMODE_PKTSIZE_SHIFT 12    /* Input Packet Size */
#define ZXC_VXMODE_PKTSIZE_MASK 0x1f
#define ZXC_VXMODE_PKTSIZE_OFFSET 1
#define ZXC_VXMODE_VTXDMA   0x00000800 /* Vertex DMA Mode */
#define ZXC_VXMODE_POLYLINE 0x00000400 /* Polyline Mode */
#define ZXC_VXMODE_EDGE     0x00000200 /* Edge Mode */
#define ZXC_VXMODE_BACKFACE 0x00000100 /* Backface Function */
#define ZXC_VXMODE_PICKCTL  0x000000e0 /* Subelement Pick ID Control */
#define ZXC_VXMODE_FACETNRM 0x00000010 /* Facet Normal Enable */
#define ZXC_VXMODE_VTXFMT_SHIFT 2      /* Vertex Format */
#define ZXC_VXMODE_VTXFMT_MASK 3
#define ZXC_VXMODE_VTXFMT_XYZ2 0 /* XYZ + tuple 1 + tuple 2 */
#define ZXC_VXMODE_VTXFMT_XYZ1 1 /* XYZ + tuple 1 */
#define ZXC_VXMODE_VTXFMT_XYZ  2 /* XYZ (also 3) */
#define ZXC_VXMODE_VTXTYPE_SHIFT 0     /* Vertex Type */
#define ZXC_VXMODE_VTXTYPE_MASK 3
#define ZXC_VXMODE_VTXTYPE_RESERVED 0
#define ZXC_VXMODE_VTXTYPE_DOT      1
#define ZXC_VXMODE_VTXTYPE_LINE     2
#define ZXC_VXMODE_VTXTYPE_TRIANGLE 3
  unsigned int misc1[5];	/* not documented here */
  unsigned int hole6;
  unsigned int misc2[13];	/* not documented here */
  unsigned int hole7[228];
  unsigned int dmastatus;	/* DMA Status */
#define ZXC_DMASTAT_PICKFE   0x00000010 /* Pick FIFO Empty */
#define ZXC_DMASTAT_WDMABUSY 0x00000008 /* Write DMA Busy */
#define ZXC_DMASTAT_RDMABUSY 0x00000004 /* Read DMA Busy */
#define ZXC_DMASTAT_WDMAON   0x00000002 /* Write DMA On */
#define ZXC_DMASTAT_RDMAON   0x00000001 /* Read DMA On */
  unsigned int rdma_run;	/* DMA Read On/Off */
  unsigned int rdmasize_go;	/* DMA Read Word Count, Start DMA Read */
  unsigned int rdmasize;	/* DMA Read Word Count, Do Not Start */
  unsigned int rdmaaddr;	/* DMA Read Virtual Address */
  unsigned int rpickfifo;	/* Read Pick FIFO */
  } ;

struct zx_draw_ss0 { /* 0x00400000 */
  unsigned int status;		/* LD CSR */
#define ZXD_STAT_ACCRESET 0x00000080 /* Accelerator Port Reset */
/* 0x00000070 bits are PIDs In Pipeline */
#define ZXD_STAT_PICKHIT  0x00000008 /* LD Pick Hit */
#define ZXD_STAT_LDSEMA   0x00000004 /* LD Semaphore */
#define ZXD_STAT_STALL    0x00000002 /* Stall Accelerator */
#define ZXD_STAT_STALLED  0x00000001 /* Accelerator Stalled */
  unsigned int wid;		/* Current Window ID */
#define ZXD_WID_PWID_SHIFT 4
#define ZXD_WID_PWID_MASK 0x3f
#define ZXD_WID_QWID_SHIFT 0
#define ZXD_WID_QWID_MASK 0xf
  unsigned int wwmask;		/* Window Write Mask */
  unsigned int widclip;		/* WID Clip Mask */
  unsigned int vclipmin;	/* View Clip Minimum Bound */
/* Value is a LeoDraw framebuffer address */
  unsigned int vclipmax;	/* View Clip Maximum Bound */
/* Value is a LeoDraw framebuffer address */
  unsigned int pad0[2];		/* SS1 only */
  unsigned int fg;		/* Stencil/Fill Foreground Colour */
  unsigned int bg;		/* Stencil Background Colour */
  unsigned int srcaddr;		/* Copy/Scroll Source Address */
#define ZXD_SRC_IMG 0x00000000 /* if ZXD_DST_BYTEMODE clear in dst */
#define ZXD_SRC_Z   0x40000000
#define ZXD_SRC_WID 0x80000000
#define ZXD_SRC_O 0x00000000 /* if ZXD_DST_BYTEMODE set in dst */
#define ZXD_SRC_B 0x40000000
#define ZXD_SRC_G 0x80000000
#define ZXD_SRC_R 0xc0000000
  unsigned int dstaddr;		/* Copy/Scroll Destination Address */
#define ZXD_DST_BYTEMODE 0x80000000
/* if ZXD_DST_BYTEMODE clear, destination plane given by... */
#define ZXD_DST_IMG 0x00000000
#define ZXD_DST_Z   0x20000000
#define ZXD_DST_WID 0x40000000
#define ZXD_DST_I_Z 0x60000000
/* if ZXD_DST_BYTEMODE set, destination plane(s) given by imgwmask */
/* low bits are a LeoDraw framebuffer address */
  unsigned int size;		/* Copy/Scroll/Fill Size/Direction */
#define ZXD_DIR_ULtoLR 0x00000000
#define ZXD_DIR_LRtoUL 0x80000000
  unsigned int hole0[3];
  unsigned int pad2[5];		/* SS1 only */
  unsigned int hole1[11];
  unsigned int winbg;		/* Window Background Colour */
  unsigned int imgwmask;	/* Image Write Mask */
  unsigned int attr;		/* LD Attribute */
#define ZXD_ATTR_PICK_ENABLE   0x80000000 /* enable picking - SS1 only */
#define ZXD_ATTR_PICK_RENDER   0x40000000 /* render while picking - SS1 only */
#define ZXD_ATTR_PICK_3D       0x20000000 /* 3D pick aperture - SS1 only */
#define ZXD_ATTR_DCE           0x10000000 /* depth cue enable - SS1 only */
#define ZXD_ATTR_SDE           0x08000000 /* screen door enable - SS1 only */
#define ZXD_ATTR_CFORCE        0x04000000 /* force colour enable - SS1 only */
#define ZXD_ATTR_AA            0x02000000 /* antialias enable - SS1 only */
#define ZXD_ATTR_BLEND_FN      0x01000000 /* blend function select - SS1 only */
#define ZXD_ATTR_BLEND_ENB     0x00800000 /* blend enable - SS1 only */
#define ZXD_ATTR_BLTSRC        0x00400000 /* BLT source - SS0 only */
#define ZXD_ATTR_ROP           0x003c0000 /* ROP code - both */
#define ZXD_ROP_SRC 0x00300000
#define ZXD_ROP_DST 0x00280000
#define ZXD_ROP_MASK(x) ((x)&ZX_ROP)
#define ZXD_ROP_AND(x,y) ZX_ROP_MASK((x)&(y))
#define ZXD_ROP_OR(x,y) ZX_ROP_MASK((x)|(y))
#define ZXD_ROP_XOR(x,y) ZX_ROP_MASK((x)^(y))
#define ZXD_ROP_NOT(x) ZX_ROP_MASK(~(x))
#define ZXD_ATTR_HSR           0x00020000 /* hidden surface removal enable - both */
#define ZXD_ATTR_WRITEZ        0x00010000 /* Z write enable - both */
#define ZXD_ATTR_Z             0x00008000 /* constant Z enable - both */
#define ZXD_ATTR_WCLIP         0x00004000 /* WID extension clip enable - both */
#define ZXD_ATTR_STEREO_RIGHT  0x00002000 /* stereo right/left - both */
#define ZXD_ATTR_STEREO_ENABLE 0x00001000 /* stereo enable - both */
#define ZXD_ATTR_WIDGE         0x00000800 /* window ID group enable - both */
#define ZXD_ATTR_FCE           0x00000400 /* fast clear enable - both */
#define ZXD_ATTR_RE            0x00000200 /* red plane enable - both */
#define ZXD_ATTR_GE            0x00000100 /* green plane enable - both */
#define ZXD_ATTR_BE            0x00000080 /* blue plane enable - both */
#define ZXD_ATTR_OE            0x00000040 /* overlay plane enable - both */
#define ZXD_ATTR_ZE            0x00000020 /* Z plane enable - both */
#define ZXD_ATTR_FORCE_WID     0x00000010 /* force current WID - both */
#define ZXD_ATTR_FC_PLANE_MASK 0x0000000e /* fast-clear plane select - both */
#define ZXD_ATTR_FC_PLANE(n) ((n)<<1)
#define ZXD_ATTR_BUFFER        0x00000001 /* double-buffer buffer - both */
  unsigned int z;		/* Constant Z Source */
  unsigned int hole2[4];
  unsigned int pad3[21];	/* SS1 only */
  unsigned int hole3[963];
  unsigned int scrstart;	/* Screen Start Address Left (Even) */
  unsigned int scrstart2;	/* Screen Start Address Right (Odd) */
  unsigned int scroff;		/* Screen Offset Left (Even) */
  unsigned int scroff2;		/* Screen Offset Right (Odd) */
  unsigned int vidctr;		/* Video Counter */
  unsigned int hole4[3];
  unsigned int fbwid;		/* Frame Buffer Width */
  unsigned int interleave;	/* LD Interleave - actually, chip number */
  unsigned int chipcode;	/* LD Chip ID Code */
  unsigned int setstall;	/* Set Stall LD Accelerator */
  unsigned int clrstall;	/* Clear Stall LD Accelerator */
  unsigned int accreset;	/* Reset LD Accelerator Port */
  unsigned int clraccreset;	/* Clear LD Accelerator Port Reset */
  } ;

struct zx_draw_ss1 { /* 0x01400000 */
  unsigned int status;		/* LD CSR */
  unsigned int wid;		/* Current Window ID */
  unsigned int wwmask;		/* Window Write Mask */
  unsigned int widclip;		/* WID Clip Mask */
  unsigned int vclipmin;	/* View Clip Minimum Bound */
  unsigned int vclipmax;	/* View Clip Maximum Bound */
  unsigned int pickmin;		/* Pick Minimum Bound */
/* X and Y of UL pixel of pick aperture as a LeoDraw framebuffer address */
  unsigned int pickmax;		/* Pick Maximum Bound */
/* X and Y of LR pixel of pick aperture as a LeoDraw framebuffer address */
  unsigned int fg;		/* Stencil/Fill Foreground Colour */
  unsigned int bg;		/* Stencil Background Colour */
  unsigned int pad1[3];		/* SS0 only */
  unsigned int hole0[3];
  unsigned int setsema;		/* Set LD Semaphore */
  unsigned int clrsema;		/* Clear LD Semaphore */
  unsigned int clrpick;		/* Clear Pick Hit */
  unsigned int fcdata;		/* Fast Clear Data */
#define ZXD_FCDATA_SHIFT 10
#define ZXD_FCDATA_MASK 0x3f
  unsigned int alpha;		/* Constant Alpha Source */
  unsigned int hole1[11];
  unsigned int winbg;		/* Window Background Colour */
  unsigned int imgwmask;	/* Image Write Mask */
  unsigned int attr;		/* LD Attribute */
  unsigned int z;		/* Constant Z Source */
  unsigned int hole2[4];
  unsigned int dcfront;		/* Depth Cue Z-Front */
  unsigned int dcback;		/* Depth Cue Z-Back */
  unsigned int dcscale;		/* Depth Cue Scale */
  unsigned int dczscale;	/* Depth Cue Z-Scale */
  unsigned int pickfront;	/* Pick Front Bound */
  unsigned int pickback;	/* Pick Back Bound */
  unsigned int dcfade;		/* Depth Cue Fade Colour */
  unsigned int forcecol;	/* Force Colour */
  unsigned int screendoor[8];	/* Screen Door */
  unsigned int pickid[5];	/* Pick ID Register 0..4 */
  } ;

struct zx_cross { /* 0x00600000 and 0x01600000 */
  unsigned int lxaddr;		/* indirect number for lxctl/lxcol */
  unsigned int lxctl;		/* various control registers */
/* 0x0000..0x0fff documented as reserved */
#define ZXX_XADDR_ICSR0            0x1000 /* Image 0 CSR */
#define ZXX_XADDR_ICSR1            0x1001 /* Image 1 CSR */
#define ZXX_XADDR_ICSR2            0x1002 /* Image 2 CSR */
#define ZXX_ICSR_BUSY 0x4 /* Device Status */
#define ZXX_ICSR_XCMD 0x2 /* Transfer Command */
#define ZXX_ICSR_XDIR 0x1 /* Transfer Direction */
#define ZXX_XADDR_WID_FC_CSR       0x1003 /* WID/FC CSR */
#define ZXX_WFCSR_BUSY 0x4 /* Device Status */
#define ZXX_WFCSR_XCMD 0x2 /* Transfer Command */
#define ZXX_WFCSR_XDIR 0x1 /* Transfer Direction */
/* 0x1004 not documented */
#define ZXX_XADDR_INTR_EVENT       0x1005 /* Interrupt Event */
#define ZXX_XADDR_INTR_ENB         0x1006 /* Interrupt Enable Mask */
#define ZXX_XADDR_INTR_CLR         0x1007 /* Interrupt Clear Mask */
/* 0x1008..0x1fff documented as reserved */
#define ZXX_XADDR_CHIPCODE         0x2000 /* LX Chip Code */
#define ZXX_XADDR_MONITOR_ID       0x2001 /* Monitor ID */
#if 0
/* This produces redefinitions for the 1152_900_66 values... */
#define ZXX_MONID_1024_768_76  0 /* 1024x768, 76Hz */
#define ZXX_MONID_1152_900_66  1 /* 1152x900, 66Hz */
#define ZXX_MONID_1280_1024_76 2 /* 1280x1024, 76Hz */
#define ZXX_MONID_1152_900_66  3 /* 1152x900, 66Hz */
#define ZXX_MONID_1280_1024_67 4 /* 1280x1024, 67Hz */
#define ZXX_MONID_1152_900_66  5 /* 1152x900, 66Hz */
#define ZXX_MONID_1152_900_76  6 /* 1152x900, 76Hz */
#define ZXX_MONID_NONE         7 /* No monitor (defaults to 1152x900@66) */
#endif
#define ZXX_XADDR_DRAM_REFRESH     0x2002 /* DRAM Refresh Count */
#define ZXX_XADDR_CONFIG_0         0x2003 /* Configuration Register 0 */
#define ZXX_CONF0_FRAMECOUNT       0x8000 /* Frame Counter Enable */
#define ZXX_CONF0_REFRESH_DISABLE  0x4000 /* DRAM Refresh Counter Disable */
#define ZXX_CONF0_IFFT             0x2000 /* Interlaced First Field to Transfer */
#define ZXX_CONF0_1280x1024        0x1000 /* 1280x1024 Enable */
#define ZXX_CONF0_SHUTTER_POLARITY 0x0800 /* Stereo Shutter Polarity */
#define ZXX_CONF0_STEREO_MONITOR   0x0400 /* Stereo Monitor */
#define ZXX_CONF0_STEREO_FFT       0x0200 /* Stereo First Field to Transfer */
#define ZXX_CONF0_ILACE_MONITOR    0x0100 /* Interlaced Monitor */
#define ZXX_CONF0_IFF              0x0080 /* Interlaced First Field */
#define ZXX_CONF0_EQ_ENB           0x0040 /* Equalization Pulse Enable */
#define ZXX_CONF0_SERR_ENB         0x0020 /* Serration Pulse Enable */
#define ZXX_CONF0_SYNC_ON_VID      0x0010 /* Synch On Video */
#define ZXX_CONF0_VIDEO_ENB        0x0008 /* Video Enable */
#define ZXX_CONF0_BLANK_POLARITY   0x0004 /* Composite Blank Polarity */
#define ZXX_CONF0_SYNC_ENB         0x0002 /* Synch Enable */
#define ZXX_CONF0_SYNC_POLARITY    0x0001 /* Composite Synch Polarity */
#define ZXX_XADDR_CONFIG_1         0x2004 /* Configuration Register 1 */
#define ZXX_CONF1_SOFTRESET    0x4000 /* Soft System Reset */
#define ZXX_CONF1_SYNTH_RIGHT  0x2000 /* Synthesized Right Field */
#define ZXX_CONF1_SYNTH_ODD    0x1000 /* Synthesized Odd Field */
#define ZXX_CONF1_SYNTH_HSYNC  0x0800 /* Synthesized Horiz Synch */
#define ZXX_CONF1_INC_FRAME    0x0400 /* Increment Frame Counter */
#define ZXX_CONF1_SYNTH_VBLANK 0x0200 /* Synthesized Vertical Blank */
#define ZXX_CONF1_CLUT_ENB     0x0100 /* Active CLUT Enable */
#define ZXX_CONF1_INC_CURS_AP  0x0080 /* Increment Cursor Address Pointer */
#define ZXX_CONF1_INC_LX_AP    0x0040 /* Increment LX Address Pointer */
#define ZXX_CONF1_INC_CLUT_XC  0x0020 /* Increment CLUT Transfer Counter */
#define ZXX_CONF1_INC_WID_XC   0x0010 /* Increment WID Transfer Counter */
#define ZXX_CONF1_INC_HCOUNT   0x0008 /* Increment Horizontal Counter */
#define ZXX_CONF1_INC_VCOUNT   0x0004 /* Increment Vertical Counter */
#define ZXX_CONF1_INC_REFRESH  0x0002 /* Increment DRAM Refresh Counter */
#define ZXX_CONF1_DIAG_ENB     0x0001 /* Diagnostic Enable */
#define ZXX_XADDR_CONFIG_2         0x2005 /* Configuration Register 2 */
#define ZXX_XADDR_CONFIG_3         0x2006 /* Configuration Register 3 */
#define ZXX_CONF3_VSYNC_CMP  0x20 /* Vertical Sync Test Comparator */
#define ZXX_CONF3_EQ2_CMP    0x10 /* Equalization Interval 2 Test Comparator */
#define ZXX_CONF3_EQ1_CMP    0x08 /* Equalization Interval 1 Test Comparator */
#define ZXX_CONF3_VBLANK_CMP 0x04 /* Vertical Blank Test Comparator */
#define ZXX_CONF3_HBLANK_CMP 0x02 /* Horizontal Blank Test Comparator */
#define ZXX_CONF3_CSYNC_CMP  0x01 /* Composite Sync Test Comparator */
#define ZXX_XADDR_HSIZE            0x2007 /* Horizontal Counter Size */
#define ZXX_XADDR_HBLANK_END       0x2008 /* HBlank End Address */
#define ZXX_XADDR_HBLANK_START     0x2009 /* HBlank Start Address */
#define ZXX_XADDR_HCOUNT           0x200a /* Horizontal Counter */
#define ZXX_XADDR_HSYNC_END        0x200b /* HSynch End Address */
#define ZXX_XADDR_HSYNC_START      0x200c /* HSynch Start Address */
#define ZXX_XADDR_VSIZE            0x200d /* Vertical Counter Size */
#define ZXX_XADDR_VBLANK_END       0x200e /* VBlank End Address */
#define ZXX_XADDR_VBLANK_START     0x200f /* VBlank Start Address */
#define ZXX_XADDR_VSYNC_END        0x2010 /* VSynch End Address */
#define ZXX_XADDR_VSYNC_START      0x2011 /* VSynch Start Address */
#define ZXX_XADDR_VCOUNT           0x2012 /* Vertical Counter */
#define ZXX_XADDR_EQ_END           0x2013 /* Equalization Pulse End Address */
#define ZXX_XADDR_EQ_START         0x2014 /* Equalization Pulse Start Address */
#define ZXX_XADDR_EQ1_END          0x2015 /* Equalization Interval 1 End */
#define ZXX_XADDR_EQ1_START        0x2016 /* Equalization Interval 1 Start */
#define ZXX_XADDR_EQ2_END          0x2017 /* Equalization Interval 2 End */
#define ZXX_XADDR_EQ2_START        0x2018 /* Equalization Interval 2 Start */
/* 0x2019 not documented */
#define ZXX_XADDR_SERR_END         0x201a /* Serration Pulse End Address */
#define ZXX_XADDR_SERR_START       0x201b /* Serration Pulse Start Address */
/* 0x201c not documented */
#define ZXX_XADDR_EXMACH_COUNT     0x201c /* EXmach Loop Count */
#define ZXX_XADDR_EXMACH_WID_START 0x201e /* EXmach WID Start Address */
#define ZXX_XADDR_EXMACH_IMG_START 0x201f /* EXmach Image Start Address */
/* 0x2020 not documented */
#define ZXX_XADDR_SHUTTER_SWITCH   0x2021 /* Stereo Shutter Switch */
#define ZXX_XADDR_CLUT_XCOUNT      0x2022 /* CLUT Transfer Counter */
#define ZXX_XADDR_WID_XCOUNT       0x2023 /* WID Transfer Counter */
#define ZXX_XADDR_DRAM_RFCOUNT     0x2024 /* DRAM Refresh Counter */
/* 0x2025..0x2026 documented as reserved */
#define ZXX_XADDR_VCLK_GEN         0x2027 /* Video Clock Generator */
#define ZXX_VCLK_PRESCALE_SHIFT 6 /* Prescale selection */
#define ZXX_VCLK_PRESCALE_MASK 3
#define ZXX_VCLK_PRESCALE_DIV1 0 /* ÷1 */
#define ZXX_VCLK_PRESCALE_DIV2 1 /* ÷2 */
#define ZXX_VCLK_PRESCALE_DIV4 2 /* ÷4 */
#define ZXX_VCLK_PRESCALE_RESV 3 /* Reserved */
#define ZXX_VCLK_SYNTH_STROBE 0x20
#define ZXX_VCLK_ADDR_DATA    0x0f
/* 0x2028..0x2fff documented as reserved */
/* 0x3000..0xffff not documented */
  unsigned int lxcol;		/* various colour fields */
/* 0x0000..0x2fff not documented */
/* 0x3308..0x33ff documented as reserved */
/* 0x3708..0x3fff documented as reserved */
/* 0x4106..0x41ff documented as reserved */
/* 0x4200..0x4fff not documented */
/* 0x5N50..0x5Nff (N in 0..4) not documented */
/* 0x5500..0x57ff documented as reserved */
/* 0x5850..0x5fff documented as reserved */
/* 0x6000..0xffff not documented */
#define ZXX_XADDR_ACTIVE_CLUT(bank,clut,word) (0x3000+((bank)*0x400)+((clut)*0x100)+(word))
#define ZXX_XADDR_ACTIVE_FCCOL(bank,num) (0x3300+((bank)*0x400)+(num))
#define ZXX_XADDR_ACTIVE_CURSCOL(bank,col) (0x3306+((bank)*0x400)+(col))
#define ZXX_XADDR_ACTIVE_PWID(bank,cell) (0x5000+((bank)*0x100)+(cell))
#define ZXX_XADDR_ACTIVE_QWID(bank,cell) (0x5040+((bank)*0x100)+(cell))
#define ZXX_XADDR_SHADOW_CLUT(word) (0x4000+(word))
#define ZXX_XADDR_SHADOW_FCCOL(num) (0x4100+(num))
#define ZXX_XADDR_SHADOW_PWID(cell) (0x5800+(cell))
#define ZXX_XADDR_SHADOW_QWID(cell) (0x5840+(cell))
  unsigned int hole0[5];
  unsigned int dacaddr;		/* RAMDAC Address Pointer */
  unsigned int dacclut;		/* RAMDAC Colour Table */
  unsigned int dacctl;		/* RAMDAC Control Registers */
#define ZXX_DACADDR_PIXTEST 0x00 /* Pixel Test */
#define ZXX_DACADDR_DACTEST 0x01 /* DAC Test */
#define ZXX_DACADDR_SBITEST 0x02 /* Sync, Blank, & IPLL Test */
#define ZXX_DACADDR_ID      0x03 /* ID */
#define ZXX_DACADDR_PIXMASK 0x04 /* Pixel Mask */
/* 0x05 not documented */
#define ZXX_DACADDR_CMDREG2 0x06 /* Command Register 2 */
#define ZXX_DACCMD2_CMODECTL_SHIFT 4 /* True Colour & Pseudo Colour Mode Control */
#define ZXX_DACCMD2_CMODECTL_MASK 0x0f
#define ZXX_DACCMD2_CMODECTL_PSEUDO8_R 0x0 /* 8-bit Pseudo on R7:R0 */
#define ZXX_DACCMD2_CMODECTL_PSEUDO8_G 0x4 /* 8-bit Pseudo on G7:G0 */
#define ZXX_DACCMD2_CMODECTL_PSEUDO8_B 0x8 /* 8-bit Pseudo on B7:B0 */
#define ZXX_DACCMD2_CMODECTL_TRUE12_HI 0xc /* 12-bit True on [RGB]7:4 */
#define ZXX_DACCMD2_CMODECTL_TRUE12_RG 0xd /* 12-bit True on R6:3 R1:0 G6:5 G3:0 */
#define ZXX_DACCMD2_CMODECTL_TRUE24    0xe /* 24-bit True on [RGB]7:0 */
#define ZXX_DACCMD2_PEDENB    0x08 /* Pedestal Enable Control */
#define ZXX_DACCMD2_SYNCRECOG 0x04 /* Sync Recognition Control */
#define ZXX_DACCMD2_IPLLTRIG  0x02 /* IPLL Trigger Control */
#define ZXX_DACCMD2_R7TRIGPOL 0x01 /* R7 Trigger Polarity Control */
#define ZXX_DACADDR_CMDREG3 0x07 /* Command Register 3 */
#define ZXX_DACCMD3_PIXMULT_SHIFT 6
#define ZXX_DACCMD3_PIXMULT_MASK 3
#define ZXX_DACCMD3_PIXMULT_1_1 0 /* 1:1 muxing, loadout=clk/1 */
#define ZXX_DACCMD3_PIXMULT_2_1 1 /* 2:1 muxing, loadout=clk/2 */
/* 2 and 3 documented as reserved */
#define ZXX_DACCMD3_BLANKDELAY_SHIFT 2
#define ZXX_DACCMD3_BLANKDELAY_MASK 0x0f
#define ZXX_DACCMD3_PRGCKOUT_SHIFT 0
#define ZXX_DACCMD3_PRGCKOUT_MASK 3
#define ZXX_DACCMD3_PRGCKOUT_DIV_4  0 /* prgckout = clock/4 */
#define ZXX_DACCMD3_PRGCKOUT_DIV_8  1 /* prgckout = clock/8 */
#define ZXX_DACCMD3_PRGCKOUT_DIV_16 2 /* prgckout = clock/16 */
#define ZXX_DACCMD3_PRGCKOUT_DIV_32 3 /* prgckout = clock/32 */
/* 0x08..0xff not documented */
  unsigned int dacmode;		/* RAMDAC Mode */
#define ZXX_DACMODE_PALETTE    0xc0 /* Palette Select Control */
#define ZXX_DACMODE_OPMODE     0x18 /* Operational Mode Control */
#define ZXX_DACMODE_BUSWIDTH   0x04 /* XCBus Data Width */
#define ZXX_DACMODE_RESOLUTION 0x02 /* Resolution Control */
#define ZXX_DACMODE_RESET      0x01 /* Reset Control */
  unsigned int hole1[1016];
  unsigned int cursaddr;	/* indirect number for cursfxn */
  unsigned int curscsr;		/* Cursor CSR */
#define ZXX_CURS_ENB        0x80 /* Cursor Enable */
#define ZXX_CURS_COORD_BUSY 0x40 /* Cursor Coordinate Busy */
#define ZXX_CURS_COORD_XCMD 0x20 /* Cursor Coordinate Transfer Command */
#define ZXX_CURS_COORD_XDIR 0x10 /* Cursor Coordinate Transfer Direction */
#define ZXX_CURS_TRAPENB    0x08 /* Enable Trap Register */
#define ZXX_CURS_CTBL_BUSY  0x04 /* Cursor Colour Table Busy */
#define ZXX_CURS_CTBL_XCMD  0x02 /* Cursor Colour Table Transfer Command */
#define ZXX_CURS_CTBL_XDIR  0x01 /* Cursor Colour Table Transfer Direction */
  unsigned int cursloc;
#define ZXX_CURS_TRAPRIGHT 0x00800000 /* Trap Stereo Right Frame */
/* low bits are cursor position as a LeoCommand framebuffer address */
  unsigned int cursfxn;		/* various cursor functions */
#define ZXX_CADDR_MASKDATA(row) (0x00+(row))
#define ZXX_CADDR_COLDATA(row)  (0x20+(row))
#define ZXX_CADDR_ACTCOORD 0x40
/* 0x41..0x4f documented as reserved */
#define ZXX_CADDR_COLOUR_0 0x50
#define ZXX_CADDR_COLOUR_1 0x51
/* 0x52..0x7f documented as reserved */
#define ZXX_CADDR_TRAP_0 0x80
#define ZXX_CADDR_TRAP_1 0x81
/* 0x82..0x8f documented as reserved */
/* 0x90..0xff not documented */
  unsigned int hole2[1019];
  unsigned int vframecnt;	/* Video Frame Counter */
  } ;

#endif
