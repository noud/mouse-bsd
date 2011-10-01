PROG = mount_pfs
REQUESTS != cat requests.list
SRCS = $(REQUESTS:C/.*/rq-&.c/) getmntopts.c mount_pfs.c request-list.c\
 state.c
DPSRCS = request-list.h request-list.c

request-list.h: requests.list .NOTMAIN
	sed -e 's/#.*//' -e '/^[ 	]*$$/d' -e 's/.*/extern void rq_&(struct pfs_req *, int, const void *, int, int);/' < requests.list > request-list.h+
	mv request-list.h+ request-list.h

request-list.c: requests.list .NOTMAIN
	(echo '#include "requests.h"'; echo 'struct reqtbl reqtbl[] = {'; sed -e 's/#.*//' -e '/^[ 	]*$$/d' -e h -e y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/ -e 's/.*/{ PFSOP_&,/p' -e g -e 's/.*/\&rq_& },/' <requests.list; echo '}; int reqtbl_n ='; wc -l <requests.list; echo ';') >request-list.c+
	mv request-list.c+ request-list.c

MAN = mount_pfs.8

MOUNT = ${.CURDIR}/../mount
CPPFLAGS += -I${.CURDIR}/../../sys -I${MOUNT}
.PATH:	${MOUNT}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
