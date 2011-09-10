/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <dev/pseudo/if_encap.h>

extern const char *__progname;

#define ACT_ERROR     1 /* none of the below */
#define ACT_QUERYALL  2 /* encapX */
#define ACT_QUERYONE  3 /* encapX N */
#define ACT_DEL       4 /* encapX del N */
#define ACT_SEND      5 /* encapX send N */
#define ACT_REBOOT    6 /* encapX reboot N */
#define ACT_ADD       7 /* encapX add srcaddr mask src dst id secret */
#define ACT_SET       8 /* encapX set N srcaddr mask src dst id secret */
#define ACT_FLAGS     9 /* encapX flags */
#define ACT_SFLAG    10 /* encapX flags [+|-]flag */
#define ACT_BLOCK    11 /* encapX block [net[/mask] ...] */
#define ACT_POISON   12 /* encapX poison [net[/mask] ...] */
#define ACT_DEBUG    13 /* encapX debug */
static int action = ACT_ERROR;

static char *txt_dev;
static char *txt_n;
static char *txt_flg;
static char *txt_addr;
static char *txt_mask;
static char *txt_src;
static char *txt_dst;
static char *txt_id;
static char *txt_ttl;
static char *txt_secret;
static char **txt_toblock;
static char **txt_topoison;

static int devfd;

static struct {
	 const char *name;
	 unsigned int bit;
	 } flagbits[] = { { "sig", ESF_D_SIG },
			  { "pkt", ESF_D_PKT },
			  { 0 } };

static void handleargs(int ac, char **av)
{
 txt_dev = av[1];
 if (ac == 2)
  { action = ACT_QUERYALL;
  }
 else if ((ac >= 3) && !strcmp(av[2],"block"))
  { action = ACT_BLOCK;
    txt_toblock = av + 3;
  }
 else if ((ac >= 3) && !strcmp(av[2],"poison"))
  { action = ACT_POISON;
    txt_topoison = av + 3;
  }
 else if ((ac == 3) && !strcmp(av[2],"debug"))
  { action = ACT_DEBUG;
  }
 else if ((ac == 3) && !strcmp(av[2],"flags"))
  { action = ACT_FLAGS;
  }
 else if (ac == 3)
  { action = ACT_QUERYONE;
    txt_n = av[2];
  }
 else if ((ac == 4) && !strcmp(av[2],"del"))
  { action = ACT_DEL;
    txt_n = av[3];
  }
 else if ((ac == 4) && !strcmp(av[2],"send"))
  { action = ACT_SEND;
    txt_n = av[3];
  }
 else if ((ac == 4) && !strcmp(av[2],"reboot"))
  { action = ACT_REBOOT;
    txt_n = av[3];
  }
 else if ((ac == 4) && !strcmp(av[2],"flags"))
  { action = ACT_SFLAG;
    txt_flg = av[3];
  }
 else if ((ac == 10) && !strcmp(av[2],"add"))
  { action = ACT_ADD;
    txt_addr = av[3];
    txt_mask = av[4];
    txt_src = av[5];
    txt_dst = av[6];
    txt_id = av[7];
    txt_ttl = av[8];
    txt_secret = av[9];
  }
 else if ((ac == 11) && !strcmp(av[2],"set"))
  { action = ACT_SET;
    txt_n = av[3];
    txt_addr = av[4];
    txt_mask = av[5];
    txt_src = av[6];
    txt_dst = av[7];
    txt_id = av[8];
    txt_ttl = av[9];
    txt_secret = av[10];
  }
 if (action == ACT_ERROR)
  { fprintf(stderr,"Usage: %s encapX\n",__progname);
    fprintf(stderr,"       %s encapX N\n",__progname);
    fprintf(stderr,"       %s encapX del N\n",__progname);
    fprintf(stderr,"       %s encapX send N\n",__progname);
    fprintf(stderr,"       %s encapX reboot N\n",__progname);
    fprintf(stderr,"       %s encapX add addr mask src dst id ttl secret\n",__progname);
    fprintf(stderr,"       %s encapX set N addr mask src dst id ttl secret\n",__progname);
    fprintf(stderr,"       %s encapX flags {[+|-]flag}\n",__progname);
    fprintf(stderr,"       %s encapX block [net[/mask] ...]\n",__progname);
    fprintf(stderr,"       %s encapX poison [net[/mask] ...]\n",__progname);
    fprintf(stderr,"       %s encapX debug\n",__progname);
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

static int cidr_len(unsigned long int v)
{
 int l;

 v = 0xffffffff & ~v;
 if (v & (v+1)) return(-1);
 for (l=0;v;v>>=1,l++) ;
 return(32-l);
}

static void query_n(int n)
{
 struct encap_rt r;
 int len;
 int l;

 r.inx = n;
 r.secretlen = 0;
 if (ioctl(devfd,ENCAP_GETRT,&r) < 0)
  { fprintf(stderr,"%s: can't get rt #%d: %s\n",__progname,n,strerror(errno));
    return;
  }
 /* this loop is to defend against racing with another run
    which increases the secret's length. */
 while (1)
  { len = r.secretlen;
    r.secret = malloc(len);
    if (ioctl(devfd,ENCAP_GETRT,&r) < 0)
     { fprintf(stderr,"%s: can't get rt #%d: %s\n",__progname,n,strerror(errno));
       return;
     }
    if (r.secretlen <= len) break;
    free(r.secret);
  }
 printf("%d:",n);
 printf(" %s",inet_ntoa(r.srcmatch));
 l = cidr_len(ntohl(r.srcmask.s_addr));
 if (l >= 0) printf(" /%d",l); else printf(" %s",inet_ntoa(r.srcmask));
 printf(" %s",inet_ntoa(r.src));
 printf(" %s",inet_ntoa(r.dst));
 if (r.curdst.s_addr != r.dst.s_addr) printf(" (%s)",inet_ntoa(r.curdst));
 printf(" %d %d ",r.id,r.ttl);
 fwrite(r.secret,1,r.secretlen,stdout);
 printf("\n");
}

static void print_list(struct encap_listio *l, const char *tag)
{
 int i;
 int n;

 printf("%s list:",tag);
 for (i=0;i<l->oldn;i++)
  { struct in_addr ia;
    ia.s_addr = htonl(l->oldlist[i].net);
    printf(" %s",inet_ntoa(ia));
    n = cidr_len(l->oldlist[i].mask);
    if (n >= 0)
     { printf("/%d",n);
     }
    else
     { ia.s_addr = htonl(l->oldlist[i].mask);
       printf("/%s",inet_ntoa(ia));
     }
  }
 printf("\n");
}

static void query_list(unsigned long int ioccmd, const char *tag)
{
 struct encap_listio l;
 struct encap_listentry *e;
 int n;

 l.newn = -1;
 l.oldn = 0;
 if (ioctl(devfd,ioccmd,&l) < 0)
  { fprintf(stderr,"%s: can't get %s list size: %s\n",__progname,tag,strerror(errno));
    exit(1);
  }
 while (1)
  { n = l.oldn;
    e = malloc(n*sizeof(*e));
    l.oldlist = e;
    if (ioctl(devfd,ioccmd,&l) < 0)
     { fprintf(stderr,"%s: can't get %s list: %s\n",__progname,tag,strerror(errno));
       exit(1);
     }
    if (l.oldn != n)
     { free(e);
       continue;
     }
    break;
  }
 print_list(&l,tag);
 free(e);
}

static void do_query(int narg)
{
 int i;
 int n;

 open_dev(O_RDONLY);
 if (narg >= 0)
  { query_n(narg);
  }
 else
  { if (ioctl(devfd,ENCAP_GETNRT,&n) < 0)
     { fprintf(stderr,"%s: can't get count: %s\n",__progname,strerror(errno));
       exit(1);
     }
    for (i=0;i<n;i++) query_n(i);
    query_list(ENCAP_BLOCK,"block");
    query_list(ENCAP_POISON,"poison");
    query_list(ENCAP_POISONED,"poisoned");
  }
}

static void do_del(unsigned int n)
{
 open_dev(O_RDWR);
 if (ioctl(devfd,ENCAP_DELRT,&n) < 0)
  { fprintf(stderr,"%s: can't delete #%u: %s\n",__progname,n,strerror(errno));
    exit(1);
  }
}

static void do_send(unsigned int n)
{
 open_dev(O_RDWR);
 if (ioctl(devfd,ENCAP_SENDTO,&n) < 0)
  { fprintf(stderr,"%s: can't send to #%u: %s\n",__progname,n,strerror(errno));
    exit(1);
  }
}

static void do_reboot(unsigned int n)
{
 open_dev(O_RDWR);
 if (ioctl(devfd,ENCAP_REBOOT,&n) < 0)
  { fprintf(stderr,"%s: can't send reboot request to #%u: %s\n",__progname,n,strerror(errno));
    exit(1);
  }
}

static void do_set(int n)
{
 struct encap_rt r;
 long int lv;
 char *ep;

 open_dev(O_RDWR);
 if (n < 0)
  { unsigned int v;
    if (ioctl(devfd,ENCAP_GETNRT,&v) < 0)
     { fprintf(stderr,"%s: can't get count: %s\n",__progname,strerror(errno));
       exit(1);
     }
    n = v;
  }
 r.inx = n;
 if (! inet_aton(txt_addr,&r.srcmatch))
  { fprintf(stderr,"%s: %s: invalid match address\n",__progname,txt_addr);
    exit(1);
  }
 if (txt_mask[0] == '/')
  { int w;
    w = atoi(txt_mask+1);
    if ((w < 0) || (w > 32))
     { fprintf(stderr,"%s: %s: out-of-range CIDR width\n",__progname,txt_mask);
       exit(1);
     }
#define XFF 0xffffffffUL
    r.srcmask.s_addr = (w == 32) ? XFF : htonl(XFF&~(XFF>>w));
#undef XFF
  }
 else if (! inet_aton(txt_mask,&r.srcmask))
  { fprintf(stderr,"%s: %s: invalid match mask\n",__progname,txt_mask);
    exit(1);
  }
 if (! inet_aton(txt_src,&r.src))
  { fprintf(stderr,"%s: %s: invalid source address\n",__progname,txt_src);
    exit(1);
  }
 if (! inet_aton(txt_dst,&r.dst))
  { fprintf(stderr,"%s: %s: invalid destination address\n",__progname,txt_dst);
    exit(1);
  }
 lv = strtol(txt_id,&ep,0);
 if (*ep || (ep == txt_id) || (lv < 0) || (lv > 255))
  { fprintf(stderr,"%s: %s: invalid tunnel ID\n",__progname,txt_id);
    exit(1);
  }
 r.id = lv;
 lv = strtol(txt_ttl,&ep,0);
 if (*ep || (ep == txt_ttl) || (lv < 0) || (lv > 255))
  { fprintf(stderr,"%s: %s: invalid TTL value\n",__progname,txt_ttl);
    exit(1);
  }
 r.ttl = lv;
 r.secret = txt_secret;
 r.secretlen = strlen(txt_secret);
 if (ioctl(devfd,ENCAP_SETRT,&r) < 0)
  { fprintf(stderr,"%s: can't set route: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_flags(void)
{
 unsigned int f;
 int i;

 open_dev(O_RDONLY);
 if (ioctl(devfd,ENCAP_GFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't get flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
 for (i=0;flagbits[i].name;i++)
  { printf(" %c%s",(f&flagbits[i].bit)?'+':'-',flagbits[i].name);
    f &= ~flagbits[i].bit;
  }
 if (f) printf(" +0x%x",f);
 printf("\n");
}

static void do_sflag(void)
{
 unsigned int f;
 unsigned int b;
 int i;

 switch (txt_flg[0])
  { case '+': case '-': break;
    default:
       fprintf(stderr,"%s: last argument must be +flag or -flag\n",__progname);
       exit(1);
       break;
  }
 for (i=0;flagbits[i].name;i++) if (!strcmp(flagbits[i].name,txt_flg+1)) break;
 if (! flagbits[i].name)
  { fprintf(stderr,"%s: unrecognized flag bit `%s'\n",__progname,txt_flg+1);
    exit(1);
  }
 b = flagbits[i].bit;
 open_dev(O_RDWR);
 if (ioctl(devfd,ENCAP_GFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't get flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (txt_flg[0] == '+') f |= b; else f &= ~b;
 if (ioctl(devfd,ENCAP_SFLAGS,&f) < 0)
  { fprintf(stderr,"%s: can't set flags: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_debug(void)
{
 void *vp;

 open_dev(O_RDWR);
 vp = 0;
 if (ioctl(devfd,ENCAP_DEBUG,&vp) < 0)
  { fprintf(stderr,"%s: can't ENCAP_DEBUG: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void do_list(char **txtlist, unsigned long int ioccmd, const char *tag)
{
 int i;
 int n;
 struct encap_listentry *e;
 struct encap_listio l;
 int also_query;
 char *t;
 char *post;
 struct in_addr ia;
 char *slash;

 open_dev(O_RDWR);
 for (n=0;txtlist[n];n++) ;
 e = malloc(n*sizeof(*e));
 l.newn = 0;
 l.newlist = e;
 also_query = 0;
 for (i=0;i<n;i++)
  { if (!strcmp(txtlist[i],"query"))
     { also_query = 1;
       continue;
     }
    t = strdup(txtlist[i]);
    post = 0;
    slash = index(t,'/');
    if (slash)
     { *slash++ = '\0';
       post = slash;
     }
    if (! inet_aton(t,&ia))
     { fprintf(stderr,"%s; %s: invalid address\n",__progname,t);
       exit(1);
     }
    l.newlist[l.newn].net = ntohl(ia.s_addr);
    if (post)
     { char *ep;
       int w;
       w = strtol(post,&ep,10);
       if ((ep == post) || *ep)
	{ if (! inet_aton(post,&ia))
	   { fprintf(stderr,"%s; %s: invalid mask\n",__progname,t);
	     exit(1);
	   }
	  l.newlist[l.newn].mask = ntohl(ia.s_addr);
	}
       else
#define XFF 0xffffffffUL
	{ l.newlist[l.newn].mask = (w == 32) ? XFF : htonl(XFF&~(XFF>>w));
	}
#undef XFF
     }
    else
     { l.newlist[l.newn].mask = 0xffffffffUL;
     }
    free(t);
    l.newn ++;
  }
 if (also_query)
  { struct encap_listio t;
    t.oldn = 0;
    t.newn = -1;
    if (ioctl(devfd,ioccmd,&t) < 0)
     { fprintf(stderr,"%s: can't get %s list size: %s\n",__progname,tag,strerror(errno));
       exit(1);
     }
    l.oldn = t.oldn;
    l.oldlist = malloc(t.oldn*sizeof(*l.oldlist));
  }
 else
  { l.oldn = -1;
    l.oldlist = 0;
  }
 if (ioctl(devfd,ioccmd,&l) < 0)
  { fprintf(stderr,"%s: can't get/set %s list: %s\n",__progname,tag,strerror(errno));
    exit(1);
  }
 if (also_query) print_list(&l,tag);
 free(l.newlist);
 free(l.oldlist);
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 switch (action)
  { case ACT_QUERYALL:
       do_query(-1);
       break;
    case ACT_QUERYONE:
       do_query(atoi(txt_n));
       break;
    case ACT_DEL:
       do_del(atoi(txt_n));
       break;
    case ACT_SEND:
       do_send(atoi(txt_n));
       break;
    case ACT_REBOOT:
       do_reboot(atoi(txt_n));
       break;
    case ACT_ADD:
       do_set(-1);
       break;
    case ACT_SET:
       do_set(atoi(txt_n));
       break;
    case ACT_FLAGS:
       do_flags();
       break;
    case ACT_SFLAG:
       do_sflag();
       break;
    case ACT_BLOCK:
       do_list(txt_toblock,ENCAP_BLOCK,"block");
       break;
    case ACT_POISON:
       do_list(txt_topoison,ENCAP_POISON,"poison");
       break;
    case ACT_DEBUG:
       do_debug();
       break;
    default:
       abort();
       break;
  }
 exit(0);
}
