/*	$NetBSD: openfirm.c,v 1.8 1999/10/31 15:22:32 mrg Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/psl.h>
#include <machine/stdarg.h>

#include <machine/openfirm.h>

#define min(x,y)	((x<y)?(x):(y))


int
OF_peer(phandle)
	int phandle;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t sibling;
	} args;

	args.name = ADR2CELL(&"peer");
	args.nargs = 1;
	args.nreturns = 1;
	args.phandle = HDL2CELL(phandle);
	if (openfirmware(&args) == -1)
		return 0;
	return args.sibling;
}

int
OF_child(phandle)
	int phandle;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t child;
	} args;

	args.name = ADR2CELL(&"child");
	args.nargs = 1;
	args.nreturns = 1;
	args.phandle = HDL2CELL(phandle);
	if (openfirmware(&args) == -1)
		return 0;
	return args.child;
}

int
OF_parent(phandle)
	int phandle;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t parent;
	} args;

	args.name = ADR2CELL(&"parent");
	args.nargs = 1;
	args.nreturns = 1;
	args.phandle = HDL2CELL(phandle);
	if (openfirmware(&args) == -1)
		return 0;
	return args.parent;
}

int
OF_instance_to_package(ihandle)
	int ihandle;
{
	static struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t phandle;
	} args;

	args.name = ADR2CELL(&"instance-to-package");
	args.nargs = 1;
	args.nreturns = 1;
	args.ihandle = HDL2CELL(ihandle);
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

/* Should really return a `long' */
int
OF_getproplen(handle, prop)
	int handle;
	char *prop;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t prop;
		cell_t size;
	} args;

	args.name = ADR2CELL(&"getproplen");
	args.nargs = 2;
	args.nreturns = 1;
	args.phandle = HDL2CELL(handle);
	args.prop = ADR2CELL(prop);
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_getprop(handle, prop, buf, buflen)
	int handle;
	char *prop;
	void *buf;
	int buflen;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t prop;
		cell_t buf;
		cell_t buflen;
		cell_t size;
	} args;

	if (buflen > NBPG)
		return -1;
	args.name = ADR2CELL(&"getprop");
	args.nargs = 4;
	args.nreturns = 1;
	args.phandle = HDL2CELL(handle);
	args.prop = ADR2CELL(prop);
	args.buf = ADR2CELL(buf);
	args.buflen = buflen;
	if (openfirmware(&args) == -1)
		return -1;
	return args.size;
}

int
OF_finddevice(name)
char *name;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t device;
		cell_t phandle;
	} args;

	args.name = ADR2CELL(&"finddevice");
	args.nargs = 1;
	args.nreturns = 1;
	args.device = ADR2CELL(name);
	if (openfirmware(&args) == -1)
		return -1;
	return args.phandle;
}

int
OF_instance_to_path(ihandle, buf, buflen)
	int ihandle;
	char *buf;
	int buflen;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t buf;
		cell_t buflen;
		cell_t length;
	} args;

	if (buflen > NBPG)
		return -1;
	args.name = ADR2CELL(&"instance-to-path");
	args.nargs = 3;
	args.nreturns = 1;
	args.ihandle = HDL2CELL(ihandle);
	args.buf = ADR2CELL(buf);
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	return args.length;
}

int
OF_package_to_path(phandle, buf, buflen)
	int phandle;
	char *buf;
	int buflen;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t phandle;
		cell_t buf;
		cell_t buflen;
		cell_t length;
	} args;

	if (buflen > NBPG)
		return -1;
	args.name = ADR2CELL(&"package-to-path");
	args.nargs = 3;
	args.nreturns = 1;
	args.phandle = HDL2CELL(phandle);
	args.buf = ADR2CELL(buf);
	args.buflen = buflen;
	if (openfirmware(&args) < 0)
		return -1;
	return args.length;
}

/*
 * The following two functions may need to be re-worked to be 64-bit clean.
 */
int
#ifdef	__STDC__
OF_call_method(char *method, int ihandle, int nargs, int nreturns, ...)
#else
OF_call_method(method, ihandle, nargs, nreturns, va_alist)
	char *method;
	int ihandle;
	int nargs;
	int nreturns;
	va_dcl
#endif
{
	va_list ap;
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t args_n_results[12];
	} args;
	long *ip, n;

	if (nargs > 6)
		return -1;
	args.name = ADR2CELL(&"call-method");
	args.nargs = nargs + 2;
	args.nreturns = nreturns + 1;
	args.method = ADR2CELL(method);
	args.ihandle = HDL2CELL(ihandle);
	va_start(ap, nreturns);
	for (ip = (long*)(args.args_n_results + (n = nargs)); --n >= 0;)
		*--ip = va_arg(ap, unsigned long);
	if (openfirmware(&args) == -1)
		return -1;
	if (args.args_n_results[nargs])
		return args.args_n_results[nargs];
	for (ip = (long*)(args.args_n_results + nargs + (n = args.nreturns)); --n > 0;)
		*va_arg(ap, unsigned long *) = *--ip;
	va_end(ap);
	return 0;
}

int
#ifdef	__STDC__
OF_call_method_1(char *method, int ihandle, int nargs, ...)
#else
OF_call_method_1(method, ihandle, nargs, va_alist)
	char *method;
	int ihandle;
	int nargs;
	va_dcl
#endif
{
	va_list ap;
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t method;
		cell_t ihandle;
		cell_t args_n_results[16];
	} args;
	long *ip, n;

	if (nargs > 6)
		return -1;
	args.name = ADR2CELL(&"call-method");
	args.nargs = nargs + 2;
	args.nreturns = 1;
	args.method = ADR2CELL(method);
	args.ihandle = HDL2CELL(ihandle);
	va_start(ap, nargs);
	for (ip = (long*)(args.args_n_results + (n = nargs)); --n >= 0;)
		*--ip = va_arg(ap, unsigned long);
	va_end(ap);
	if (openfirmware(&args) == -1)
		return -1;
	if (args.args_n_results[nargs])
		return -1;
	return args.args_n_results[nargs + 1];
}

int
OF_open(dname)
	char *dname;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t dname;
		cell_t handle;
	} args;
	int l;

	if ((l = strlen(dname)) >= NBPG)
		return -1;
	args.name = ADR2CELL(&"open");
	args.nargs = 1;
	args.nreturns = 1;
	args.dname = ADR2CELL(dname);
	if (openfirmware(&args) == -1)
		return -1;
	return args.handle;
}

void
OF_close(handle)
	int handle;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t handle;
	} args;

	args.name = ADR2CELL(&"close");
	args.nargs = 1;
	args.nreturns = 0;
	args.handle = HDL2CELL(handle);
	openfirmware(&args);
}

int
OF_test(service)
	char* service;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t service;
		cell_t status;
	} args;

	args.name = ADR2CELL(&"test");
	args.nargs = 1;
	args.nreturns = 1;
	args.service = ADR2CELL(service);
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

int
OF_test_method(service, method)
	int service;
	char* method;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t service;
		cell_t method;
		cell_t status;
	} args;

	args.name = ADR2CELL(&"test-method");
	args.nargs = 2;
	args.nreturns = 1;
	args.service = HDL2CELL(service);
	args.method = ADR2CELL(method);
	openfirmware(&args);
	return args.status;
}


/*
 * This assumes that character devices don't read in multiples of NBPG.
 */
int
OF_read(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t addr;
		cell_t len;
		cell_t actual;
	} args;
	int l, act = 0;

	args.name = ADR2CELL(&"read");
	args.nargs = 3;
	args.nreturns = 1;
	args.ihandle = HDL2CELL(handle);
	args.addr = ADR2CELL(addr);
	for (; len > 0; len -= l, addr += l) {
		l = min(NBPG, len);
		args.len = l;
		if (openfirmware(&args) == -1)
			return -1;
		if (args.actual > 0) {
			act += args.actual;
		}
		if (args.actual < l) {
			if (act)
				return act;
			else
				return args.actual;
		}
	}
	return act;
}

void prom_printf __P((const char *fmt, ...));	/* XXX for below */

int
OF_write(handle, addr, len)
	int handle;
	void *addr;
	int len;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t ihandle;
		cell_t addr;
		cell_t len;
		cell_t actual;
	} args;
	int l, act = 0;

	args.name = ADR2CELL(&"write");
	args.nargs = 3;
	args.nreturns = 1;
	args.ihandle = HDL2CELL(handle);
	args.addr = ADR2CELL(addr);
if (len>1024) { prom_printf("OF_write() > 1024\n");
#ifdef DDB
Debugger();
#else
panic("OF_write");
#endif
}
	for (; len > 0; len -= l, addr += l) {
		l = min(NBPG, len);
		args.len = l;
		if (openfirmware(&args) == -1)
			return -1;
		l = args.actual;
		act += l;
	}
	return act;
}


int
OF_seek(handle, pos)
	int handle;
	u_quad_t pos;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t handle;
		cell_t poshi;
		cell_t poslo;
		cell_t status;
	} args;

	args.name = ADR2CELL(&"seek");
	args.nargs = 3;
	args.nreturns = 1;
	args.handle = HDL2CELL(handle);
	args.poshi = HDL2CELL(pos >> 32);
	args.poslo = HDL2CELL(pos);
	if (openfirmware(&args) == -1)
		return -1;
	return args.status;
}

void
OF_boot(bootspec)
	char *bootspec;
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t bootspec;
	} args;
	int l;

	if ((l = strlen(bootspec)) >= NBPG)
		panic("OF_boot");
	args.name = ADR2CELL(&"boot");
	args.nargs = 1;
	args.nreturns = 0;
	args.bootspec = ADR2CELL(bootspec);
	openfirmware(&args);
	panic("OF_boot failed");
}

void
OF_enter()
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
	} args;

	args.name = ADR2CELL(&"enter");
	args.nargs = 0;
	args.nreturns = 0;
	openfirmware(&args);
}

void
OF_exit()
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
	} args;

	args.name = ADR2CELL(&"exit");
	args.nargs = 0;
	args.nreturns = 0;
	openfirmware(&args);
	panic("OF_exit failed");
}

void
OF_poweroff()
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
	} args;

	args.name = ADR2CELL(&"SUNW,power-off");
	args.nargs = 0;
	args.nreturns = 0;
	openfirmware(&args);
	panic("OF_poweroff failed");
}

void
(*OF_set_callback(newfunc))()
	void (*newfunc)();
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t newfunc;
		cell_t oldfunc;
	} args;

	args.name = ADR2CELL(&"set-callback");
	args.nargs = 1;
	args.nreturns = 1;
	args.newfunc = ADR2CELL(newfunc);
	if (openfirmware(&args) == -1)
		return 0;
	return (void*)(long)args.oldfunc;
}

void
OF_set_symbol_lookup(s2v, v2s)
	void (*s2v)(void *);
	void (*v2s)(void *);
{
	struct {
		cell_t name;
		cell_t nargs;
		cell_t nreturns;
		cell_t sym2val;
		cell_t val2sym;
	} args;

	args.name = ADR2CELL(&"set-symbol-lookup");
	args.nargs = 2;
	args.nreturns = 0;
	args.sym2val = ADR2CELL(s2v);
	args.val2sym = ADR2CELL(v2s);

	(void)openfirmware(&args);
}

#include "opt_ddb.h"
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

void OF_sym2val(cells)
	void *cells;
{
	struct args {
		cell_t service;
		cell_t nargs;
		cell_t nreturns;
		cell_t symbol;
		cell_t result;
		cell_t value;
	} *args = (struct args*)cells;
	db_sym_t symbol;
	db_expr_t value;

	/* Set data segment pointer */
	__asm __volatile("clr %%g4" : :);

	/* No args?  Nothing to do. */
	if (!args->nargs ||
	    !args->nreturns) return;

	/* Do we have a place for the value? */
	if (args->nreturns != 2) {
		args->nreturns = 1;
		args->result = -1;
		return;
	}
	symbol = (db_sym_t)args->symbol;
prom_printf("looking up symbol %s\n", symbol);
	db_symbol_values(symbol, (char**)NULL, &value);
	args->result = 0;
	args->value = ADR2CELL(value);
}

void OF_val2sym(cells)
	void *cells;
{
	struct args {
		cell_t service;
		cell_t nargs;
		cell_t nreturns;
		cell_t value;
		cell_t offset;
		cell_t symbol;
	} *args = (struct args*)cells;
	db_sym_t symbol;
	db_expr_t value;
	db_expr_t offset;

	/* Set data segment pointer */
	__asm __volatile("clr %%g4" : :);

	/* No args?  Nothing to do. */
	if (!args->nargs ||
	    !args->nreturns) return;

	/* Do we have a place for the value? */
	if (args->nreturns != 2) {
		args->nreturns = 1;
		args->offset = -1;
		return;
	}

	value = args->value;
prom_printf("looking up value %ld\n", value);
	symbol = db_search_symbol(value, 0, &offset);
	if (symbol == DB_SYM_NULL) {
		args->nreturns = 1;
		args->offset = -1;
		return;
	}
	args->offset = offset;
	args->symbol = ADR2CELL(symbol);

}
#endif
