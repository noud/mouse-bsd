/*	$NetBSD: machdep.c,v 1.2 1999/03/26 06:54:40 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin Jewell
 * Copyright (c) 1994 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>

#include <stand.h>
#include <next68k/next68k/nextrom.h>

char *mg;

#define	MON(type, off) (*(type *)((u_int) (mg) + off))

extern char *entry_point;


#ifdef DEBUG
int debug = 1;
#else
int debug = 0;
#endif

#ifdef DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

void
machdep_start(char *entry, int howto, char *loadaddr, char *ssym, char *esym)
{
  DPRINTF(("machdep_start(entry=0x%lx,howto=0x%x,loadaddr=0x%lx,ssym=0x%lx,esym=0x%lx\n",
           (u_long)entry,howto,(u_long)loadaddr,(u_long)ssym,(u_long)esym));
  MON(int,MG_boot_how) = howto;
  entry_point = entry + (long)loadaddr;
  DPRINTF(("start=0x%lx\n", (u_long)entry_point));
  
  /* @@@ hack to pass esym to kernel */
  *((u_int *)loadaddr) = (u_int)esym;

  /* return to exec, so that main can return entry point */
}

typedef int (*getcptr)(void);
typedef int (*putcptr)(int);

int
getchar(void)
{
  return(MON(getcptr,MG_getc)());
}

void
putchar(int c)
{
  MON(putcptr,MG_putc)(c);
}

__dead void
_rtt(void)
{
    extern __dead void _halt __P((void)) __attribute__((noreturn));

    printf("Press any key to halt.\n");
    getchar();
    _halt();
    /* NOTREACHED */
}

struct trapframe {
    int dregs[8];
    int aregs[8];
    short sr;
    int pc;
    short frame;
    char info[0];
};

int trap __P((struct trapframe *fp));

int
trap(struct trapframe *fp)
{
    static int intrap = 0;

    if (intrap)
	return 0;
    intrap = 1;
    printf("Got unexpected trap: format=%x vector=%x sr=%x pc=%x\n",
	   (fp->frame>>12)&0xF, fp->frame&0xFFF, fp->sr, fp->pc);
    printf("dregs: %x %x %x %x %x %x %x %x\n",
	   fp->dregs[0], fp->dregs[1], fp->dregs[2], fp->dregs[3], 
	   fp->dregs[4], fp->dregs[5], fp->dregs[6], fp->dregs[7]);
    printf("aregs: %x %x %x %x %x %x %x %x\n",
	   fp->aregs[0], fp->aregs[1], fp->aregs[2], fp->aregs[3], 
	   fp->aregs[4], fp->aregs[5], fp->aregs[6], fp->aregs[7]);
    intrap = 0;
    printf("Halting.\n");
    return 0;
}
