/* This file is in the public domain. */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>

extern const char *__progname;

extern char *crypt_makesalt(void); /* belongs in an include file somewhere */

static const char *dot_otp = ".motp";
static const char *pwfile = "passwords";

static const char *otp_dir = 0;
static int dot_fd;
static int otpdir_fd;
static int pwfile_fd;
static volatile int got_sigalrm;

static void handleargs(int ac, char **av)
{
 int skip;
 int errs;

 skip = 0;
 errs = 0;
 for (ac--,av++;ac;ac--,av++)
  { if (skip > 0)
     { skip --;
       continue;
     }
    if (**av != '-')
     { fprintf(stderr,"%s: unrecognized argument `%s'\n",__progname,*av);
       errs ++;
       continue;
     }
    if (0)
     {
needarg:;
       fprintf(stderr,"%s: %s needs a following argument\n",__progname,*av);
       errs ++;
       continue;
     }
#define WANTARG() do { if (++skip >= ac) goto needarg; } while (0)
    if (!strcmp(*av,"-dir"))
     { WANTARG();
       otp_dir = av[skip];
       continue;
     }
#undef WANTARG
    fprintf(stderr,"%s: unrecognized option `%s'\n",__progname,*av);
    errs ++;
  }
 if (errs)
  { exit(1);
  }
}

static void make_otp_dir(void)
{
 struct stat stb;
 int maybe_create;

 if (otp_dir)
  { maybe_create = 0;
  }
 else
  { char *home;
    char *tmp;
    home = getenv("HOME");
    if (home == 0)
     { fprintf(stderr,"%s: no $HOME\n",__progname);
       exit(1);
     }
    tmp = malloc(strlen(home)+1+strlen(dot_otp)+1);
    sprintf(tmp,"%s/%s",home,dot_otp);
    otp_dir = tmp;
    maybe_create = 1;
  }
 otpdir_fd = open(otp_dir,O_RDONLY,0);
 if (otpdir_fd < 0)
  { if (maybe_create && (errno == ENOENT))
     { if (mkdir(otp_dir,0700) < 0)
	{ fprintf(stderr,"%s: can't mkdir %s: %s\n",__progname,otp_dir,strerror(errno));
	  exit(1);
	}
       otpdir_fd = open(otp_dir,O_RDONLY,0);
     }
    if (otpdir_fd < 0)
     { fprintf(stderr,"%s: %s: %s\n",__progname,otp_dir,strerror(errno));
       exit(1);
     }
  }
 fstat(otpdir_fd,&stb);
 if ((stb.st_mode & S_IFMT) != S_IFDIR)
  { fprintf(stderr,"%s: %s: %s\n",__progname,otp_dir,strerror(ENOTDIR));
    exit(1);
  }
}

static void open_dot(void)
{
 dot_fd = open(".",O_RDONLY,0);
 if (dot_fd < 0)
  { fprintf(stderr,"%s: .: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void sigalrm(int sig __attribute__ ((unused)))
{
 got_sigalrm = 1;
}

static void sigalrm_in(double sec)
{
 struct itimerval itv;

 if (sec < 0) abort();
 itv.it_interval.tv_sec = 0;
 itv.it_interval.tv_usec = 0;
 if (sec == 0)
  { itv.it_value = itv.it_interval;
  }
 else
  { itv.it_value.tv_sec = (int) sec;
    if (itv.it_value.tv_sec > sec) itv.it_value.tv_sec --;
    sec -= itv.it_value.tv_sec;
    itv.it_value.tv_usec = sec * 1000000;
    if (itv.it_value.tv_usec >= 1000000)
     { itv.it_value.tv_usec -= 1000000;
       itv.it_value.tv_sec ++;
     }
    if ((itv.it_value.tv_sec == 0) && (itv.it_value.tv_usec == 0)) itv.it_value.tv_usec = 1;
  }
 setitimer(ITIMER_REAL,&itv,0);
}

static void lock_pwfile(void)
{
 if (flock(pwfile_fd,LOCK_EX|LOCK_NB) >= 0) return;
 if (errno != EWOULDBLOCK)
  { fprintf(stderr,"%s: flock %s/%s: %s\n",__progname,otp_dir,pwfile,strerror(errno));
    exit(1);
  }
 printf("[waiting for lock]\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
 fflush(stdout);
 got_sigalrm = 0;
 sigalrm_in(10);
 while (1)
  { /* race here - hope flock() blocks before the signal arrives! */
    /* unfortunately is no way to fix this, short of sigflock() */
    if (flock(pwfile_fd,LOCK_EX) >= 0)
     { sigalrm_in(0);
       return;
     }
    if (errno != EINTR)
     { fprintf(stderr,"%s: flock %s/%s: %s\n",__progname,otp_dir,pwfile,strerror(errno));
       exit(1);
     }
    if (got_sigalrm)
     { fprintf(stderr,"%s: lock on %s/%s appears stuck\n",__progname,otp_dir,pwfile);
       exit(1);
     }
  }
}

static void unlock_pwfile(void)
{
 if (flock(pwfile_fd,LOCK_UN) < 0)
  { fprintf(stderr,"%s: can't unlock %s/%s: %s\n",__progname,otp_dir,pwfile,strerror(errno));
    exit(1);
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 handleargs(ac,av);
 signal(SIGALRM,sigalrm);
 open_dot();
 make_otp_dir();
 fchdir(otpdir_fd);
 pwfile_fd = open(pwfile,O_RDWR|O_CREAT,0600);
 if (pwfile_fd < 0)
  { fprintf(stderr,"%s: %s/%s: %s\n",__progname,otp_dir,pwfile,strerror(errno));
    exit(1);
  }
 while (1)
  { char pw[1024];
    int l;
    char *cryptresult;
    printf("motp-add> ");
    if (fgets(&pw[0],sizeof(pw),stdin) != &pw[0]) exit(0);
    l = strlen(&pw[0]);
    if ((l > 0) && (pw[l-1] == '\n')) pw[--l] = '\0';
    cryptresult = crypt(&pw[0],crypt_makesalt());
    bzero(&pw[0],sizeof(pw));
    lock_pwfile();
    lseek(pwfile_fd,0,SEEK_END);
    write(pwfile_fd,cryptresult,strlen(cryptresult)+1);
    unlock_pwfile();
  }
}
