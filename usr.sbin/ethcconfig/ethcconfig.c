/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <dev/pseudo/if_ethc.h>

extern const char *__progname;

static int devfd;

static void open_dev(const char *path, int how)
{
 if (! index(path,'/'))
  { char *tmp;
    asprintf(&tmp,"/dev/%s",path);
    path = tmp;
  }
 devfd = open(path,how,0);
 if (devfd < 0)
  { fprintf(stderr,"%s; can't open %s: %s\n",__progname,path,strerror(errno));
    exit(1);
  }
}

static void do_query(const char *dev)
{
 struct ethc_config conf;
 int i;

 open_dev(dev,O_RDONLY);
 conf.nifs = 0;
 if (ioctl(devfd,ETHCIOC_GCONF,&conf) < 0)
  { fprintf(stderr,"%s: can't get count: %s\n",__progname,strerror(errno));
    exit(1);
  }
 while (1)
  { int nnames;
    nnames = conf.nifs;
    conf.ifnames = malloc(nnames*sizeof(*conf.ifnames));
    if (ioctl(devfd,ETHCIOC_GCONF,&conf) < 0)
     { fprintf(stderr,"%s: can't get config: %s\n",__progname,strerror(errno));
       exit(1);
     }
    if (nnames >= conf.nifs) break;
    free(conf.ifnames);
  }
 for (i=0;i<conf.nifs;i++) printf("%.*s\n",(int)IFNAMSIZ,&conf.ifnames[i][0]);
 free(conf.ifnames);
}

static void do_set(const char *dev, int nifs, /*const*/ char **ifs)
{
 struct ethc_config conf;
 int i;

 open_dev(dev,O_RDWR);
 conf.nifs = nifs;
 conf.ifnames = malloc(nifs*sizeof(*conf.ifnames));
 for (i=0;i<nifs;i++)
  { if (strlen(ifs[i]) > IFNAMSIZ)
     { fprintf(stderr,"%s: %s: interface name too long\n",__progname,ifs[i]);
       exit(1);
     }
    strncpy(&conf.ifnames[i][0],ifs[i],IFNAMSIZ);
  }
 if (ioctl(devfd,ETHCIOC_SCONF,&conf) < 0)
  { fprintf(stderr,"%s: can't set config: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 if ((ac == 3) && !strcmp(av[2],"-q"))
  { do_query(av[1]);
  }
 else if ((ac >= 3) && !strcmp(av[2],"-set"))
  { do_set(av[1],ac-3,av+3);
  }
 else
  { fprintf(stderr,"Usage: %s ethX -q\n",__progname);
    fprintf(stderr,"       %s ethX -set [if1 [if2 ...]]\n",__progname);
    exit(1);
  }
 exit(0);
}
