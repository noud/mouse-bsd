/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <dev/pseudo/if_vlan.h>

extern const char *__progname;

#define ACT_ERROR 1 /* none of the below */
#define ACT_QUERY 2 /* vlanX */
#define ACT_SET   3 /* vlanX iface vlan */
#define ACT_DOWN  4 /* vlanX down */
static int action = ACT_ERROR;

static char *txt_dev;
static char *txt_iface;
static char *txt_vlan;

static int devfd;

static void handleargs(int ac, char **av)
{
 txt_dev = av[1];
 if (ac == 2)
  { action = ACT_QUERY;
  }
 else if ((ac == 3) && !strcmp(av[2],"down"))
  { action = ACT_DOWN;
  }
 else if (ac == 4)
  { action = ACT_SET;
    txt_iface = av[2];
    txt_vlan = av[3];
  }
 if (action == ACT_ERROR)
  { fprintf(stderr,"Usage: %s vlanX\n",__progname);
    fprintf(stderr,"       %s vlanX <iface> <vlan>\n",__progname);
    fprintf(stderr,"       %s vlanX down\n",__progname);
    exit(1);
  }
}

static void open_dev(int how)
{
 if (! index(txt_dev,'/'))
  { char *tmp;
    asprintf(&tmp,"/dev/%s",txt_dev);
    txt_dev = tmp;
  }
 devfd = open(txt_dev,how,0);
 if (devfd < 0)
  { fprintf(stderr,"%s; can't open %s: %s\n",__progname,txt_dev,strerror(errno));
    exit(1);
  }
}

static void do_query(void)
{
 struct vlan_config cnf;

 open_dev(O_RDONLY);
 if (ioctl(devfd,VLANIOC_GCONF,&cnf) < 0)
  { fprintf(stderr,"%s: can't get config: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (cnf.u.dstifn[0])
  { printf("%.*s ",IFNAMSIZ,&cnf.u.dstifn[0]);
    switch (cnf.tag)
     { case VLAN_NONE:  printf("none");       break;
       case VLAN_OTHER: printf("other");      break;
       default:         printf("%d",cnf.tag); break;
     }
    printf("\n");
  }
 else
  { printf("down\n");
  }
}

static void do_set(void)
{
 struct vlan_config cnf;

 open_dev(O_RDWR);
 if (!strcmp(txt_vlan,"none"))
  { cnf.tag = VLAN_NONE;
  }
 else if (!strcmp(txt_vlan,"other"))
  { cnf.tag = VLAN_OTHER;
  }
 else
  { cnf.tag = atoi(txt_vlan);
  }
 strncpy(&cnf.u.dstifn[0],txt_iface,IFNAMSIZ);
 if (ioctl(devfd,VLANIOC_SCONF,&cnf) < 0)
  { fprintf(stderr,"%s: can't set config: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_down(void)
{
 struct vlan_config cnf;

 open_dev(O_RDWR);
 cnf.tag = VLAN_OTHER;
 strncpy(&cnf.u.dstifn[0],"",IFNAMSIZ);
 if (ioctl(devfd,VLANIOC_SCONF,&cnf) < 0)
  { fprintf(stderr,"%s: can't set config: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 switch (action)
  { case ACT_QUERY:
       do_query();
       break;
    case ACT_SET:
       do_set();
       break;
    case ACT_DOWN:
       do_down();
       break;
    default:
       abort();
       break;
  }
 exit(0);
}
