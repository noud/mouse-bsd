/*	$NetBSD: a.out.c,v 1.1 1999/06/13 12:54:40 mrg Exp $	*/

/*
 * Copyright (c) 1993 Terrence R. Lambert.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: a.out.c,v 1.1 1999/06/13 12:54:40 mrg Exp $");

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/lkm.h>

#include <a.out.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "modload.h"

/*
 * Expected linker options:
 *
 * -A		executable to link against
 * -e		entry point
 * -o		output file
 * -T		address to link to in hex (assumes it's a page boundry)
 * <target>	object file
 */

#define	LINKCMD "ld -A %s -e _%s -o %s -T %p %s"

void
a_out_linkcmd(char *buf,
	      size_t len,
	      const char *kernel,
	      const char *entry,
	      const char *outfile,
	      const void *address,
	      const char *object)
{
	ssize_t n;

	n = snprintf(buf, len, LINKCMD, kernel, entry,
		     outfile, address, object);
	if (n >= len)
		errx(1, "link command longer than %lu bytes", (u_long)len);
}

static int
a_out_read_header(int fd, struct exec *info_buf)
{
	ssize_t n;

	n = read(fd, info_buf, sizeof(*info_buf));
	if (n < 0)
		err(1, "failed reading %lu bytes", (u_long)sizeof(*info_buf));
	if (n != sizeof(*info_buf)) {
		if (debug)
			fprintf(stderr, "failed to read %lu bytes",
				(u_long)sizeof(*info_buf));
		return -1;
	}

	/*
	 * Magic number...
	 */
	if (N_BADMAG(*info_buf))
		errx(4, "not an a.out format file");
	return 0;
}

extern int symtab;

int
a_out_mod_sizes(fd, modsize, strtablen, resrvp, sp)
	int fd;
	size_t *modsize;
	int *strtablen;
	struct lmc_resrv *resrvp;
	struct stat *sp;
{
	struct exec info_buf;

	if (a_out_read_header(fd, &info_buf) < 0)
		return -1;

	/*
	 * Calculate the size of the module
	 */
	*modsize = info_buf.a_text + info_buf.a_data + info_buf.a_bss;

	*strtablen = sp->st_size - N_STROFF(info_buf);

	if (symtab) {
		/*
		 * XXX TODO:  grovel through symbol table looking for
		 * just the symbol table stuff from the new module,
		 * and skip the stuff from the kernel.
		 */
		resrvp->sym_size = info_buf.a_syms + *strtablen;
		resrvp->sym_symsize = info_buf.a_syms;
	} else
		resrvp->sym_size = resrvp->sym_symsize = 0;

 	return (0);
}

void *
a_out_mod_load(int fd)
{
	struct exec info_buf;
	size_t b;
	ssize_t n;
	char buf[10 * BUFSIZ];

	/*
	 * Get the load module post load size... do this by reading the
	 * header and doing page counts.
	 */
	if (a_out_read_header(fd, &info_buf) < 0)
		return NULL;

	/*
	 * Seek to the text offset to start loading...
	 */
	if (lseek(fd, N_TXTOFF(info_buf), 0) == -1)
		err(12, "lseek");

	/*
	 * Transfer the relinked module to kernel memory in chunks of
	 * MODIOBUF size at a time.
	 */
	b = info_buf.a_text + info_buf.a_data;
	while (b) {
		n = read(fd, buf, MIN(b, sizeof(buf)));
		if (n < 0)
			err(1, "while reading from prelinked module");
		if (n == 0)
			errx(1, "EOF while reading from prelinked module");

		loadbuf(buf, n);
		b -= n;
	}
	return (void*)info_buf.a_entry;
}

extern int devfd, modfd;
extern struct lmc_resrv resrv;

void
a_out_mod_symload(strtablen)
	int strtablen;
{
	struct exec info_buf;
	struct lmc_loadbuf ldbuf;
	struct nlist *nlp;
	char buf[10 * BUFSIZ];
	char *symbuf;
	int bytesleft, sz;
	int numsyms;	/* XXX unused? */

	if (a_out_read_header(modfd, &info_buf) < 0)
		return;

	/*
	 * Seek to the symbol table to start loading it...
	 */
	if (lseek(modfd, N_SYMOFF(info_buf), SEEK_SET) == -1)
		err(12, "lseek");

	/*
	 * Transfer the symbol table entries.  First, read them all in,
	 * then adjust their string table pointers, then
	 * copy in bulk.  Then copy the string table itself.
	 */

	symbuf = malloc(info_buf.a_syms);
	if (symbuf == 0)
		err(13, "malloc");

	if (read(modfd, symbuf, info_buf.a_syms) != info_buf.a_syms)
		err(14, "read");
	numsyms = info_buf.a_syms / sizeof(struct nlist);

	for (nlp = (struct nlist *)symbuf;
	    (char *)nlp < symbuf + info_buf.a_syms; nlp++) {
		register int strx;

		strx = nlp->n_un.n_strx;
		if (strx != 0) {
			/*
			 * If a valid name, set the name ptr to point at the
			 * loaded address for the string in the string table.
			 */
			if (strx > strtablen)
				nlp->n_un.n_name = 0;
			else
				nlp->n_un.n_name = (char *)(strx +
				    resrv.sym_addr + info_buf.a_syms);
		}
	}
	/*
	 * we've fixed the symbol table entries, now load them
	 */
	for (bytesleft = info_buf.a_syms; bytesleft > 0; bytesleft -= sz) {
		sz = MIN(bytesleft, MODIOBUF);
		ldbuf.cnt = sz;
		ldbuf.data = symbuf;
		if (ioctl(devfd, LMLOADSYMS, &ldbuf) == -1)
			err(11, "error transferring sym buffer");
		symbuf += sz;
	}

	free(symbuf - info_buf.a_syms);
	/* and now read the string table and load it. */
	for (bytesleft = strtablen; bytesleft > 0; bytesleft -= sz) {
		sz = MIN(bytesleft, MODIOBUF);
		read(modfd, buf, sz);
		ldbuf.cnt = sz;
		ldbuf.data = buf;
		if (ioctl(devfd, LMLOADSYMS, &ldbuf) == -1)
			err(11, "error transferring stringtable buffer");
	}
}
