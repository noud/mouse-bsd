#	$NetBSD: Makefile,v 1.34 1999/12/07 23:55:14 soren Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

MAN=	compat_freebsd.8 compat_ibcs2.8 compat_linux.8 \
	compat_osf1.8 compat_sunos.8 compat_svr4.8 compat_ultrix.8 \
	diskless.8 genassym.sh.8 intro.8 rc.8 sticky.8 yp.8
MLINKS+=	compat_svr4.8 compat_sco.8
MLINKS+=	compat_svr4.8 compat_solaris.8
MLINKS+=	compat_svr4.8 compat_xenix.8
MLINKS+=	yp.8 nis.8
SUBDIR=	man8.alpha man8.amiga man8.arm32 man8.atari man8.bebox \
	man8.hp300 man8.hpcmips man8.i386 man8.mac68k man8.mvme68k \
	man8.next68k man8.pc532 man8.pmax man8.sparc man8.sparc64 \
	man8.sun3 man8.vax man8.x68k

.include <bsd.man.mk>
.include <bsd.subdir.mk>
