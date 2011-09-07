/*
 * ptape daemon for pseudo-copytape files.  Such files consist of a
 *  fixed-size header followed by a stream of tape records.  Each
 *  record is both prefixed and suffixed with its length (the suffix so
 *  that backward seeks are possible).  The header contains:
 *
 *	Format version number (one byte)
 *		Currently always 0x01
 *	Endianness indicator (one byte)
 *		This is 0x42 (ASCII B) or 0x6c (ASCII l), according as
 *		the multi-byte quantities in the file are stored
 *		big-endian or little-endian.  Other values for this
 *		byte are file format errors.
 *
 * Following the header are tape records.  A tape mark is represented
 *  as a tape record of length zero; end-of-media is represented by
 *  end-of-file.  End-of-file occurring other than between tape records
 *  is a file format error.
 *
 * Each tape record consists of the tape record length, the tape record
 *  data, and the tape record length.
 *
 * Tape record lengths are always stored in binary as three bytes, in
 *  the endianness indicated by the endianness indicator byte in the
 *  header.  There are no shims between records, for alignment or any
 *  other reason, and the format cannot represent tape records over
 *  16777215 bytes long.
 *
 * An empty file represents a new tape; the first time such a "tape" is
 *  opened, it will acquire a header.
 *
 * This file is in the public domain.
 */

#define FORMAT_VER 0x01
#define ENDIAN_MSBFIRST 0x42
#define ENDIAN_LSBFIRST 0x6c

#define MAXRECLEN 0x00ffffff

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mtio.h>

extern const char *__progname;

static int verbose = 0;
static int force_ro = 0;
static int background = 0;
static const char *dev_name;
static const char *file_name;

static unsigned char endian;
static unsigned char our_endian;
static int devfd;
static int filefd;
static off_t filepos;
static int lastwrite;
static int rdonly;
/* We'd like to use IOCPARM_MAX, but on some ports that's defined in terms of
   NBPG and hence isn't a compile-time constant.  So we use IOCPARM_MASK. */
static char recbuf[
#if IOCPARM_MASK > MAXRECLEN
		   IOCPARM_MASK
#else
		   MAXRECLEN
#endif
			       ];
static int reclen;
static int recpos;
static int exiting;

static void handleargs(int ac, char **av)
{
 int argno;
 int skip;
 int errs;

 argno = 0;
 skip = 0;
 errs = 0;
 for (ac--,av++;ac;ac--,av++)
  { if (**av != '-')
     { switch (argno++)
	{ case 0:
	     dev_name = *av;
	     break;
	  case 1:
	     file_name = *av;
	     break;
	  default:
	     fprintf(stderr,"%s: stray argument `%s'\n",__progname,*av);
	     errs ++;
	     break;
	}
     }
    else
     { if (!strcmp(*av,"-v"))
	{ verbose ++;
	}
       else if (!strcmp(*av,"-ro"))
	{ force_ro ++;
	}
       else if (!strcmp(*av,"-bg"))
	{ background ++;
	}
       else
	{ fprintf(stderr,"%s: unrecognized flag `%s'\n",__progname,*av);
	  errs ++;
	}
     }
  }
 if (!dev_name || !file_name)
  { fprintf(stderr,"%s: must supply master device and backing file names\n",__progname);
    errs ++;
  }
 if (errs)
  { fprintf(stderr,"Usage: %s [-v] master-device file\n",__progname);
    exit(1);
  }
}

static void opendev(void)
{
 devfd = open(dev_name,O_RDWR,0);
 if (devfd < 0)
  { fprintf(stderr,"%s: %s: %s\n",__progname,dev_name,strerror(errno));
    exit(1);
  }
 if (verbose) printf("Opened %s\n",dev_name);
}

static void initfile(void)
{
 unsigned char fmtver;
 int rv;

 rdonly = 0;
 if ( force_ro ||
      ( ((filefd=open(file_name,O_RDWR,0)) < 0) &&
	(errno == EACCES) ) )
  { filefd = open(file_name,O_RDONLY,0);
    rdonly = 1;
  }
 if (filefd < 0)
  { fprintf(stderr,"%s: can't open %s: %s\n",__progname,file_name,strerror(errno));
    exit(1);
  }
 if (verbose) printf("Opened %s %s\n",file_name,rdonly?"read-only":"read/write");
 rv = pread(filefd,&fmtver,1,0);
 if (rv < 0)
  { fprintf(stderr,"%s: %s: header read: %s\n",__progname,file_name,strerror(errno));
    exit(1);
  }
 if (rv == 0)
  { if (rdonly)
     { fprintf(stderr,"%s: %s: read-only file with no header\n",__progname,file_name);
       exit(1);
     }
    if (verbose) printf("Empty file, writing a header\n");
    fmtver = FORMAT_VER;
    endian = our_endian;
    rv = pwrite(filefd,&fmtver,1,0);
    if (rv == 1) rv = pwrite(filefd,&endian,1,1);
    if (rv < 0)
     { fprintf(stderr,"%s: %s: can't write initial header: %s\n",__progname,file_name,strerror(errno));
       exit(1);
     }
    if (rv != 1)
     { fprintf(stderr,"%s: %s: can't write initial header (wanted 1, wrote %d)\n",__progname,file_name,rv);
       exit(1);
     }
  }
 if (fmtver != FORMAT_VER)
  { fprintf(stderr,"%s: %s: format version %d, not expected %d\n",__progname,file_name,fmtver,FORMAT_VER);
    exit(1);
  }
 rv = pread(filefd,&endian,1,1);
 if (rv < 0)
  { fprintf(stderr,"%s: %s: can't read header: %s\n",__progname,file_name,strerror(errno));
    exit(1);
  }
 if (rv == 0)
  { fprintf(stderr,"%s: %s: can't read header (unexpected EOF)\n",__progname,file_name);
    exit(1);
  }
 if (rv != 1)
  { fprintf(stderr,"%s: %s: can't read header (wanted 1, read %d)\n",__progname,file_name,rv);
    exit(1);
  }
 switch (endian)
  { case ENDIAN_LSBFIRST:
    case ENDIAN_MSBFIRST:
       if (verbose) printf("File header: version=%d endian=%s\n",fmtver,(endian==ENDIAN_LSBFIRST)?"LSBFIRST":"MSBFIRST");
       break;
    default:
       fprintf(stderr,"%s: %s: invalid endianness flag 0x%02x in header\n",__progname,file_name,endian);
       exit(1);
       break;
  }
 filepos = 2;
 if (verbose) printf("filepos = 2\n");
}

static void check_endian(void)
{
 int v;

 v = 1;
 our_endian = (*(char *)&v == 1) ? ENDIAN_LSBFIRST : ENDIAN_MSBFIRST;
 if (verbose)
  { switch (our_endian)
     { case ENDIAN_LSBFIRST: printf("Our endianness is LSBFIRST\n"); break;
       case ENDIAN_MSBFIRST: printf("Our endianness is MSBFIRST\n"); break;
       default: abort(); break;
     }
  }
}

static void devprotoerr(const char *, ...)
	__attribute__((__format__(__printf__,1,2),__noreturn__));
static void devprotoerr(const char *fmt, ...)
{
 va_list ap;

 fprintf(stderr,"%s: device protocol error: ",__progname);
 va_start(ap,fmt);
 vfprintf(stderr,fmt,ap);
 va_end(ap);
 fprintf(stderr,"\n");
 exit(1);
}

static void devget(void *buf, int nb, const char *what)
{
 int n;
 char *bp;
 int left;

 fflush(stdout);
 left = nb;
 bp = buf;
 while (left > 0)
  { n = read(devfd,bp,left);
    if (n < 0)
     { fprintf(stderr,"%s: %s: read %s: %s\n",__progname,dev_name,what,strerror(errno));
       exit(1);
     }
    if (n == 0)
     { fprintf(stderr,"%s: %s: read %s: EOF\n",__progname,dev_name,what);
       exit(1);
     }
    bp += n;
    left -= n;
  }
 if (verbose) printf("devgot %d\n",nb);
}

static void dev_int(int v)
{
 write(devfd,&v,sizeof(v));
}

static void cmd_open(void)
{
 int err;
 char rwflag;

 if (verbose) printf("cmd_open\n");
 devget(&rwflag,1,"open rwflag");
 if (verbose) printf("rwflag = %c\n",rwflag);
 err = 0;
 if (rdonly && (rwflag != 'r'))
  { err = EROFS;
    if (verbose) printf("rw open, ro filesystem -> EROFS\n");
  }
 if (verbose) printf("returning err %d\n",err);
 dev_int(err);
}

static void truncat(off_t at)
{
 if (verbose) printf("truncating at %lu\n",(unsigned long int)at);
 ftruncate(filefd,at);
}

static void showfilepos(void)
{
 if (verbose) printf("filepos = %lu\n",(unsigned long int)filepos);
}

static void panic(const char *, ...)
	__attribute__((__format__(__printf__,1,2),__noreturn__));
static void panic(const char *fmt, ...)
{
 va_list ap;

 fprintf(stderr,"%s: panic: ",__progname);
 va_start(ap,fmt);
 vfprintf(stderr,fmt,ap);
 va_end(ap);
 fprintf(stderr,"\n");
 abort();
}

static unsigned int endian_3toi(const void *buf)
{
 switch (endian)
  { case ENDIAN_MSBFIRST:
       return( (((const unsigned char *)buf)[0] << 16) |
	       (((const unsigned char *)buf)[1] <<  8) |
	       (((const unsigned char *)buf)[2]      ) );
       break;
    case ENDIAN_LSBFIRST:
       return( (((const unsigned char *)buf)[0]      ) |
	       (((const unsigned char *)buf)[1] <<  8) |
	       (((const unsigned char *)buf)[2] << 16) );
       break;
  }
 panic("endian_3toi (%d)",endian);
}

static void endian_ito3(unsigned int v, void *buf)
{
 switch (endian)
  { case ENDIAN_MSBFIRST:
       ((unsigned char *)buf)[0] = (v >> 16)       ;
       ((unsigned char *)buf)[1] = (v >>  8) & 0xff;
       ((unsigned char *)buf)[2] = (v      ) & 0xff;
       return;
       break;
    case ENDIAN_LSBFIRST:
       ((unsigned char *)buf)[0] = (v      ) & 0xff;
       ((unsigned char *)buf)[1] = (v >>  8) & 0xff;
       ((unsigned char *)buf)[2] = (v >> 16)       ;
       return;
       break;
  }
 panic("endian_ito3 (%d)",endian);
}

static void dolastwrite(void)
{
 if (lastwrite)
  { char lenbuf[3];
    if (verbose) printf("dolastwrite: writing EOF marker\n");
    endian_ito3(0,&lenbuf[0]);
    pwrite(filefd,&lenbuf[0],3,filepos);
    pwrite(filefd,&lenbuf[0],3,filepos+3);
    pwrite(filefd,&lenbuf[0],3,filepos+6);
    pwrite(filefd,&lenbuf[0],3,filepos+9);
    filepos += 6;
    truncat(filepos+6);
    showfilepos();
    lastwrite = 0;
  }
}

static void cmd_close(void)
{
 if (verbose) printf("cmd_close\n");
 dolastwrite();
}

static void readrecord(void)
{
 char lenbuf[3];
 char lenbuf2[3];
 int len;
 int n;

 n = pread(filefd,&lenbuf[0],3,filepos);
 if (n == 0)
  { reclen = 0;
    if (verbose) printf("readrecord at eof\n");
    return;
  }
 if (n != 3)
  { reclen = -EIO;
    if (verbose) printf("readrecord partial length\n");
    return;
  }
 len = endian_3toi(&lenbuf[0]);
 if (verbose) printf("readrecord reclen = %d\n",len);
 if (len > 0)
  { n = pread(filefd,&recbuf[0],len,filepos+3);
    if (n != len)
     { reclen = -EIO;
       if (verbose) printf("readrecord partial record\n");
       return;
     }
  }
 n = pread(filefd,&lenbuf2[0],3,filepos+3+len);
 if (n != 3)
  { if (verbose) printf("readrecord partial trailing length\n");
    reclen = -EIO;
    return;
  }
 if ( (lenbuf[0] != lenbuf2[0]) ||
      (lenbuf[1] != lenbuf2[1]) ||
      (lenbuf[2] != lenbuf2[2]) )
  { if (verbose) printf("readrecord lengths don't match\n");
    reclen = -EIO;
    return;
  }
 reclen = len;
 if (verbose) printf("readrecord success, reclen = %d\n",reclen);
 filepos += 3 + len + 3;
 showfilepos();
 recpos = 0;
}

static void cmd_read(void)
{
 unsigned char subcmd;

 if (verbose) printf("cmd_read\n");
 devget(&subcmd,1,"read subcommand");
 if (verbose) printf("subcommand %c\n",subcmd);
 switch (subcmd)
  { case 'o':
       readrecord();
       if (verbose) printf("returning %d\n",reclen);
       dev_int(reclen);
       lastwrite = 0;
       break;
    case 'r':
	{ int v;
	  devget(&v,sizeof(v),"read data length");
	  if (recpos+v > reclen) devprotoerr("read overran record");
	  if (verbose) printf("returning data, %d@%d\n",v,recpos);
	  write(devfd,&recbuf[recpos],v);
	  recpos += v;
	}
       break;
    case 'c':
       break;
    default:
       devprotoerr("invalid read subcommand 0x%02x",subcmd);
       break;
  }
}

static void cmd_write(void)
{
 unsigned char subcmd;

 if (verbose) printf("cmd_write\n");
 devget(&subcmd,1,"write subcommand");
 if (verbose) printf("subcommand %c\n",subcmd);
 switch (subcmd)
  { case 'o':
       devget(&reclen,sizeof(int),"write record length");
       if (rdonly)
	{ if (verbose) printf("write to read-only tape -> EROFS\n");
	  if (verbose) printf("returning %d\n",-EROFS);
	  dev_int(-EROFS);
	  return;
	}
       if (reclen > MAXRECLEN) reclen = MAXRECLEN;
       if (verbose) printf("returning %d\n",reclen);
       dev_int(reclen);
       recpos = 0;
       break;
    case 'w':
	{ int v;
	  devget(&v,sizeof(v),"write data length");
	  if (recpos+v > reclen) devprotoerr("write overran record");
	  if (verbose) printf("accepting data, %d@%d\n",v,recpos);
	  devget(&recbuf[recpos],v,"write data");
	  recpos += v;
	}
       break;
    case 'c':
       if (recpos != reclen) devprotoerr("write missing data");
	{ char lenbuf[3];
	  endian_ito3(reclen,&lenbuf[0]);
	  if (verbose) printf("writing record, len = %d\n",reclen);
	  if ( (pwrite(filefd,&lenbuf[0],3,filepos) != 3) ||
	       (pwrite(filefd,&recbuf[0],reclen,filepos+3) != reclen) ||
	       (pwrite(filefd,&lenbuf[0],3,filepos+3+reclen) != 3) )
	   { if (verbose) printf("write failed\n");
	     if (verbose) printf("returning %d\n",EIO);
	     dev_int(EIO);
	   }
	  else
	   { if (verbose) printf("returning 0\n");
	     dev_int(0);
	     filepos += 3 + reclen + 3;
	     showfilepos();
	   }
	  truncat(filepos);
	  lastwrite = 1;
	}
       break;
    default:
       devprotoerr("invalid write subcommand 0x%02x",subcmd);
       break;
  }
}

static int do_fs(int count, int (*fn)(int))
{
 int n;
 int len;
 char lenbuf[3];
 char lenbuf2[3];

 if (verbose) printf("do_fs\n");
 while (1)
  { if (count < 1)
     { if (verbose) printf("count expired -> success\n");
       break;
     }
    n = pread(filefd,&lenbuf[0],3,filepos);
    if (n == 0)
     { if (verbose) printf("at EOF -> EIO\n");
       return(EIO);
     }
    if (n != 3)
     { if (verbose) printf("partial length -> EIO\n");
       return(EIO);
     }
    len = endian_3toi(&lenbuf[0]);
    n = pread(filefd,&lenbuf2[0],3,filepos+3+len);
    if (n != 3)
     { if (verbose) printf("partial trailing length -> EIO\n");
       return(EIO);
     }
    if ( (lenbuf[0] != lenbuf2[0]) ||
	 (lenbuf[1] != lenbuf2[1]) ||
	 (lenbuf[2] != lenbuf2[2]) )
     { if (verbose) printf("length mismatch -> EIO\n");
       return(EIO);
     }
    filepos += 3 + len + 3;
    if ((*fn)(len)) count --;
  }
 showfilepos();
 return(0);
}

static int do_bs(int count, int (*fn)(int))
{
 int n;
 int len;
 char lenbuf[3];
 char lenbuf2[3];

 if (verbose) printf("do_bs\n");
 while (1)
  { if (count < 1)
     { if (verbose) printf("count expired -> success\n");
       break;
     }
    if (filepos == 2)
     { if (verbose) printf("at BOT -> EIO\n");
       return(EIO);
     }
    if (filepos < 8)
     { if (verbose) printf("too close to BOT -> EIO\n");
       return(EIO);
     }
    n = pread(filefd,&lenbuf[0],3,filepos-3);
    if (n != 3)
     { if (verbose) printf("partial length -> EIO\n");
       return(EIO);
     }
    len = endian_3toi(&lenbuf[0]);
    n = pread(filefd,&lenbuf2[0],3,filepos-3-len-3);
    if (n != 3)
     { if (verbose) printf("partial leading length -> EIO\n");
       return(EIO);
     }
    if ( (lenbuf[0] != lenbuf2[0]) ||
	 (lenbuf[1] != lenbuf2[1]) ||
	 (lenbuf[2] != lenbuf2[2]) )
     { if (verbose) printf("length mismatch -> EIO\n");
       return(EIO);
     }
    filepos -= 3 + len + 3;
    if ((*fn)(len)) count --;
  }
 showfilepos();
 return(0);
}

static int do_eom(void)
{
 int n;
 int len;
 char lenbuf[3];
 char lenbuf2[3];

 if (verbose) printf("do_eom\n");
 while (1)
  { n = pread(filefd,&lenbuf[0],3,filepos);
    if (n == 0)
     { if (verbose) printf("at EOF -> done\n");
       break;
     }
    if (n != 3)
     { if (verbose) printf("partial length -> EIO\n");
       return(EIO);
     }
    len = endian_3toi(&lenbuf[0]);
    n = pread(filefd,&lenbuf2[0],3,filepos+3+len);
    if (n != 3)
     { if (verbose) printf("partial trailing length -> EIO\n");
       return(EIO);
     }
    if ( (lenbuf[0] != lenbuf2[0]) ||
	 (lenbuf[1] != lenbuf2[1]) ||
	 (lenbuf[2] != lenbuf2[2]) )
     { if (verbose) printf("length mismatch -> EIO\n");
       return(EIO);
     }
    filepos += 3 + len + 3;
  }
 showfilepos();
 return(0);
}

static int skip_file(int len)
{
 return(len==0);
}

static int skip_record(int len __attribute__((__unused__)))
{
 return(1);
}

static int do_ioctl(unsigned long int cmd)
{
 switch (cmd)
  { case MTIOCGET:
	{ struct mtget sts;
	  int i;
	  if (reclen != sizeof(sts))
	   { if (verbose) printf("MTIOCGET: length wrong\n");
	     return(EIO);
	   }
	  sts.mt_type = MT_ISAR; /* closest to a "generic" there is */
	  sts.mt_dsreg = 0;
	  sts.mt_erreg = 0;
	  sts.mt_resid = 0;
	  sts.mt_fileno = 0;
	  sts.mt_blkno = 0;
	  sts.mt_blksiz = 0;
	  sts.mt_density = 0;
	  for (i=0;i<4;i++)
	   { sts.mt_mblksiz[i] = 0;
	     sts.mt_mdensity[i] = 0;
	   }
	  bcopy(&sts,&recbuf[0],sizeof(sts));
	  reclen = sizeof(sts);
	  if (verbose) printf("MTIOCGET: success\n");
	  return(0);
	}
       break;
    case MTIOCTOP:
	{ struct mtop op;
	  daddr_t c;
	  if (reclen != sizeof(op)) return(EIO);
	  bcopy(&recbuf[0],&op,sizeof(op));
	  if (verbose) printf("MTIOCTOP: op=%d count=%d\n",(int)op.mt_op,(int)op.mt_count);
	  switch (op.mt_op)
	   { case MTWEOF:
		 { char lenbuf[3];
		   if (verbose) printf("MTIOCTOP: WEOF\n");
		   if (rdonly)
		    { if (verbose) printf("MTIOCTOP: WEOF read-only -> EROFS\n");
		      return(EROFS);
		    }
		   endian_ito3(0,&lenbuf[0]);
		   for (c=op.mt_count;c>0;c--)
		    { if ( (pwrite(filefd,&lenbuf[0],3,filepos) != 3) ||
			   (pwrite(filefd,&lenbuf[0],3,filepos+3) != 3) )
		       { if (verbose) printf("MTIOCTOP: WEOF write failed -> EIO\n");
			 return(EIO);
		       }
		      filepos += 6;
		    }
		   if (op.mt_count > 0)
		    { showfilepos();
		      truncat(filepos);
		      lastwrite = 0;
		    }
		 }
		break;
	     case MTFSF:
		if (verbose) printf("MTIOCTOP: FSF\n");
		dolastwrite();
		return(do_fs(op.mt_count,skip_file));
		break;
	     case MTBSF:
		if (verbose) printf("MTIOCTOP: BSF\n");
		dolastwrite();
		return(do_bs(op.mt_count,skip_file));
		break;
	     case MTFSR:
		if (verbose) printf("MTIOCTOP: FSR\n");
		dolastwrite();
		return(do_fs(op.mt_count,skip_record));
		break;
	     case MTBSR:
		if (verbose) printf("MTIOCTOP: BSR\n");
		dolastwrite();
		return(do_bs(op.mt_count,skip_record));
		break;
	     case MTREW:
		if (verbose) printf("MTIOCTOP: REW\n");
		dolastwrite();
		filepos = 2;
		return(0);
		break;
	     case MTOFFL:
		if (verbose) printf("MTIOCTOP: OFFL\n");
		dolastwrite();
		exiting = 1;
		return(0);
		break;
	     case MTNOP:
	     case MTRETEN:
		if (verbose) printf("MTIOCTOP: NOP/RETEN\n");
		return(0);
		break;
	     case MTERASE:
		if (verbose) printf("MTIOCTOP: ERASE\n");
		if (rdonly)
		 { if (verbose) printf("MTIOCTOP: ERASE read-only -> EROFS\n");
		   return(EROFS);
		 }
		filepos = 2;
		ftruncate(filefd,2);
		lastwrite = 0;
		if (verbose) printf("MTIOCTOP: ERASE succeeded\n");
		return(0);
		break;
	     case MTEOM:
		if (verbose) printf("MTIOCTOP: EOM\n");
		dolastwrite();
		lastwrite = 0;
		return(do_eom());
		break;
	     default:
		if (verbose) printf("MTIOCTOP: unknown -> EIO\n");
		return(EIO);
		break;
	   }
	}
       break;
  }
 if (verbose) printf("do_ioctl: unknown -> ENOTTY\n");
 return(ENOTTY);
}

static void cmd_ioctl(void)
{
 unsigned char subcmd;
 int rv;

 if (verbose) printf("cmd_ioctl\n");
 devget(&subcmd,1,"ioctl subcommand");
 if (verbose) printf("subcommand %c\n",subcmd);
 switch (subcmd)
  { case 'o':
	{ unsigned long int cmd;
	  devget(&cmd,sizeof(cmd),"ioctl operation");
	  if (verbose) printf("cmd = %lx\n",cmd);
	  reclen = IOCPARM_LEN(cmd);
	  if (verbose) printf("data len = %d\n",reclen);
	  if (reclen > IOCPARM_MASK) devprotoerr("impossible ioctl IOCPARM_LEN");
	  if (cmd & IOC_IN) devget(&recbuf[0],reclen,"ioctl data");
	  rv = do_ioctl(cmd);
	  if (verbose) printf("returning %d\n",rv);
	  dev_int(rv);
	  recpos = 0;
	}
       break;
    case 'd':
	{ int v;
	  devget(&v,sizeof(v),"ioctl data length");
	  if (recpos+v > reclen) devprotoerr("ioctl data overrun");
	  write(devfd,&recbuf[recpos],v);
	  if (verbose) printf("sent ioctl data, %d@%d\n",v,recpos);
	  recpos += v;
	}
       break;
  }
}

static void doreq(void)
{
 unsigned char opc;

 devget(&opc,1,"opcode");
 switch (opc)
  { case 'o': cmd_open();  break;
    case 'c': cmd_close(); break;
    case 'r': cmd_read();  break;
    case 'w': cmd_write(); break;
    case 'i': cmd_ioctl(); break;
    default:
       devprotoerr("invalid request opcode 0x%02x",opc);
       break;
  }
}

static void fork_and_exit(void)
{
 pid_t kid;
 int dev_null;

 dev_null = open("/dev/null",O_RDWR,0);
 if (dev_null < 0)
  { fprintf(stderr,"%s: can't open /dev/null: %s\n",__progname,strerror(errno));
    exit(1);
  }
 kid = fork();
 if (kid < 0)
  { fprintf(stderr,"%s: fork: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (kid > 0) exit(0);
 if (dev_null != 0)
  { dup2(dev_null,0);
    close(dev_null);
  }
 dup2(0,1);
 dup2(0,2);
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 check_endian();
 opendev();
 initfile();
 if (background) fork_and_exit();
 while (! exiting) doreq();
 exit(0);
}
