#include <ctype.h>
#include <stdarg.h>

#include "include.h"

typedef enum {
	  /* not an expression at all */
	  XOP_NONE = 1,
	  XOP_ERR,
	  /* nilary */
	  XOP_N_CONST,
	  XOP_N_VAR,
	  /* unary */
	  XOP_U_PAREN,
	  XOP_U_NEG,
	  /* binary */
	  XOP_B_ADD,
	  XOP_B_SUB,
	  XOP_B_MUL,
	  XOP_B_FDIV,
	  XOP_B_RDIV,
	  XOP_B_CDIV,
	  XOP_B_ZDIV,
	  XOP_B_IDIV,
	  XOP_B_EQ,
	  XOP_B_NE,
	  XOP_B_LT,
	  XOP_B_GE,
	  XOP_B_GT,
	  XOP_B_LE,
	  /* ternary */
	  XOP_T_COND,
	  } XOP;

typedef enum {
	  XVAR_NONE = 1,
	  XVAR_LEVEL,
	  XVAR_CIRCLE,
	  XVAR_ENERGY,
	  XVAR_MAXENERGY,
	  XVAR_MANA,
	  XVAR_SPEED,
	  XVAR_QUICK,
	  XVAR_MIGHT,
	  XVAR_STRENGTH,
	  XVAR_GOLD,
	  XVAR_GEMS,
	  } XVAR;

typedef enum {
	  SCHK_NONE = 1,
	  SCHK_CHAR,
	  SCHK_EXPR,
	  } SCHK;

typedef enum {
	  ASSOC_LEFT = 1,
	  ASSOC_RIGHT,
	  ASSOC_NON,
	  } ASSOC;

typedef struct expr EXPR;
typedef struct sch SCH;
typedef struct eb EB;
typedef struct edit EDIT;
typedef struct editops EDITOPS;
typedef struct val VAL;

struct val {
  int err;
  int val;
  } ;
#define VAL_ERR() ((VAL){.err=1,.val=0})
#define VAL_NUM(n) ((VAL){.err=0,.val=(n)})

struct editops {
  void (*dstart)(int);
  void (*disp)(void *, int, int);
  void (*ddone)(int, int);
  void (*instr)(void);
  void (*cmd)(EDIT *, char);
  int (*typein)(void *, char);
  } ;
#define EDIT_OPS(name) {\
	&name##_dstart,		\
	&name##_disp,		\
	&name##_ddone,		\
	&name##_instr,		\
	&name##_cmd,		\
	&name##_typein }

struct edit {
  EDITOPS *ops;
  EB *eb;
  int curs;
  int xoff;
  } ;

struct eb {
  int esz;
  char *b;
  int a;
  int l;
  } ;

struct expr {
  XOP op;
  union {
    struct {
      int nat;
      int *at;
      int px;
      char *msg;
      } err;
    unsigned int val;
    XVAR var;
    struct {
      EXPR *arg;
      } u;
    struct {
      EXPR *lhs;
      EXPR *rhs;
      } b;
    struct {
      EXPR *lhs;
      EXPR *mhs;
      EXPR *rhs;
      } t;
    } u;
  } ;

struct sch {
  SCHK kind;
  union {
    char c;
    struct {
      char *text;
      int curs;
      EXPR *e;
      } x;
    } u;
  } ;

static int didinit = 0;
static EB db;
static int lspc;
static int lshift;
static int lmargin;
static EB ob;
static EB xb;
static EDIT edit_main;
static EDIT edit_expr;
static const char *parse_str;
static const char *parse_str0;
static int xedit_curs_nnote;
static int *xedit_curs_notev;
static int *xedit_curs_inxv;
static int xedit_curs_col;
static int xedit_pcol;
static const char *xedit_operation;
static SCH *main_expr;
static int main_ccol;

#define Cisspace(x) isspace((unsigned char)(x))

static void eb_init(EB *eb, int sz)
{
 eb->esz = sz;
 eb->b = 0;
 eb->a = 0;
 eb->l = 0;
}

static void eb_done(EB *eb)
{
 free(eb->b);
 eb->b = 0;
}

static void eb_clear(EB *eb)
{
 eb->l = 0;
}

static void eb_grow(EB *eb, int by)
{
 if (eb->l+by > eb->a) eb->b = realloc(eb->b,(eb->a=eb->l+by+8)*eb->esz);
}

static void *eb_ptr(EB *eb, int x)
{
 if ((x < 0) || (x >= eb->l)) abort();
 return(eb->b+(x*eb->esz));
}

static int eb_len(EB *eb)
{
 return(eb->l);
}

static void *eb_insert_at(EB *eb, int x)
{
 if ((x < 0) || (x > eb->l)) abort();
 eb_grow(eb,1);
 if (x < eb->l) bcopy(eb->b+(x*eb->esz),eb->b+((x+1)*eb->esz),(eb->l-x)*eb->esz);
 eb->l ++;
 return(eb->b+(x*eb->esz));
}

static void *eb_append1(EB *eb)
{
 eb_grow(eb,1);
 return(eb->b+(eb->l++*eb->esz));
}

static void eb_delete_at(EB *eb, int x)
{
 if ((x < 0) || (x >= eb->l)) abort();
 if (x < eb->l) bcopy(eb->b+((x+1)*eb->esz),eb->b+(x*eb->esz),(eb->l-x)*eb->esz);
 eb->l --;
}

static void eb_nulterm(EB *eb)
{
 eb_grow(eb,1);
 eb->b[eb->l] = '\0';
}

static void edit_init(EDIT *e, EB *eb, EDITOPS *ops)
{
 e->ops = ops;
 e->eb = eb;
 e->curs = 0;
 e->xoff = 0;
}

static void edit_update(EDIT *e)
{
 int i;
 int l;
 int c;
 int o;

 l = eb_len(e->eb);
 c = e->curs;
 o = e->xoff;
 if (c > l) c = l; else if (c < 0) c = 0;
 if (c-o > lspc-lmargin)
  { o = c - (lspc - lshift);
  }
 else if (c < o+lmargin)
  { o = c - lshift;
  }
 if (o < 0) o = 0;
 e->curs = c;
 e->xoff = o;
 (*e->ops->dstart)(o>0);
 for (i=0;i<lspc;i++)
  { if (o+i >= l) break;
    (*e->ops->disp)(eb_ptr(e->eb,o+i),o+i,c);
  }
 (*e->ops->ddone)(o+i<l,c-o);
}

static void edit_run(EDIT *e)
{
 int c;

 while (1)
  { (*e->ops->instr)();
    edit_update(e);
    refresh();
    c = getch();
    switch ((unsigned char)c)
     { case 0x01: /* ^A */
	  e->curs = 0;
	  break;
       case 0x02: /* ^B */
	  e->curs --;
	  break;
       case 0x05: /* ^E */
	  e->curs = eb_len(e->eb);
	  break;
       case 0x06: /* ^F */
	  e->curs ++;
	  break;
       case 0x0a: /* ^J */
       case 0x0d: /* ^M */
	  return;
	  break;
       case 0x0c: /* ^L */
	  clearok(stdscr,TRUE);
	  break;
       case 0x20 ... 0x7e:
       case 0xa0 ... 0xff:
	  if ((*e->ops->typein)(eb_insert_at(e->eb,e->curs),c))
	   { e->curs ++;
	   }
	  else
	   { eb_delete_at(e->eb,c);
	   }
	  break;
       default:
	  (*e->ops->cmd)(e,c);
	  break;
     }
  }
}

static int edit_curs(EDIT *e)
{
 return(e->curs);
}

static int edit_len(EDIT *e)
{
 return(eb_len(e->eb));
}

static void *edit_ptr(EDIT *e, int x)
{
 if ((x < 0) || (x >= eb_len(e->eb))) abort();
 return(eb_ptr(e->eb,x));
}

static void edit_delete_at(EDIT *e, int x)
{
 if ((x < 0) || (x >= eb_len(e->eb))) abort();
 eb_delete_at(e->eb,x);
}

static void *edit_insert_at(EDIT *e, int x)
{
 if ((x < 0) || (x > eb_len(e->eb))) abort();
 return(eb_insert_at(e->eb,x));
}

static void edit_setcurs(EDIT *e, int x)
{
 if ((x < 0) || (x > eb_len(e->eb))) abort();
 e->curs = x;
}

static EXPR *new_expr(void)
{
 EXPR *e;

 e = malloc(sizeof(EXPR));
 e->op = XOP_NONE;
 return(e);
}

static EXPR *expr_err_multiple(int, int *, int, const char *, ...)
	__attribute__((__format__(__printf__,4,5)));
static EXPR *expr_err_multiple(int nat, int *atv, int px, const char *fmt, ...)
{
 va_list ap;
 char *s;
 EXPR *e;

 if ((px < 0) || (px >= nat)) abort();
 va_start(ap,fmt);
 vasprintf(&s,fmt,ap);
 va_end(ap);
 e = new_expr();
 e->op = XOP_ERR;
 e->u.err.nat = nat;
 e->u.err.at = atv;
 e->u.err.px = px;
 e->u.err.msg = s;
 return(e);
}

static EXPR *expr_err_simple(const char *, ...)
	__attribute__((__format__(__printf__,1,2)));
static EXPR *expr_err_simple(const char *fmt, ...)
{
 va_list ap;
 char *s;
 EXPR *e;

 va_start(ap,fmt);
 vasprintf(&s,fmt,ap);
 va_end(ap);
 e = new_expr();
 e->op = XOP_ERR;
 e->u.err.nat = 1;
 e->u.err.at = malloc(sizeof(int));
 e->u.err.at[0] = parse_str - parse_str0;
 e->u.err.px = 0;
 e->u.err.msg = s;
 return(e);
}

static void expr_free(EXPR *e)
{
 if (! e) return;
 switch (e->op)
  { case XOP_NONE:
       break;
    case XOP_ERR:
       free(e->u.err.at);
       free(e->u.err.msg);
       break;
    case XOP_N_CONST:
    case XOP_N_VAR:
       break;
    case XOP_U_PAREN:
    case XOP_U_NEG:
       expr_free(e->u.u.arg);
       break;
    case XOP_B_ADD:
    case XOP_B_SUB:
    case XOP_B_MUL:
    case XOP_B_FDIV:
    case XOP_B_RDIV:
    case XOP_B_CDIV:
    case XOP_B_ZDIV:
    case XOP_B_IDIV:
    case XOP_B_EQ:
    case XOP_B_NE:
    case XOP_B_LT:
    case XOP_B_GE:
    case XOP_B_GT:
    case XOP_B_LE:
       expr_free(e->u.b.lhs);
       expr_free(e->u.b.rhs);
       break;
    case XOP_T_COND:
       expr_free(e->u.t.lhs);
       expr_free(e->u.t.mhs);
       expr_free(e->u.t.rhs);
       break;
  }
 free(e);
}

static EXPR *expr_clone(EXPR *e)
{
 EXPR *n;

 if (! e) return(0);
 n = new_expr();
 n->op = e->op;
 switch (e->op)
  { case XOP_NONE:
       break;
    case XOP_ERR:
       n->u.err.nat = e->u.err.nat;
       n->u.err.at = malloc(e->u.err.nat*sizeof(int));
       bcopy(e->u.err.at,n->u.err.at,e->u.err.nat*sizeof(int));
       n->u.err.msg = strdup(e->u.err.msg);
       break;
    case XOP_N_CONST:
       n->u.val = e->u.val;
       break;
    case XOP_N_VAR:
       n->u.var = e->u.var;
       break;
    case XOP_U_PAREN:
    case XOP_U_NEG:
       n->u.u.arg = expr_clone(e->u.u.arg);
       break;
    case XOP_B_ADD:
    case XOP_B_SUB:
    case XOP_B_MUL:
    case XOP_B_FDIV:
    case XOP_B_RDIV:
    case XOP_B_CDIV:
    case XOP_B_ZDIV:
    case XOP_B_IDIV:
    case XOP_B_EQ:
    case XOP_B_NE:
    case XOP_B_LT:
    case XOP_B_GE:
    case XOP_B_GT:
    case XOP_B_LE:
       n->u.b.lhs = expr_clone(e->u.b.lhs);
       n->u.b.rhs = expr_clone(e->u.b.rhs);
       break;
    case XOP_T_COND:
       n->u.t.lhs = expr_clone(e->u.t.lhs);
       n->u.t.mhs = expr_clone(e->u.t.mhs);
       n->u.t.rhs = expr_clone(e->u.t.rhs);
       break;
  }
 return(n);
}

static void skipwhite(void)
{
 while (*parse_str && Cisspace(*parse_str)) parse_str ++;
}

static EXPR *parse_top(void); /* forward */

static EXPR *parse_bin(EXPR *(*parse_sub)(void), ASSOC assoc, ...)
{
 typedef union {
	   EXPR *e;
	   struct {
	     XOP op;
	     int at;
	     } op;
	   } BQE;
 va_list ap;
 const char *s;
 XOP op;
 int l;
 EXPR *e;
 EXPR *n;
 EB eb;
 BQE *qe;
 int i;

 void free_exprs(void)
  { int i;
    for (i=eb_len(&eb)-1;i>=0;i--)
     { if (! (i & 1)) expr_free(((BQE *)eb_ptr(&eb,i))->e);
     }
  }

 eb_init(&eb,sizeof(BQE));
 while <"loop"> (1)
  { e = (*parse_sub)();
    if (e->op == XOP_ERR)
     { free_exprs();
       eb_done(&eb);
       return(e);
     }
    ((BQE *)eb_append1(&eb))->e = e;
    skipwhite();
    va_start(ap,assoc);
    while (1)
     { s = va_arg(ap,const char *);
       if (! s) break <"loop">;
       op = va_arg(ap,XOP);
       l = strlen(s);
       if (! strncmp(parse_str,s,l))
	{ qe = eb_append1(&eb);
	  qe->op.op = op;
	  qe->op.at = parse_str - parse_str0;
	  parse_str += l;
	  break;
	}
     }
  }
 if ((assoc == ASSOC_NON) && (eb_len(&eb) > 3))
  { int nat;
    int *atv;
    free_exprs();
    nat = eb_len(&eb) / 2;
    atv = malloc(nat*sizeof(int));
    for (i=nat-1;i>=0;i--) atv[i] = ((BQE *)eb_ptr(&eb,i+i+1))->op.at;
    eb_done(&eb);
    return(expr_err_multiple(nat,atv,0,"Non-associative operators used associatively"));
  }
 l = eb_len(&eb) / 2;
 if (assoc == ASSOC_LEFT)
  { e = ((BQE *)eb_ptr(&eb,0))->e;
    for (i=0;i<l;i++)
     { n = new_expr();
       n->op = ((BQE *)eb_ptr(&eb,i+i+1))->op.op;
       n->u.b.lhs = e;
       n->u.b.rhs = ((BQE *)eb_ptr(&eb,i+i+2))->e;
       e = n;
     }
  }
 else
  { e = ((BQE *)eb_ptr(&eb,l+l))->e;
    for (i=l-1;i>=0;i--)
     { n = new_expr();
       n->op = ((BQE *)eb_ptr(&eb,i+i+1))->op.op;
       n->u.b.lhs = ((BQE *)eb_ptr(&eb,i+i))->e;
       n->u.b.rhs = e;
       e = n;
     }
  }
 eb_done(&eb);
 return(e);
}

static EXPR *parse_unary(void)
{
 EXPR *sub;
 char *endp;
 long int val;
 XVAR var;
 int adv;
 EXPR *e;

 skipwhite();
 if (*parse_str == '-')
  { parse_str ++;
    sub = parse_unary();
    if (sub->op == XOP_ERR) return(sub);
    e = new_expr();
    e->op = XOP_U_NEG;
    e->u.u.arg = sub;
    return(e);
  }
 if (*parse_str == '(')
  { int openat;
    openat = parse_str - parse_str0;
    parse_str ++;
    sub = parse_top();
    if (sub->op == XOP_ERR) return(sub);
    skipwhite();
    if (*parse_str == ')')
     { parse_str ++;
       e = new_expr();
       e->op = XOP_U_PAREN;
       e->u.u.arg = sub;
     }
    else
     { int *atv;
       expr_free(sub);
       atv = malloc(2*sizeof(int));
       atv[0] = openat;
       atv[1] = parse_str - parse_str0;
       e = expr_err_multiple(2,atv,1,"Improperly closed (");
     }
    return(e);
  }
 val = strtol(parse_str,&endp,0);
 if (endp != parse_str)
  { parse_str = endp;
    e = new_expr();
    e->op = XOP_N_CONST;
    e->u.val = val;
    return(e);
  }
 var = XVAR_NONE;
 switch (*parse_str)
  { case 'c':
       if (!strncmp(parse_str,"circle",6)) { var = XVAR_CIRCLE; adv = 6; }
       break;
    case 'e':
       if (!strncmp(parse_str,"energy",6)) { var = XVAR_ENERGY; adv = 6; }
       break;
    case 'g':
       if (!strncmp(parse_str,"gold",4)) { var = XVAR_GOLD; adv = 4; }
       if (!strncmp(parse_str,"gems",4)) { var = XVAR_GEMS; adv = 4; }
       break;
    case 'l':
       if (!strncmp(parse_str,"level",5)) { var = XVAR_LEVEL; adv = 5; }
       break;
    case 'm':
       if (!strncmp(parse_str,"mana",4)) { var = XVAR_MANA; adv = 4; }
       if (!strncmp(parse_str,"maxenergy",9)) { var = XVAR_MAXENERGY; adv = 9; }
       if (!strncmp(parse_str,"might",5)) { var = XVAR_MIGHT; adv = 5; }
       break;
    case 'q':
       if (!strncmp(parse_str,"quick",5)) { var = XVAR_QUICK; adv = 5; }
       break;
    case 's':
       if (!strncmp(parse_str,"speed",5)) { var = XVAR_SPEED; adv = 5; }
       if (!strncmp(parse_str,"strength",8)) { var = XVAR_STRENGTH; adv = 8; }
       break;
  }
 if (var == XVAR_NONE) return(expr_err_simple("Unrecognized primitive"));
 parse_str += adv;
 e = new_expr();
 e->op = XOP_N_VAR;
 e->u.var = var;
 return(e);
}

static EXPR *parse_muldiv(void)
{
 return(parse_bin(&parse_unary,ASSOC_LEFT,"*",XOP_B_MUL,"/",XOP_B_FDIV,"\\f",XOP_B_FDIV,"\\r",XOP_B_RDIV,"\\c",XOP_B_CDIV,"\\z",XOP_B_ZDIV,"\\i",XOP_B_IDIV,(const char *)0));
}

static EXPR *parse_addsub(void)
{
 return(parse_bin(&parse_muldiv,ASSOC_LEFT,"+",XOP_B_ADD,"-",XOP_B_SUB,(const char *)0));
}

static EXPR *parse_cmp(void)
{
 return(parse_bin(&parse_addsub,ASSOC_NON,"==",XOP_B_EQ,"!=",XOP_B_NE,"<=",XOP_B_LE,">=",XOP_B_GE,"<",XOP_B_LT,">",XOP_B_GT,(const char *)0));
}

static EXPR *parse_cond(void)
{
 EXPR *lhs;
 EXPR *mhs;
 EXPR *rhs;
 EXPR *e;

 lhs = parse_cmp();
 if (lhs->op == XOP_ERR) return(lhs);
 skipwhite();
 if (*parse_str != '?') return(lhs);
 parse_str ++;
 mhs = parse_cond();
 if (mhs->op == XOP_ERR)
  { expr_free(lhs);
    return(mhs);
  }
 skipwhite();
 if (*parse_str == ':')
  { parse_str ++;
    rhs = parse_cond();
    if (rhs->op == XOP_ERR)
     { expr_free(lhs);
       expr_free(mhs);
       return(rhs);
     }
    e = new_expr();
    e->op = XOP_T_COND;
    e->u.t.lhs = lhs;
    e->u.t.mhs = mhs;
    e->u.t.rhs = rhs;
  }
 else
  { expr_free(lhs);
    expr_free(mhs);
    e = expr_err_simple("Malformed ? : (missing :)");
  }
 return(e);
}

static EXPR *parse_top(void)
{
 return(parse_cond());
}

static EXPR *expr_parse(const char *s)
{
 parse_str = s;
 parse_str0 = s;
 skipwhite();
 if (! *parse_str) return(expr_err_simple("Empty expression"));
 return(parse_top());
}

#define DIVARGS int q, int r, int rhs __attribute__((__unused__))

static int div_f(DIVARGS)
{
 if (r < 0) q --;
 return(q);
}

static int div_r(DIVARGS)
{
 if (r < 0)
  { r += r;
    q --;
  }
 if (r*2 > rhs) q ++; else if ((r*2 == rhs) && (q & 1)) q ++;
 return(q);
}

static int div_c(DIVARGS)
{
 if (r > 0) q ++;
 return(q);
}

static int div_z(DIVARGS)
{
 if (r < 0)
  { if (q > 0) q --;
  }
 else if (r > 0)
  { if (q < 0) q ++;
  }
 return(q);
}

static int div_i(DIVARGS)
{
 if (r < 0)
  { if (q > 0) q ++;
  }
 else if (r > 0)
  { if (q < 0) q --;
  }
 return(q);
}

#undef DIVARGS

static VAL expr_eval(EXPR *e)
{
 VAL lhs;
 VAL rhs;

 switch (e->op)
  { case XOP_NONE:
       abort();
       break;
    case XOP_ERR:
       return(VAL_ERR());
       break;
    case XOP_N_CONST:
       return(VAL_NUM(e->u.val));
       break;
    case XOP_N_VAR:
       switch (e->u.var)
	{ case XVAR_LEVEL: return(VAL_NUM(Player.p_level)); break;
	  case XVAR_CIRCLE: return(VAL_NUM(Circle)); break;
	  case XVAR_ENERGY: return(VAL_NUM(Player.p_energy)); break;
	  case XVAR_MAXENERGY: return(VAL_NUM(Player.p_maxenergy+Player.p_shield)); break;
	  case XVAR_MANA: return(VAL_NUM(Player.p_mana)); break;
	  case XVAR_SPEED: return(VAL_NUM(Player.p_speed)); break;
	  case XVAR_QUICK: return(VAL_NUM(Player.p_quickness+Player.p_quksilver)); break;
	  case XVAR_MIGHT: return(VAL_NUM(Player.p_might)); break;
	  case XVAR_STRENGTH: return(VAL_NUM(Player.p_strength+Player.p_sword)); break;
	  case XVAR_GOLD: return(VAL_NUM(Player.p_gold)); break;
	  case XVAR_GEMS: return(VAL_NUM(Player.p_gems)); break;
	  default: abort(); break;
	}
       break;
    case XOP_U_PAREN:
       return(expr_eval(e->u.u.arg));
       break;
    case XOP_U_NEG:
       lhs = expr_eval(e->u.u.arg);
       if (! lhs.err) lhs.val = - lhs.val;
       return(lhs);
       break;
    case XOP_B_ADD:
    case XOP_B_SUB:
    case XOP_B_MUL:
    case XOP_B_FDIV:
    case XOP_B_RDIV:
    case XOP_B_CDIV:
    case XOP_B_ZDIV:
    case XOP_B_IDIV:
    case XOP_B_EQ:
    case XOP_B_NE:
    case XOP_B_LT:
    case XOP_B_GE:
    case XOP_B_GT:
    case XOP_B_LE:
       lhs = expr_eval(e->u.b.lhs);
       if (lhs.err) return(lhs);
       rhs = expr_eval(e->u.b.rhs);
       if (rhs.err) return(rhs);
       switch (e->op)
	{ case XOP_B_ADD: return(VAL_NUM(lhs.val+rhs.val)); break;
	  case XOP_B_SUB: return(VAL_NUM(lhs.val-rhs.val)); break;
	  case XOP_B_MUL: return(VAL_NUM(lhs.val*rhs.val)); break;
	  case XOP_B_FDIV:
	  case XOP_B_RDIV:
	  case XOP_B_CDIV:
	  case XOP_B_ZDIV:
	  case XOP_B_IDIV:
	      { int q;
		int r;
		if (rhs.val == 0) return(VAL_ERR());
		if (rhs.val < 0)
		 { rhs.val = - rhs.val;
		   lhs.val = - lhs.val;
		 }
		q = lhs.val / rhs.val;
		r = lhs.val - (q * rhs.val);
		switch (e->op)
		 { case XOP_B_FDIV: return(VAL_NUM(div_f(q,r,rhs.val))); break;
		   case XOP_B_RDIV: return(VAL_NUM(div_r(q,r,rhs.val))); break;
		   case XOP_B_CDIV: return(VAL_NUM(div_c(q,r,rhs.val))); break;
		   case XOP_B_ZDIV: return(VAL_NUM(div_z(q,r,rhs.val))); break;
		   case XOP_B_IDIV: return(VAL_NUM(div_i(q,r,rhs.val))); break;
		   default: abort(); break;
		 }
	      }
	     break;
	  case XOP_B_EQ: return(VAL_NUM(lhs.val==rhs.val)); break;
	  case XOP_B_NE: return(VAL_NUM(lhs.val!=rhs.val)); break;
	  case XOP_B_LT: return(VAL_NUM(lhs.val<rhs.val));  break;
	  case XOP_B_GE: return(VAL_NUM(lhs.val>=rhs.val)); break;
	  case XOP_B_GT: return(VAL_NUM(lhs.val>rhs.val));  break;
	  case XOP_B_LE: return(VAL_NUM(lhs.val<=rhs.val)); break;
	  default: abort(); break;
	}
       break;
    case XOP_T_COND:
       lhs = expr_eval(e->u.t.lhs);
       if (lhs.err) return(lhs);
       return(expr_eval(lhs.val?e->u.t.mhs:e->u.t.rhs));
       break;
    default:
       abort();
       break;
  }
}

static void parseerr_instr(void)
{
 int i;

 mvprintw(0,0,"Parse error");
 clrtoeol();
 move(1,0);
 clrtoeol();
 mvprintw(2,0,"a - abort %s",xedit_operation);
 clrtoeol();
 mvprintw(3,0,"r - re-edit");
 clrtoeol();
 for (i=4;i<LINES-4;i++)
  { move(i,0);
    clrtoeol();
  }
}

static void xedit_load(const char *s, int curs)
{
 int i;

 edit_setcurs(&edit_expr,0);
 eb_clear(&xb);
 eb_grow(&xb,strlen(s));
 for (i=0;s[i];i++) *(char *)eb_append1(&xb) = s[i];
 edit_setcurs(&edit_expr,curs);
}

static int xedit_edit(SCH *sch)
{
 EXPR *x;
 int i;
 int cmd;

 while <"edit"> (1)
  { edit_run(&edit_expr);
    eb_nulterm(&xb);
    if (eb_len(&xb) > 0)
     { x = expr_parse(eb_ptr(&xb,0));
     }
    else
     { x = expr_parse("");
     }
    if (x->op == XOP_ERR)
     { while (1)
	{ move(LINES-4,0);
	  addstr(x->u.err.msg);
	  edit_setcurs(&edit_expr,x->u.err.at[x->u.err.px]);
	  xedit_curs_nnote = x->u.err.nat;
	  xedit_curs_notev = x->u.err.at;
	  xedit_curs_inxv = malloc(x->u.err.nat*sizeof(int));
	  for (i=xedit_curs_nnote-1;i>=0;i--) xedit_curs_inxv[i] = -1;
	  edit_update(&edit_expr);
	  for (i=xedit_curs_nnote-1;i>=0;i--)
	   { if (xedit_curs_inxv[i] >= 0)
	      { mvaddch(LINES-3,xedit_curs_inxv[i],'v');
	      }
	   }
	  parseerr_instr();
	  move(LINES-2,xedit_pcol);
	  refresh();
	  cmd = getch();
	  switch (cmd)
	   { case 'a':
		expr_free(x);
		return(1);
		break;
	     case 'r':
		expr_free(x);
		continue <"edit">;
		break;
	     default:
		beep();
		break;
	   }
	}
     }
    else
     { free(sch->u.x.text);
       expr_free(sch->u.x.e);
       sch->u.x.text = strdup(eb_ptr(&xb,0));
       sch->u.x.curs = edit_curs(&edit_expr);
       sch->u.x.e = x;
       return(0);
     }
  }
}

static void insert_expr(EDIT *e, int at)
{
 SCH *sch;

 sch = edit_insert_at(e,at);
 sch->kind = SCHK_EXPR;
 sch->u.x.text = 0;
 sch->u.x.curs = 0;
 sch->u.x.e = 0;
 edit_update(e);
 xedit_load("",0);
 xedit_operation = "insert";
 if (xedit_edit(sch)) edit_delete_at(e,at);
}

static void frobx_instr(void)
{
 int i;

 mvprintw(0,0,"d - delete");
 clrtoeol();
 mvprintw(1,0,"e - edit");
 clrtoeol();
 mvprintw(2,0,"c - clone");
 clrtoeol();
 for (i=3;i<LINES-2;i++)
  { move(i,0);
    clrtoeol();
  }
}

static void frob_expr(EDIT *e, int x)
{
 int cmd;
 SCH *sch;
 SCH *sch2;

 sch = (SCH *) edit_ptr(e,x);
 while (1)
  { frobx_instr();
    move(LINES-1,main_ccol);
    refresh();
    cmd = getch();
    switch (cmd)
     { case 'c':
	  sch2 = edit_insert_at(e,x+1);
	  sch2->kind = SCHK_EXPR;
	  sch2->u.x.text = strdup(sch->u.x.text);
	  sch2->u.x.curs = sch->u.x.curs;
	  sch2->u.x.e = expr_clone(sch->u.x.e);
	  return;
	  break;
       case 'd':
	  free(sch->u.x.text);
	  expr_free(sch->u.x.e);
	  edit_delete_at(e,x);
	  return;
	  break;
       case 'e':
	  xedit_load(sch->u.x.text,sch->u.x.curs);
	  xedit_edit(sch);
	  return;
	  break;
       default:
	  beep();
	  break;
     }
  }
}

static void main_dstart(int notb)
{
 move(LINES-1,0);
 addch(notb?'<':' ');
 main_expr = 0;
}

static void main_disp(void *schv, int inx, int cursinx)
{
 SCH *s;

 s = schv;
 switch (s->kind)
  { case SCHK_CHAR:
       addch(s->u.c);
       break;
    case SCHK_EXPR:
       standout();
       addch('*');
       standend();
       if (inx == cursinx) main_expr = s;
       break;
    default:
       abort();
       break;
  }
}

static void main_ddone(int note, int cx)
{
 if (note) addch('>');
 clrtoeol();
 move(LINES-2,0);
 if (main_expr && main_expr->u.x.text)
  { xedit_load(main_expr->u.x.text,main_expr->u.x.curs);
    edit_update(&edit_expr);
  }
 else
  { clrtoeol();
  }
 main_ccol = 1 + cx;
 move(LINES-1,main_ccol);
}

static void main_instr(void)
{
 int i;

 mvprintw(0,0,"^A ^B ^E ^F - move, ^D ^H - delete, ^T - swap, RETURN - done, type - insert");
 clrtoeol();
 mvprintw(1,0,"^I - insert expression, ^X - manipulate existing expression");
 clrtoeol();
 for (i=2;i<LINES-2;i++)
  { move(i,0);
    clrtoeol();
  }
}

static void main_cmd(EDIT *e, char ch)
{
 int c;
 int l;

 c = edit_curs(e);
 l = edit_len(e);
 switch ((unsigned char)ch)
  { case 0x04: /* ^D */
       if ((c < l) && (((SCH *)edit_ptr(e,c))->kind == SCHK_CHAR)) edit_delete_at(e,c);
       break;
    case 0x08: /* ^H */
    case 0x7f: /* DEL */
       if ((c > 0) && (((SCH *)edit_ptr(e,c-1))->kind == SCHK_CHAR))
	{ edit_delete_at(e,c-1);
	  edit_setcurs(e,c-1);
	}
       break;
    case 0x09: /* ^I */
       insert_expr(e,c);
       break;
    case 0x14: /* ^T */
       if (c >= 2)
	{ SCH t;
	  t = *(SCH *)edit_ptr(e,c-2);
	  *(SCH *)edit_ptr(e,c-2) = *(SCH *)edit_ptr(e,c-1);
	  *(SCH *)edit_ptr(e,c-1) = t;
	}
       break;
    case 0x18: /* ^X */
       if ((c < l) && (((SCH *)edit_ptr(e,c))->kind == SCHK_EXPR)) frob_expr(e,c);
       break;
    default:
       beep();
       break;
  }
}

static int main_typein(void *schv, char c)
{
 *(SCH *)schv = (SCH) { .kind = SCHK_CHAR, .u = { .c = c } };
 return(1);
}

static EDITOPS main_ops = EDIT_OPS(main);

static void xedit_dstart(int notb)
{
 move(LINES-2,0);
 addch(notb?'<':' ');
 xedit_curs_col = 1;
}

static void xedit_disp(void *cv, int inx, int cursinx __attribute__((__unused__)))
{
 int i;

 for (i=xedit_curs_nnote-1;i>=0;i--)
  { if (inx == xedit_curs_notev[i]) xedit_curs_inxv[i] = xedit_curs_col;
  }
 addch(*(char *)cv);
 xedit_curs_col ++;
}

static void xedit_ddone(int note, int cx)
{
 if (note) addch('>');
 clrtoeol();
 xedit_pcol = 1 + cx;
 move(LINES-2,1+cx);
}

static void xedit_instr(void)
{
 int i;

 mvprintw(0,0,"^A ^B ^E ^F - move, ^D ^H - delete, ^T - swap, RETURN - done, type - insert");
 clrtoeol();
 mvprintw(1,0,"Vars: level circle energy maxenergy mana speed quick might strength gold");
 clrtoeol();
 for (i=2;i<LINES-2;i++)
  { move(i,0);
    clrtoeol();
  }
}

static void xedit_cmd(EDIT *e, char ch)
{
 int c;
 int l;

 c = edit_curs(e);
 l = edit_len(e);
 switch ((unsigned char)ch)
  { case 0x04: /* ^D */
       if (c < l) edit_delete_at(e,c);
       break;
    case 0x08: /* ^H */
    case 0x7f: /* DEL */
       if (c > 0)
	{ edit_delete_at(e,c-1);
	  edit_setcurs(e,c-1);
	}
       break;
    case 0x14: /* ^T */
       if (c >= 2)
	{ char t;
	  t = *(char *)edit_ptr(e,c-2);
	  *(char *)edit_ptr(e,c-2) = *(char *)edit_ptr(e,c-1);
	  *(char *)edit_ptr(e,c-1) = t;
	}
       break;
    default:
       beep();
       break;
  }
}

static int xedit_typein(void *cv, char c)
{
 *(char *)cv = c;
 return(1);
}

static EDITOPS xedit_ops = EDIT_OPS(xedit);

static void maybeinit(void)
{
 if (didinit) return;
 didinit = 1;
 eb_init(&db,sizeof(SCH));
 eb_init(&ob,1);
 eb_init(&xb,1);
 edit_init(&edit_main,&db,&main_ops);
 edit_init(&edit_expr,&xb,&xedit_ops);
 xedit_curs_nnote = 0;
}

void changedisplays(void)
{
 maybeinit();
 clear();
 lspc = COLS - 3;
 lshift = lspc / 3;
 lmargin = lspc / 8;
 leaveok(stdscr,FALSE);
 edit_run(&edit_main);
 clear();
}

static void ob_char(char c)
{
 *(char *)eb_append1(&ob) = c;
}

const char *displaystring(void)
{
 int i;
 int l;
 VAL v;
 SCH *sp;

 maybeinit();
 eb_clear(&ob);
 l = eb_len(&db);
 for (i=0;i<l;i++)
  { sp = (SCH *) eb_ptr(&db,(i));
    switch (sp->kind)
     { case SCHK_CHAR:
	  ob_char(sp->u.c);
	  break;
       case SCHK_EXPR:
	  v = expr_eval(sp->u.x.e);
	  if (v.err)
	   { ob_char('?');
	   }
	  else
	   { char *t;
	     int x;
	     asprintf(&t,"%d",v.val);
	     for (x=0;t[x];x++) ob_char(t[x]);
	     free(t);
	   }
	  break;
       default:
	  abort();
	  break;
     }
  }
 ob_char('\0');
 return(eb_ptr(&ob,0));
}
