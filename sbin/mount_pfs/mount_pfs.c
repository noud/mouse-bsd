#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <miscfs/pfs/pfs.h>

#include "mntopts.h"

#include "state.h"
#include "requests.h"

extern const char *__progname;

const struct mntopt mopts[]
 = { MOPT_STDOPTS,
     { 0 } };

static int failnum = 0;
static int failden = 1;

static void usage(void) __attribute__((__noreturn__));
static void usage(void)
{
 fprintf(stderr,"Usage: %s [-o options] arg mount-point\n",__progname);
 exit(1);
}

static int getrequest(int fd, struct pfs_req *req, void *data, int datalen, int *auxp)
{
 struct iovec iov[2];
 struct msghdr mh;
 struct cmsghdr cmh;
 char ctlbuf[sizeof(struct cmsghdr)+sizeof(int)];
 int n;

 iov[0].iov_base = req;
 iov[0].iov_len = sizeof(struct pfs_req);
 iov[1].iov_base = data;
 iov[1].iov_len = datalen;
 mh.msg_name = 0;
 mh.msg_namelen = 0;
 mh.msg_iov = &iov[0];
 mh.msg_iovlen = 2;
 mh.msg_control = &ctlbuf[0];
 mh.msg_controllen = sizeof(ctlbuf);
 mh.msg_flags = 0;
 n = recvmsg(fd,&mh,0);
 if (n < 0)
  { fprintf(stderr,"%s: daemon recvmsg: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (n == 0)
  { fprintf(stderr,"%s: daemon EOF\n",__progname);
    exit(0);
  }
 if (mh.msg_controllen == 0) return(n);
 if (mh.msg_controllen != sizeof(struct cmsghdr)+sizeof(int))
  { fprintf(stderr,"%s: daemon controllen %d not %d\n",__progname,(int)mh.msg_controllen,(int)(sizeof(struct cmsghdr)+sizeof(int)));
    exit(1);
  }
 bcopy(&ctlbuf[0],&cmh,sizeof(cmh));
 if (cmh.cmsg_len != mh.msg_controllen)
  { fprintf(stderr,"%s: daemon cmsg_len %d not %d\n",__progname,(int)cmh.cmsg_len,(int)(sizeof(struct cmsghdr)+sizeof(int)));
    exit(1);
  }
 if (cmh.cmsg_level != SOL_SOCKET)
  { fprintf(stderr,"%s: daemon cmsg_level %d not %d\n",__progname,(int)cmh.cmsg_level,(int)SOL_SOCKET);
    exit(1);
  }
 if (cmh.cmsg_type != SCM_RIGHTS)
  { fprintf(stderr,"%s: daemon cmsg_type %d not %d\n",__progname,(int)cmh.cmsg_level,(int)SCM_RIGHTS);
    exit(1);
  }
 bcopy(&ctlbuf[sizeof(cmh)],auxp,sizeof(int));
 return(n);
}

void sendreply(int fd, unsigned int id, void *data, int datalen)
{
 struct pfs_rep rep;
 struct iovec iov[2];
 struct msghdr mh;
 int n;

 rep.id = id;
 iov[0].iov_base = &rep;
 iov[0].iov_len = sizeof(struct pfs_rep);
 iov[1].iov_base = data;
 iov[1].iov_len = datalen;
 mh.msg_name = 0;
 mh.msg_namelen = 0;
 mh.msg_iov = &iov[0];
 mh.msg_iovlen = 2;
 mh.msg_control = 0;
 mh.msg_controllen = 0;
 mh.msg_flags = 0;
 n = sendmsg(fd,&mh,0);
 if (n < 0)
  { fprintf(stderr,"%s: daemon sendmsg: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (n != datalen+sizeof(struct pfs_rep))
  { fprintf(stderr,"%s: daemon short sendmsg (wanted %d, sent %d)\n",__progname,datalen+sizeof(struct pfs_rep),n);
    exit(0);
  }
}

void protofail(const char *fmt, ...)
{
 va_list ap;

 fprintf(stderr,"%s: protocol failure: ",__progname);
 va_start(ap,fmt);
 vfprintf(stderr,fmt,ap);
 va_end(ap);
 fprintf(stderr,"\n");
 exit(1);
}

static void daemon_run(int fd)
{
 int i;
 int n;
 struct pfs_req req;
 char reqbuf[PFS_REQ_MAX-sizeof(struct pfs_req)];
 int auxsock;

 signal(SIGPIPE,SIG_IGN);
 init_state();
 while (1)
  { auxsock = -1;
    n = getrequest(fd,&req,&reqbuf[0],sizeof(reqbuf),&auxsock);
    if (n < sizeof(struct pfs_req))
     { fprintf(stderr,"%s: daemon undersize request (%d)\n",__progname,n);
       exit(0);
     }
    for (i=reqtbl_n-1;i>=0;i--)
     { if (req.op == reqtbl[i].op)
	{ (*reqtbl[i].handler)(&req,fd,&reqbuf[0],n-sizeof(req),auxsock);
	  break;
	}
     }
    if (i < 0)
     { fprintf(stderr,"%s: daemon unrecognized request %u\n",__progname,req.op);
       exit(1);
     }
    if (auxsock >= 0) close(auxsock);
  }
}

static void setfailrate(const char *s)
{
 char *e;
 long int n;
 long int d;

 srandom(time(0));
 n = strtol(s,&e,0);
 if (s == e)
  { fprintf(stderr,"%s: failure rate argument %s doesn't begin with a number\n",__progname,s);
    exit(1);
  }
 if (n < 0)
  { fprintf(stderr,"%s: failure rate argument %s has negative number\n",__progname,s);
    exit(1);
  }
 if (*e == '\0')
  { if (n > 100)
     { fprintf(stderr,"%s: failure rate argument %s is not a percentage but has no /\n",__progname,s);
       exit(1);
     }
    failnum = n;
    failden = 100;
    return;
  }
 if (*e != '/')
  { fprintf(stderr,"%s: failure rate argument %s has garbage after first number\n",__progname,s);
    exit(1);
  }
 s = e + 1;
 d = strtol(s,&e,0);
 if (*e != '\0')
  { fprintf(stderr,"%s: failure rate argument %s has garbage after second number\n",__progname,s);
    exit(1);
  }
 if (d < 0)
  { fprintf(stderr,"%s: failure rate argument %s has negative number\n",__progname,s);
    exit(1);
  }
 if (d < n)
  { fprintf(stderr,"%s: failure rate argument %s has numerator > denominator\n",__progname,s);
    exit(1);
  }
 failnum = n;
 failden = d;
}

void maybefail(void)
{
 if (failnum < 1) return;
 if ((random()%failden) >= failnum) return;
 printf("simulating failure\n");
 exit(0);
}

int main(int, char **);
int main(int ac, char **av)
{
 struct pfs_args args;
 struct pfs_1_0_args args10;
 int p[2];
 int ch;
 char *mountpt;
 int err;
 int mntflags;
 pid_t kid;
 int foreground;

 mntflags = 0;
 foreground = 0;
 err = 0;
 while (1)
  { ch = getopt(ac,av,"fo:F:");
    if (ch == -1) break;
    switch (ch)
     { case 'f':
	  foreground = 1;
	  break;
       case 'o':
	  getmntopts(optarg,&mopts[0],&mntflags,0);
	  break;
       case 'F':
	  setfailrate(optarg);
	  break;
       default:
	  err = 1;
	  break;
     }
  }
 if (optind != ac-2) err = 1;
 if (err) usage();
 args10.pfsa_string = av[optind];
 mountpt = av[optind+1];
 if (socketpair(AF_LOCAL,SOCK_DGRAM,0,&p[0]) < 0)
  { fprintf(stderr,"%s: socketpair: %s\n",__progname,strerror(errno));
    exit(1);
  }
 args.pfsa_major = 0;
 args.pfsa_minor = 0;
 args.pfsa_data = 0;
 err = mount(MOUNT_PFS,mountpt,mntflags,&args);
 if (err >= 0)
  { unmount(mountpt,MNT_FORCE);
    fprintf(stderr,"%s: version zero mount succeeded(?""?)\n",__progname);
    exit(1);
  }
 if (errno != EINPROGRESS)
  { fprintf(stderr,"%s: mount: %s\n",__progname,strerror(errno));
    exit(1);
  }
 printf("kernel wants version %d.%d\n",args.pfsa_major,args.pfsa_minor);
 args.pfsa_major = 1;
 args.pfsa_minor = 0;
 args.pfsa_data = &args10;
 args10.pfsa_socket = p[0];
 kid = fork();
 if (kid < 0)
  { fprintf(stderr,"%s: fork daemon: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (foreground ? kid : !kid)
  { close(p[0]);
    daemon_run(p[1]);
    _exit(1);
  }
 close(p[1]);
 err = mount(MOUNT_PFS,mountpt,mntflags,&args);
 if (err < 0)
  { fprintf(stderr,"%s: mount: %s\n",__progname,strerror(errno));
    exit(1);
  }
 exit(0);
}
