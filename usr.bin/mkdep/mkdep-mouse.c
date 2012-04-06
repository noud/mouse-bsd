/*
 * This program is in the public domain.
 *
 * mkdep - make dependencies.
 *
 * Loosely speaking, runs $CC -M "$@" to get a list of dependencies,
 *  which it dumps in ".depend".  If $CC isn't set in the environment,
 *  searches for cc, then gcc, on the path, using the first one it
 *  finds.  Flags:
 *
 *	-f file	Write to "file" instead of ".depend".
 *
 *	-a	Append to, rather than overwriting, the output file.
 *
 *	-p	Produce dependencies of the "foo: foo.c" style, instead
 *		of the default "foo.o: foo.c" style.
 *
 *	--	Explicit end-of-flags marker.
 *
 * The first unrecognized argument, flag or otherwise, marks the
 *  beginning of the list of arguments passed to $CC; -- can be used as
 *  an explicit marker in case the arguments for $CC begin, or might
 *  begin, with a flag that has meaning to mkdep.
 *
 * If $CC is set, it is run by forking sh and passing it $CC, followed
 *  by the relevant arguments (with backslashes wherever they might be
 *  necessary).  if $CC is not set, the cc (or gcc) found is run
 *  directly, bypassing the shell.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

extern const char *__progname;

static const char *depfile = ".depend";
static int aflag = 0;
static int pflag = 0;
static FILE *tmpf;
static FILE *p_t;
static FILE *p_f;
static FILE *outf;
static pid_t kidpid;

static void open_tmpf(void)
{
 tmpf = tmpfile();
 if (tmpf == 0)
  { fprintf(stderr,"%s: can't open temp file: %s\n",__progname,strerror(errno));
    exit(1);
  }
}

static void dmove(int f, int t)
{
 if (f != t)
  { dup2(f,t);
    close(f);
  }
}

static void run_child_process(const char **argv)
{
 int pt[2];
 int pf[2];
 int pv[2];
 char junk;

 if ( (pipe(&pt[0]) < 0) ||
      (pipe(&pf[0]) < 0) ||
      (pipe(&pv[0]) < 0) )
  { fprintf(stderr,"%s: pipe: %s\n",__progname,strerror(errno));
    exit(1);
  }
 kidpid = fork();
 if (kidpid < 0)
  { fprintf(stderr,"%s: fork: %s\n",__progname,strerror(errno));
    exit(1);
  }
 if (kidpid == 0)
  { close(pt[1]);
    close(pf[0]);
    close(pv[0]);
    dmove(pt[0],0);
    dmove(pf[1],1);
    fcntl(pv[1],F_SETFD,1);
    execvp(argv[0],(void *)argv);
    fprintf(stderr,"%s: can't exec %s: %s\n",__progname,argv[0],strerror(errno));
    write(pv[1],"",1);
    exit(0);
  }
 close(pt[0]);
 close(pf[1]);
 close(pv[1]);
 if (read(pv[0],&junk,1) > 0) exit(1);
 close(pv[0]);
 p_t = fdopen(pt[1],"w");
 p_f = fdopen(pf[0],"r");
}

static void write_child(const char *s)
{
 fprintf(p_t,"%s",s);
}

static void write_child_quoted(const char *s)
{
 for (;*s;s++)
  { if (isalnum(*s))
     { putc(*s,p_t);
     }
    else
     { switch (*s)
	{ case '_': case '-': case '.': case '/': case '=': case '+': case ':':
	     putc(*s,p_t);
	     break;
	  case '\n':
	     fprintf(p_t,"'\n'");
	     break;
	  default:
	     fprintf(p_t,"\\%c",*s);
	     break;
	}
     }
  }
}

static void close_child_input(void)
{
 fclose(p_t);
}

static const char *find_cc(const char *suf)
{
 int suflen;
 const char *pp;
 const char *pe_beg;
 const char *pe_end;
 int pe_len;
 int pbuflen;
 char *pbuf;

 pbuflen = 0;
 pbuf = 0;
 suflen = strlen(suf);
 pp = getenv("PATH");
 if (pp == 0) pp = _PATH_STDPATH;
 while (1)
  { pe_beg = pp;
    while (*pp && (*pp != ':')) pp ++;
    pe_end = pp;
    pe_len = pe_end - pe_beg;
    if (pe_len+1+suflen+1 > pbuflen)
     { pbuflen = pe_len+1+suflen+1+8;
       free(pbuf);
       pbuf = malloc(pbuflen);
     }
    if (pe_len == 0)
     { strcpy(pbuf,suf);
     }
    else
     { sprintf(pbuf,"%.*s/%s",pe_len,pe_beg,suf);
     }
    if (access(pbuf,X_OK) == 0) return(pbuf);
    if (*pp) pp ++; else break;
  }
 free(pbuf);
 return(0);
}

static void run_child(int ac, char **av)
{
 if (getenv("CC"))
  { int i;
    static const char *argv[] = { _PATH_BSHELL, 0 };
    run_child_process(&argv[0]);
    write_child(getenv("CC"));
    write_child(" -M");
    for (i=0;av[i];i++)
     { write_child(" ");
       write_child_quoted(av[i]);
     }
    write_child("\n");
  }
 else
  { const char *cc;
    const char **argv;
    int i;
    cc = find_cc("cc");
    if (cc == 0) cc = find_cc("gcc");
    if (cc == 0)
     { fprintf(stderr,"%s: can't find cc or gcc in $PATH, and $CC not set\n",__progname);
       exit(1);
     }
    argv = malloc((2+ac+1)*sizeof(const char *));
    argv[0] = cc;
    argv[1] = "-M";
    for (i=0;i<ac;i++) argv[2+i] = av[i];
    argv[2+i] = 0;
    run_child_process(argv);
  }
 close_child_input();
}

static void process_child_output(void)
{
 int afterspace;
 int begline;
 int beforecolon;
 int afterbackslash;
 int c;

 afterspace = 1;
 afterbackslash = 0;
 begline = 1;
 beforecolon = 1;
 while (1)
  { c = getc(p_f);
    if (c == EOF) break;
    if (afterspace && (c == '.'))
     { c = getc(p_f);
       if (c == '/') continue;
       if (c == EOF)
	{ putc('.',tmpf);
	  break;
	}
       ungetc(c,p_f);
       c = '.';
     }
    if (pflag && beforecolon && (c == '.'))
     { c = getc(p_f);
       if (c == 'o')
	{ c = getc(p_f);
	  if (c == EOF)
	   { putc('.',tmpf);
	     putc('o',tmpf);
	     break;
	   }
	  if (! (isspace(c) || (c == ':')))
	   { ungetc(c,p_f);
	     ungetc('o',p_f);
	     c = '.';
	   }
	}
       else if (c == EOF)
	{ putc('.',tmpf);
	  break;
	}
       else
	{ ungetc(c,p_f);
	  c = '.';
	}
     }
    putc(c,tmpf);
    if (beforecolon && (c == ':')) beforecolon = 0;
    if ((c == '\n') && !afterbackslash)
     { beforecolon = 1;
     }
    afterspace = isspace(c);
    afterbackslash = (c == '\\');
    begline = (c == '\n');
  }
}

static void check_child_status(void)
{
 int status;
 pid_t pid;

 while (1)
  { pid = wait(&status);
    if (pid < 0)
     { if (errno == EINTR) continue;
       if (errno == ECHILD)
	{ fprintf(stderr,"%s: warning: lost child %d, assuming success\n",__progname,(int)kidpid);
	  return;
	}
       fprintf(stderr,"%s: unexpected wait error: %s\n",__progname,strerror(errno));
       exit(1);
     }
    if (pid != kidpid)
     { fprintf(stderr,"%s: warning: ignoring unexpected child %d\n",__progname,(int)pid);
       continue;
     }
    if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) return;
    fprintf(stderr,"%s: compile failed (",__progname);
    if (WIFEXITED(status))
     { fprintf(stderr,"exit %d",(int)WEXITSTATUS(status));
     }
    else if (WIFSIGNALED(status))
     { fprintf(stderr,"%s%s",strsignal(WTERMSIG(status)),WCOREDUMP(status)?", core dumped":"");
     }
    else
     { fprintf(stderr,"strange status %#x",status);
     }
    fprintf(stderr,")\n");
    exit(1);
  }
}

static void open_outf(void)
{
 outf = fopen(depfile,aflag?"a":"w");
 if (outf == 0)
  { fprintf(stderr,"%s: can't open output file %s: %s\n",__progname,depfile,strerror(errno));
    exit(1);
  }
}

static void copy_output(void)
{
 int c;

 while (1)
  { c = getc(tmpf);
    if (c == EOF) break;
    putc(c,outf);
  }
}

int main(int, char **);
int main(int ac, char **av)
{
 for (ac--,av++;ac;ac--,av++)
  { if (!strcmp(*av,"-a"))
     { aflag = 1;
       continue;
     }
    if (!strcmp(*av,"-p"))
     { pflag = 1;
       continue;
     }
    if (!strcmp(*av,"-f") && (ac > 1))
     { depfile = av[1];
       ac --;
       av ++;
       continue;
     }
    if (!strcmp(*av,"--"))
     { ac --;
       av ++;
     }
    break;
  }
 open_tmpf();
 run_child(ac,av);
 process_child_output();
 check_child_status();
 rewind(tmpf);
 open_outf();
 copy_output();
 fclose(tmpf);
 fclose(outf);
 exit(0);
}
