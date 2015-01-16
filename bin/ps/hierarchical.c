#include <stdlib.h>
#include <sys/sysctl.h>

#include "ps.h"

extern KINFO *kinfo;

typedef struct node NODE;

struct node {
  NODE *parent;
  NODE *child;
  NODE *sibling;
  } ;

static KINFO *kivec;
static int nkiv;
static int (*basecmp)(const void *, const void *);

static int *pidx;
static int *ppidx;
static NODE *nodes;
static NODE *roots;
static int *kidvec;

static int cmp_pid(const void *a, const void *b)
{
 return(KI_PROC(kivec+*(const int *)a)->p_pid-KI_PROC(kivec+*(const int *)b)->p_pid);
}

static int cmp_ppid(const void *a, const void *b)
{
 return(KI_EPROC(kivec+*(const int *)a)->e_ppid-KI_EPROC(kivec+*(const int *)b)->e_ppid);
}

static int cmp_kid(const void *a, const void *b)
{
 if ((KI_EPROC(kivec+*(const int *)a)->e_ppid == 0) && (KI_PROC(kivec+*(const int *)a)->p_pid != 1))
  { if ((KI_EPROC(kivec+*(const int *)b)->e_ppid == 0) && (KI_PROC(kivec+*(const int *)b)->p_pid != 1))
     { return((*basecmp)(kivec+*(const int *)a,kivec+*(const int *)b));
     }
    else
     { return(-1);
     }
  }
 else
  { if ((KI_EPROC(kivec+*(const int *)b)->e_ppid == 0) && (KI_PROC(kivec+*(const int *)b)->p_pid != 1))
     { return(1);
     }
    else
     { return((*basecmp)(kivec+*(const int *)a,kivec+*(const int *)b));
     }
  }
}

static int cmp_order(const void *a, const void *b)
{
 return(((const KINFO *)a)->order-((const KINFO *)b)->order);
}

static int find_pid_x(int pid)
{
 int l;
 int m;
 int h;
 int p;

 l = -1;
 h = nkiv;
 while (h-l > 1)
  { m = (h + l) / 2;
    p = KI_PROC(kivec+pidx[m])->p_pid;
    if (p <= pid) l = m;
    if (p >= pid) h = m;
  }
 return((l==h)?pidx[l]:-1);
}

static NODE *sort_sibs(NODE *list)
{
 int i;
 NODE *n;

 i = 0;
 for (n=list;n;n=n->sibling) kidvec[i++] = n - nodes;
 if (i > 1) qsort(kidvec,i,sizeof(int),&cmp_kid);
 list = 0;
 for (i--;i>=0;i--)
  { n = &nodes[kidvec[i]];
    n->sibling = list;
    list = n;
  }
 return(list);
}

static void buildtree(NODE **listp, NODE *parentp, int parentx)
{
 int i;
 int p;
 NODE *n;
 NODE *list;

 list = 0;
 for (i=nkiv-1;i>=0;i--)
  { /*
     * This is annoying.  PPID is 0 for system processes - but one of
     *	them has PID 0!  PPID 0 seems to mean "no parent" rather than
     *	"parent is PID 0", so that's how we treat it.
     */
    p = KI_EPROC(kivec+i)->e_ppid;
    p = p ? find_pid_x(p) : -1;
    if (p == parentx)
     { n = nodes + i;
       n->parent = parentp;
       n->sibling = list;
       list = n;
       buildtree(&n->child,n,i);
       kivec[i].indent = 0;
     }
  }
 *listp = sort_sibs(list);
}

static void check_orphans(void)
{
 int i;
 NODE *n;
 int any;

 any = 0;
 for (i=nkiv-1;i>=0;i--)
  { if (kivec[i].indent < 0)
     { n = nodes + i;
       n->parent = 0;
       n->sibling = roots;
       roots = n;
       any = 1;
     }
  }
 if (any) roots = sort_sibs(roots);
}

static void gen_order(void)
{
 int x;

 void walk(NODE *list, int indent)
  { KINFO *k;
    for (;list;list=list->sibling)
     { k = kivec + (list - nodes);
       k->indent = indent;
       k->order = x ++;
       walk(list->child,indent+1);
     }
  }

 x = 0;
 walk(roots,0);
}

void sort_hierarchical(KINFO *vec, int veclen, int (*cmp)(const void *, const void *))
{
 int i;

 kivec = vec;
 nkiv = veclen;
 basecmp = cmp;
 pidx = malloc(nkiv*sizeof(int));
 ppidx = malloc(nkiv*sizeof(int));
 nodes = malloc(nkiv*sizeof(NODE));
 kidvec = malloc(nkiv*sizeof(int));
 for (i=nkiv-1;i>=0;i--)
  { pidx[i] = i;
    ppidx[i] = i;
    nodes[i].child = 0;
    kivec[i].indent = -1;
  }
 qsort(pidx,nkiv,sizeof(int),&cmp_pid);
 qsort(ppidx,nkiv,sizeof(int),&cmp_ppid);
 roots = 0;
 buildtree(&roots,0,-1);
 check_orphans();
 gen_order();
 qsort(kivec,nkiv,sizeof(KINFO),&cmp_order);
 free(pidx);
 free(ppidx);
 free(nodes);
 free(kidvec);
}
