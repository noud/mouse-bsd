#	$NetBSD: Makefile,v 1.2 1999/05/04 04:56:44 cgd Exp $

DEP=	syscalls.conf syscalls.master ../../kern/makesyscalls.sh
OBJS=	osf1_sysent.c osf1_syscalls.c osf1_syscall.h osf1_syscallargs.h

${OBJS}: ${DEP}
	-mv -f osf1_sysent.c osf1_sysent.c.bak
	-mv -f osf1_syscalls.c osf1_syscalls.c.bak
	-mv -f osf1_syscall.h osf1_syscall.h.bak
	-mv -f osf1_syscallargs.h osf1_syscallargs.h.bak
	sh ../../kern/makesyscalls.sh syscalls.conf syscalls.master

clean:
	rm -f ${OBJS}
