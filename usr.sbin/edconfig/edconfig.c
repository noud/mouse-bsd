/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <dev/pseudo/ed-intf.h>

extern const char *__progname;

static const char *ctldev = "/dev/edctl";

static int unit;
static int operation;
#define OP_KEY    1
#define OP_PROMPT 2
#define OP_DEV    3
#define OP_RESET  4
static char *arg;

static void usage(void) __attribute__((__noreturn__));
static void usage(void)
{
 fprintf(stderr,"\
Usage: %s -key edN keystring\n\
       %s -prompt edN\n\
       %s -dev edN devname\n\
       %s -reset edN\n\
",__progname,__progname,__progname,__progname);
 exit(1);
}

static void handleargs(int ac, char **av)
{
 char *np;
 char *ep;
 long int v;
 int nargs;

 if (ac < 3)
  { fprintf(stderr,"%s: too few arguments\n",__progname);
    usage();
  }
      if (!strcmp(av[1],"-key"))    { operation = OP_KEY;    nargs = 1; }
 else if (!strcmp(av[1],"-prompt")) { operation = OP_PROMPT; nargs = 0; }
 else if (!strcmp(av[1],"-dev"))    { operation = OP_DEV;    nargs = 1; }
 else if (!strcmp(av[1],"-reset"))  { operation = OP_RESET;  nargs = 0; }
 else usage();
 np = av[2];
 if (!strncmp(np,"ed",2)) np += 2;
 v = strtol(np,&ep,0);
 if ((np == ep) || *ep)
  { fprintf(stderr,"%s: %s: bad unit\n",__progname,av[2]);
    exit(1);
  }
 unit = v;
 if ((unit < 0) || (unit != v))
  { fprintf(stderr,"%s: %s: out-of-range unit number\n",__progname,av[2]);
    exit(1);
  }
 if (ac != nargs+3)
  { fprintf(stderr,"%s: wrong number of arguments for %s\n",__progname,av[1]);
    exit(1);
  }
 arg = av[3];
}

static void docmd(const char *cmd, int len)
{
 int fd;
 int w;

 fd = open(ctldev,O_WRONLY,0);
 if (fd < 0)
  { fprintf(stderr,"%s: %s: %s\n",__progname,ctldev,strerror(errno));
    exit(1);
  }
 w = write(fd,cmd,len);
 if (w < 0)
  { fprintf(stderr,"%s: can't write command: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (w != len)
  { fprintf(stderr,"%s: short command write: wanted %d, did %d\n",__progname,len,w);
    exit(1);
  }
 close(fd);
}

static void do_key(const char *arg)
{
 int l;
 char *tmp;

 l = strlen(arg);
 tmp = malloc(2+l);
 bcopy(arg,tmp+2,l);
 tmp[0] = ED_CMD_SETKEY;
 tmp[1] = unit;
 docmd(tmp,2+l);
}

static void do_prompt(void)
{
 char tmp[2];

 tmp[0] = ED_CMD_SETPROMPT;
 tmp[1] = unit;
 docmd(tmp,2);
}

static void do_dev(const char *arg)
{
 int fd;
 char tmp[2+sizeof(int)];

 fd = open(arg,O_RDWR,0);
 if (fd < 0)
  { fd = open(arg,O_RDONLY,0);
    if (fd < 0)
     { fprintf(stderr,"%s: %s: %s\n",__progname,arg,strerror(errno));
       exit(1);
     }
  }
 tmp[0] = ED_CMD_SETDEV;
 tmp[1] = unit;
 bcopy(&fd,&tmp[2],sizeof(int));
 docmd(tmp,2+sizeof(int));
}

static void do_reset(void)
{
 char tmp[2];

 tmp[0] = ED_CMD_RESET;
 tmp[1] = unit;
 docmd(tmp,2);
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 switch (operation)
  { case OP_KEY:
       do_key(arg);
       break;
    case OP_PROMPT:
       do_prompt();
       break;
    case OP_DEV:
       do_dev(arg);
       break;
    case OP_RESET:
       do_reset();
       break;
  }
 exit(0);
}
