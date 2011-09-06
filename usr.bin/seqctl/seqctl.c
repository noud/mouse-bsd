/* This file is in the public domain. */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/midiio.h>

extern const char *__progname;

static const char *seqdev = "/dev/sequencer";

static void usage(void) __attribute__((__noreturn__));
static void usage(void)
{
 fprintf(stderr,"Usage:\n");
 fprintf(stderr,"    %s [-d <sequencer-device>] operation [args ...]\n",__progname);
 fprintf(stderr,"operation   args\n");
 fprintf(stderr,"  -midi     -q        (query underlying MIDI unit list)\n");
 fprintf(stderr,"  -midi     -d        (set to default, use all available)\n");
 fprintf(stderr,"  -midi     n n ...   (set underlying MIDI unit list)\n");
 fprintf(stderr,"  -debug              (query `sequencerdebug' kernel value)\n");
 fprintf(stderr,"  -debug    n         (set `sequencerdebug' kernel value)\n");
 exit(1);
}

static void Ioctl_(int fd, unsigned long int cmd, void *arg, const char *cmdstr)
{
 if (ioctl(fd,cmd,arg) < 0)
  { fprintf(stderr,"%s: %s %s: %s\n",__progname,seqdev,cmdstr,strerror(errno));
    exit(1);
  }
}
#define Ioctl(f,c,a) Ioctl_(f,c,a,#c)

static int open_sequencer(void)
{
 int fd;

 fd = open(seqdev,O_RDWR,0);
 if (fd < 0)
  { fprintf(stderr,"%s: %s: %s\n",__progname,seqdev,strerror(errno));
    exit(1);
  }
 return(fd);
}

static void do_midi_query(void)
{
 int fd;
 struct seq_midicsg csg;
 struct seq_midiconf conf;
 int n;
 int i;

 fd = open_sequencer();
 conf.nunits = 0;
 conf.units = 0;
 csg.old = &conf;
 csg.new = 0;
 Ioctl(fd,SEQUENCER_MIDICONF,&csg);
 if (conf.nunits > 0)
  { conf.units = 0;
    do
     { free(conf.units);
       n = conf.nunits;
       conf.units = malloc(conf.nunits*sizeof(int));
       Ioctl(fd,SEQUENCER_MIDICONF,&csg);
     } while (conf.nunits > n);
  }
 if (conf.nunits < 0)
  { printf("%s: default (currently ",seqdev);
    if (conf.maxunits)
     { printf("0..%d)\n",conf.maxunits-1);
     }
    else
     { printf("no units configured)\n");
     }
  }
 else
  { printf("%s: %d:",seqdev,conf.nunits);
    for (i=0;i<conf.nunits;i++) printf(" %d",conf.units[i]);
    if (conf.maxunits)
     { printf(" (available: 0..%d)\n",conf.maxunits-1);
     }
    else
     { printf(" (available: none)\n");
     }
  }
}

static void do_midi_set(int n, int *v)
{
 int fd;
 struct seq_midicsg csg;
 struct seq_midiconf conf;

 fd = open_sequencer();
 csg.old = 0;
 csg.new = &conf;
 conf.nunits = n;
 conf.units = v;
 Ioctl(fd,SEQUENCER_MIDICONF,&csg);
}

static void do_midi(int nargs, char **args)
{
 if ((nargs == 1) && !strcmp(args[0],"-q"))
  { do_midi_query();
  }
 else if ((nargs == 1) && !strcmp(args[0],"-d"))
  { do_midi_set(-1,0);
  }
 else
  { int *vec;
    long int val;
    char *ep;
    int i;
    vec = nargs ? malloc(nargs*sizeof(int)) : 0;
    for (i=0;i<nargs;i++)
     { val = strtol(args[i],&ep,0);
       if ((ep == args[i]) || *ep)
	{ fprintf(stderr,"%s: %s: not a number\n",__progname,args[i]);
	  usage();
	}
       vec[i] = val;
       if (vec[i] != val)
	{ fprintf(stderr,"%s: %ld: out of range\n",__progname,val);
	  usage();
	}
     }
    do_midi_set(nargs,vec);
  }
}

static void do_debug_query(void)
{
 int fd;
 int dbg;

 fd = open_sequencer();
 dbg = -1;
 Ioctl(fd,SEQUENCER_DEBUG,&dbg);
 printf("%d\n",dbg);
}

static void do_debug_set(const char *s)
{
 int fd;
 int dbg;
 long int v;
 char *ep;

 v = strtol(s,&ep,0);
 if (*ep || (ep == s))
  { fprintf(stderr,"%s: %s: not a number\n",__progname,s);
    exit(1);
  }
 dbg = v;
 if ((dbg != v) || (dbg < 0))
  { fprintf(stderr,"%s: %s: out of range\n",__progname,s);
    exit(1);
  }
 fd = open_sequencer();
 Ioctl(fd,SEQUENCER_MIDICONF,&dbg);
}

static void do_debug(int nargs, char **args)
{
 if (nargs == 0)
  { do_debug_query();
  }
 else if (nargs == 1)
  { do_debug_set(args[0]);
  }
 else
  { usage();
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 if (!strcmp(av[1],"-d") && av[2])
  { seqdev = av[2];
    ac -= 2;
    av += 2;
  }
 if (!strcmp(av[1],"-midi"))
  { do_midi(ac-2,av+2);
  }
 else if (!strcmp(av[1],"-debug"))
  { do_debug(ac-2,av+2);
  }
 else
  { usage();
  }
 exit(0);
}
