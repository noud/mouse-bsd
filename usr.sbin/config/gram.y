%{
/*	$NetBSD: gram.y,v 1.28 2000/01/23 23:37:42 hubertf Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)gram.y	8.1 (Berkeley) 6/6/93
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "sem.h"

#define	FORMAT(n) ((n) > -10 && (n) < 10 ? "%d" : "0x%x")

#define	stop(s)	error(s), exit(1)

static	struct	config conf;	/* at most one active at a time */

/* the following is used to recover nvlist space after errors */
static	struct	nvlist *alloc[1000];
static	int	adepth;
#define	new0(n,s,p,i,x)	(alloc[adepth++] = newnv(n, s, p, i, x))
#define	new_n(n)	new0(n, NULL, NULL, 0, NULL)
#define	new_nx(n, x)	new0(n, NULL, NULL, 0, x)
#define	new_ns(n, s)	new0(n, s, NULL, 0, NULL)
#define	new_si(s, i)	new0(NULL, s, NULL, i, NULL)
#define	new_nsi(n,s,i)	new0(n, s, NULL, i, NULL)
#define	new_np(n, p)	new0(n, NULL, p, 0, NULL)
#define	new_s(s)	new0(NULL, s, NULL, 0, NULL)
#define	new_p(p)	new0(NULL, NULL, p, 0, NULL)
#define	new_px(p, x)	new0(NULL, NULL, p, 0, x)
#define	new_sx(s, x)	new0(NULL, s, NULL, 0, x)

#define	fx_atom(s)	new0(s, NULL, NULL, FX_ATOM, NULL)
#define	fx_not(e)	new0(NULL, NULL, NULL, FX_NOT, e)
#define	fx_and(e1, e2)	new0(NULL, NULL, e1, FX_AND, e2)
#define	fx_or(e1, e2)	new0(NULL, NULL, e1, FX_OR, e2)

static	void	cleanup __P((void));
static	void	setmachine __P((const char *, const char *));
static	void	check_maxpart __P((void));

static	void	app __P((struct nvlist *, struct nvlist *));

static	struct nvlist *mk_nsis __P((const char *, int, struct nvlist *, int));
static	struct nvlist *mk_ns __P((const char *, struct nvlist *));

%}

%union {
	struct	attr *attr;
	struct	devbase *devb;
	struct	deva *deva;
	struct	nvlist *list;
	const char *str;
	int	val;
}

%token	AND AT ATTACH BUILD CINCLUDE COMPILE_WITH CONFIG DEFFS DEFINE DEFOPT
%token	DEFPARAM DEFFLAG DEFPSEUDO DEVICE DEVCLASS DUMPS ENDFILE XFILE XOBJECT
%token	FILE_SYSTEM FLAGS IDENT INCLUDE XMACHINE MAJOR MAKEOPTIONS
%token	MAXUSERS MAXPARTITIONS MINOR ON OPTIONS PREFIX PSEUDO_DEVICE ROOT
%token	SOURCE TYPE WITH NEEDS_COUNT NEEDS_FLAG
%token	<val> NUMBER
%token	<str> PATHNAME WORD EMPTY

%left '|'
%left '&'

%type	<list>	fopts fexpr fatom
%type	<str>	fs_spec
%type	<val>	fflgs fflag oflgs oflag
%type	<str>	rule
%type	<attr>	attr
%type	<devb>	devbase
%type	<deva>	devattach_opt
%type	<list>	atlist interface_opt
%type	<str>	atname
%type	<list>	loclist_opt loclist locdef
%type	<str>	locdefault
%type	<list>	values locdefaults
%type	<list>	attrs_opt attrs
%type	<list>	locators locator
%type	<list>	dev_spec
%type	<str>	device_instance
%type	<str>	attachment
%type	<str>	value
%type	<val>	major_minor signed_number npseudo
%type	<val>	flags_opt
%type	<str>	deffs
%type	<list>	deffses
%type	<str>	fsoptfile_opt
%type	<str>	defopt
%type	<list>	defopts
%type	<list>	defoptdeps
%type	<str>	optfile_opt

%%

/*
 * A configuration consists of a machine type, followed by the machine
 * definition files (via the include() mechanism), followed by the
 * configuration specification(s) proper.  In effect, this is two
 * separate grammars, with some shared terminals and nonterminals.
 * Note that we do not have sufficient keywords to enforce any order
 * between elements of "topthings" without introducing shift/reduce
 * conflicts.  Instead, check order requirements in the C code.
 */
Configuration:
	topthings			/* dirspecs, include "std.arch" */
	machine_spec			/* "machine foo" from machine descr. */
	dev_defs dev_eof		/* sys/conf/files */
	dev_defs dev_eof		/* sys/arch/${MACHINE_ARCH}/... */
	dev_defs dev_eof		/* sys/arch/${MACHINE}/... */
					{ check_maxpart(); }
	specs;				/* rest of machine description */

topthings:
	topthings topthing |
	/* empty */;

topthing:
	SOURCE PATHNAME '\n'		{ if (!srcdir) srcdir = $2; } |
	BUILD  PATHNAME '\n'		{ if (!builddir) builddir = $2; } |
	include '\n' |
	'\n';

machine_spec:
	XMACHINE WORD '\n'		{ setmachine($2,NULL); } |
	XMACHINE WORD WORD '\n' 	{ setmachine($2,$3); } |
	error { stop("cannot proceed without machine specifier"); };

dev_eof:
	ENDFILE				{ enddefs(); checkfiles(); };

/*
 * Various nonterminals shared between the grammars.
 */
file:
	XFILE PATHNAME fopts fflgs rule	{ addfile($2, $3, $4, $5); };

object:
	XOBJECT PATHNAME fopts oflgs	{ addobject($2, $3, $4); };

/* order of options is important, must use right recursion */
fopts:
	fexpr				{ $$ = $1; } |
	/* empty */			{ $$ = NULL; };

fexpr:
	fatom				{ $$ = $1; } |
	'!' fatom			{ $$ = fx_not($2); } |
	fexpr '&' fexpr			{ $$ = fx_and($1, $3); } |
	fexpr '|' fexpr			{ $$ = fx_or($1, $3); } |
	'(' fexpr ')'			{ $$ = $2; };

fatom:
	WORD				{ $$ = fx_atom($1); };

fflgs:
	fflgs fflag			{ $$ = $1 | $2; } |
	/* empty */			{ $$ = 0; };

fflag:
	NEEDS_COUNT			{ $$ = FI_NEEDSCOUNT; } |
	NEEDS_FLAG			{ $$ = FI_NEEDSFLAG; };

oflgs:
	oflgs oflag			{ $$ = $1 | $2; } |
	/* empty */			{ $$ = 0; };

oflag:
	NEEDS_FLAG			{ $$ = OI_NEEDSFLAG; };

rule:
	COMPILE_WITH WORD		{ $$ = $2; } |
	/* empty */			{ $$ = NULL; };

include:
	INCLUDE WORD			{ (void) include($2, 0, 0); } |
	CINCLUDE WORD			{ (void) include($2, 0, 1); };

prefix:
	PREFIX PATHNAME			{ prefix_push($2); } |
	PREFIX				{ prefix_pop(); };

/*
 * The machine definitions grammar.
 */
dev_defs:
	dev_defs dev_def |
	/* empty */;

dev_def:
	one_def '\n'			{ adepth = 0; } |
	'\n' |
	error '\n'			{ cleanup(); };

one_def:
	file |
	object |
	include |
	prefix |
	DEVCLASS WORD			{ (void)defattr($2, NULL, 1); } |
	DEFFS fsoptfile_opt deffses	{ deffilesystem($2, $3); } |
	DEFINE WORD interface_opt	{ (void)defattr($2, $3, 0); } |
	DEFOPT optfile_opt defopts defoptdeps
					{ defoption($2, $3, $4); } |
	DEFFLAG optfile_opt defopts defoptdeps
					{ defflag($2, $3, $4); } |
	DEFPARAM optfile_opt defopts defoptdeps
					{ defparam($2, $3, $4); } |
	DEVICE devbase interface_opt attrs_opt
					{ defdev($2, $3, $4, 0); } |
	ATTACH devbase AT atlist devattach_opt attrs_opt
					{ defdevattach($5, $2, $4, $6); } |
	MAXPARTITIONS NUMBER		{ maxpartitions = $2; } |
	MAXUSERS NUMBER NUMBER NUMBER	{ setdefmaxusers($2, $3, $4); } |
	DEFPSEUDO devbase attrs_opt { defdev($2, NULL, $3, 1); } |
	MAJOR '{' majorlist '}';

atlist:
	atlist ',' atname		{ $$ = new_nx($3, $1); } |
	atname				{ $$ = new_n($1); };

atname:
	WORD				{ $$ = $1; } |
	ROOT				{ $$ = NULL; };

deffses:
	deffses deffs			{ $$ = new_nx($2, $1); } |
	deffs				{ $$ = new_n($1); };

deffs:
	WORD				{ $$ = $1; };

defoptdeps:
	':' defopts			{ $$ = $2; } |
	/* empty */			{ $$ = NULL; };

defopts:
	defopts defopt			{ $$ = new_nx($2, $1); } |
	defopt				{ $$ = new_n($1); };

defopt:
	WORD				{ $$ = $1; };

devbase:
	WORD				{ $$ = getdevbase($1); };

devattach_opt:
	WITH WORD			{ $$ = getdevattach($2); } |
	/* empty */			{ $$ = NULL; };

interface_opt:
	'{' loclist_opt '}'		{ $$ = new_nx("", $2); } |
	/* empty */			{ $$ = NULL; };

loclist_opt:
	loclist				{ $$ = $1; } |
	/* empty */			{ $$ = NULL; };

/* loclist order matters, must use right recursion */
loclist:
	locdef ',' loclist		{ $$ = $1; app($1, $3); } |
	locdef				{ $$ = $1; };

/* "[ WORD locdefault ]" syntax may be unnecessary... */
locdef:
	WORD locdefault 		{ $$ = new_nsi($1, $2, 0); } |
	WORD				{ $$ = new_nsi($1, NULL, 0); } |
	'[' WORD locdefault ']'		{ $$ = new_nsi($2, $3, 1); } |
	WORD '[' NUMBER ']'		{ $$ = mk_nsis($1, $3, NULL, 0); } |
	WORD '[' NUMBER ']' locdefaults	{ $$ = mk_nsis($1, $3, $5, 0); } |
	'[' WORD '[' NUMBER ']' locdefaults ']'
					{ $$ = mk_nsis($2, $4, $6, 1); };

locdefault:
	'=' value			{ $$ = $2; };

locdefaults:
	'=' '{' values '}'		{ $$ = $3; };

fsoptfile_opt:
	PATHNAME			{ $$ = $1; } |
	/* empty */			{ $$ = NULL; };

optfile_opt:
	PATHNAME			{ $$ = $1; } |
	/* empty */			{ $$ = NULL; };

value:
	WORD				{ $$ = $1; } |
	EMPTY				{ $$ = $1; } |
	signed_number			{ char bf[40];
					    (void)sprintf(bf, FORMAT($1), $1);
					    $$ = intern(bf); };

values:
	value ',' values		{ $$ = new_sx($1, $3); } |
	value				{ $$ = new_s($1); };

signed_number:
	NUMBER				{ $$ = $1; } |
	'-' NUMBER			{ $$ = -$2; };

attrs_opt:
	':' attrs			{ $$ = $2; } |
	/* empty */			{ $$ = NULL; };

attrs:
	attrs ',' attr			{ $$ = new_px($3, $1); } |
	attr				{ $$ = new_p($1); };

attr:
	WORD				{ $$ = getattr($1); };

majorlist:
	majorlist ',' majordef |
	majordef;

majordef:
	devbase '=' NUMBER		{ setmajor($1, $3); };


/*
 * The configuration grammar.
 */
specs:
	specs spec |
	/* empty */;

spec:
	config_spec '\n'		{ adepth = 0; } |
	'\n' |
	error '\n'			{ cleanup(); };

config_spec:
	one_def |
	FILE_SYSTEM fs_list |
	OPTIONS opt_list |
	MAKEOPTIONS mkopt_list |
	MAXUSERS NUMBER			{ setmaxusers($2); } |
	IDENT WORD			{ setident($2); } |
	CONFIG conf root_spec sysparam_list
					{ addconf(&conf); } |
	PSEUDO_DEVICE WORD npseudo	{ addpseudo($2, $3); } |
	device_instance AT attachment locators flags_opt
					{ adddev($1, $3, $4, $5); };

mkopt_list:
	mkopt_list ',' mkoption |
	mkoption;

mkoption:
	WORD '=' value			{ addmkoption($1, $3); }

opt_list:
	opt_list ',' option |
	option;

option:
	WORD				{ addoption($1, NULL); } |
	WORD '=' value			{ addoption($1, $3); };

fs_list:
	fs_list ',' fsoption |
	fsoption;

fsoption:
	WORD				{ addfsoption($1); };

conf:
	WORD				{ conf.cf_name = $1;
					    conf.cf_lineno = currentline();
					    conf.cf_fstype = NULL;
					    conf.cf_root = NULL;
					    conf.cf_dump = NULL; };

root_spec:
	ROOT on_opt dev_spec fs_spec_opt
				{ setconf(&conf.cf_root, "root", $3); };

fs_spec_opt:
	TYPE fs_spec		{ setfstype(&conf.cf_fstype, $2); } |
	/* empty */;

fs_spec:
	'?'				{ $$ = intern("?"); } |
	WORD				{ $$ = $1; };

sysparam_list:
	sysparam_list sysparam |
	/* empty */;

sysparam:
	DUMPS on_opt dev_spec	 { setconf(&conf.cf_dump, "dumps", $3); };

dev_spec:
	'?'				{ $$ = new_si(intern("?"), NODEV); } |
	WORD				{ $$ = new_si($1, NODEV); } |
	major_minor			{ $$ = new_si(NULL, $1); };

major_minor:
	MAJOR NUMBER MINOR NUMBER	{ $$ = makedev($2, $4); };

on_opt:
	ON | /* empty */;

npseudo:
	NUMBER				{ $$ = $1; } |
	/* empty */			{ $$ = 1; };

device_instance:
	WORD '*'			{ $$ = starref($1); } |
	WORD				{ $$ = $1; };

attachment:
	ROOT				{ $$ = NULL; } |
	WORD '?'			{ $$ = wildref($1); } |
	WORD				{ $$ = $1; };

locators:
	locators locator		{ $$ = $2; app($2, $1); } |
	/* empty */			{ $$ = NULL; };

locator:
	WORD values			{ $$ = mk_ns($1, $2); } |
	WORD '?'			{ $$ = new_ns($1, NULL); };

flags_opt:
	FLAGS NUMBER			{ $$ = $2; } |
	/* empty */			{ $$ = 0; };

%%

void
yyerror(s)
	const char *s;
{

	error("%s", s);
}

/*
 * Cleanup procedure after syntax error: release any nvlists
 * allocated during parsing the current line.
 */
static void
cleanup()
{
	struct nvlist **np;
	int i;

	for (np = alloc, i = adepth; --i >= 0; np++)
		nvfree(*np);
	adepth = 0;
}

static void
setmachine(mch, mcharch)
	const char *mch;
	const char *mcharch;
{
	char buf[MAXPATHLEN];

	machine = mch;
	machinearch = mcharch;

	(void)sprintf(buf, "arch/%s/conf/files.%s", machine, machine);
	if (include(buf, ENDFILE, 0) != 0)
		exit(1);

	if (machinearch != NULL)
		(void)sprintf(buf, "arch/%s/conf/files.%s",
		    machinearch, machinearch);
	else
		strcpy(buf, _PATH_DEVNULL);
	if (include(buf, ENDFILE, 0) != 0)
		exit(1);

	if (include("conf/files", ENDFILE, 0) != 0)
		exit(1);
}

static void
check_maxpart()
{
	if (maxpartitions <= 0) {
		stop("cannot proceed without maxpartitions specifier");
	}
}

static void
app(p, q)
	struct nvlist *p, *q;
{
	while (p->nv_next)
		p = p->nv_next;
	p->nv_next = q;
}

static struct nvlist *
mk_nsis(name, count, adefs, opt)
	const char *name;
	int count;
	struct nvlist *adefs;
	int opt;
{
	struct nvlist *defs = adefs;
	struct nvlist **p;
	char buf[200];
	int i;

	if (count <= 0) {
		fprintf(stderr, "config: array with <= 0 size: %s\n", name);
		exit(1);
	}
	p = &defs;
	for(i = 0; i < count; i++) {
		if (*p == NULL)
			*p = new_s("0");
		sprintf(buf, "%s%c%d", name, ARRCHR, i);
		(*p)->nv_name = i == 0 ? name : intern(buf);
		(*p)->nv_int = i > 0 || opt;
		p = &(*p)->nv_next;
	}
	*p = 0;
	return defs;
}


static struct nvlist *
mk_ns(name, vals)
	const char *name;
	struct nvlist *vals;
{
	struct nvlist *p;
	char buf[200];
	int i;

	for(i = 0, p = vals; p; i++, p = p->nv_next) {
		sprintf(buf, "%s%c%d", name, ARRCHR, i);
		p->nv_name = i == 0 ? name : intern(buf);
	}
	return vals;
}
