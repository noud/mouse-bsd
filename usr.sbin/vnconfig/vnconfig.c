/*	$NetBSD: vnconfig.c,v 1.18 2000/02/16 06:52:31 enami Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1993 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vnconfig.c 1.1 93/12/15$
 *
 *	@(#)vnconfig.c	8.1 (Berkeley) 12/15/93
 */

#define __POOL_EXPOSE			/* dev/vndvar.h uses struct pool */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <dev/vndvar.h>

#include <disktab.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define VND_CONFIG	1
#define VND_UNCONFIG	2
#define VND_DEBUG	3

int	verbose = 0;
int	readonly = 0;
char	*tabname;

int	config __P((char *, char *, char *, int));
int	getgeom __P((struct vndgeom *, char *));
int	main __P((int, char **));
char   *rawdevice __P((char *));
void	usage __P((void));

static int debugaction(char *dev, const char *setting)
{
 char rdev[MAXPATHLEN+1];
 int fd;
 int val;
 int rv;

 if (setting) val = strtol(setting,0,0);
 fd = opendisk(dev,setting?O_WRONLY:O_RDONLY,rdev,sizeof(rdev),0);
 if (fd < 0)
  { warn("%s: opendisk",rdev);
    return(1);
  }
 rv = 0;
 if (setting)
  { if (ioctl(fd,VNDIOSDEBUG,&val) < 0)
     { warn("%s: VNDIOSDEBUG",&rdev[0]);
       rv = 1;
     }
  }
 else
  { if (ioctl(fd,VNDIOGDEBUG,&val) < 0)
     { warn("%s: VNDIOGDEBUG",&rdev[0]);
       rv = 1;
     }
    else
     { printf("%d\n",val);
     }
  }
 close(fd);
 return(rv);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, rv, action = VND_CONFIG;

	while ((ch = getopt(argc, argv, "cdrt:uv")) != -1) {
		switch (ch) {
		case 'c':
			action = VND_CONFIG;
			break;
		case 'd':
			action = VND_DEBUG;
			break;
		case 'r':
			readonly = 1;
			break;
		case 't':
			tabname = optarg;
			break;
		case 'u':
			action = VND_UNCONFIG;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	switch (action) {
	case VND_CONFIG:
		if ((argc < 2 || argc > 3) ||
		    (argc == 3 && tabname != NULL))
			usage();
		rv = config(argv[0], argv[1], (argc == 3) ? argv[2] : NULL,
		    action);
		break;
	case VND_UNCONFIG:
		if (argc != 1 || tabname != NULL)
			usage();
		rv = config(argv[0], NULL, NULL, action);
		break;
	case VND_DEBUG:
		if ((argc < 1) || (argc > 2))
			usage();
		rv = debugaction(argv[0],(argc>1)?argv[1]:0);
		break;
	}
	exit(rv);
}

int
config(dev, file, geom, action)
	char *dev, *file, *geom;
	int action;
{
	struct vnd_ioctl vndio;
	struct disklabel *lp;
	char rdev[MAXPATHLEN + 1];
	int fd, rv;

	fd = opendisk(dev, O_RDWR, rdev, sizeof(rdev), 0);
	if (fd < 0) {
		warn("%s: opendisk", rdev);
		return (1);
	}

	memset(&vndio, 0, sizeof(vndio));
#ifdef __GNUC__
	rv = 0;			/* XXX */
#endif

	vndio.vnd_file = file;
	if (geom != NULL) {
		rv = getgeom(&vndio.vnd_geom, geom);
		if (rv != 0)
			errx(1, "invalid geometry: %s", geom);
		vndio.vnd_flags = VNDIOF_HASGEOM;
	} else if (tabname != NULL) {
		lp = getdiskbyname(tabname);
		if (lp == NULL)
			errx(1, "unknown disk type: %s", tabname);
		vndio.vnd_geom.vng_secsize = lp->d_secsize;
		vndio.vnd_geom.vng_nsectors = lp->d_nsectors;
		vndio.vnd_geom.vng_ntracks = lp->d_ntracks;
		vndio.vnd_geom.vng_ncylinders = lp->d_ncylinders;
		vndio.vnd_flags = VNDIOF_HASGEOM;
	}

	/*
	 * Clear (un-configure) the device
	 */
	if (action == VND_UNCONFIG) {
		rv = ioctl(fd, VNDIOCCLR, &vndio);
		if (rv)
			warn("%s: VNDIOCCLR", rdev);
		else if (verbose)
			printf("%s: cleared\n", rdev);
	}
	/*
	 * Configure the device
	 */
	if (action == VND_CONFIG) {
		if (readonly) vndio.vnd_flags |= VNDIOF_READONLY;
		rv = ioctl(fd, VNDIOCSET, &vndio);
		if (rv)
			warn("%s: VNDIOCSET", rdev);
		else if (verbose) {
			printf("%s: %d bytes on %s", rdev, vndio.vnd_size,
			    file);
			if (vndio.vnd_flags & VNDIOF_HASGEOM)
				printf(" using geometry %d/%d/%d/%d",
				    vndio.vnd_geom.vng_secsize,
				    vndio.vnd_geom.vng_nsectors,
				    vndio.vnd_geom.vng_ntracks,
				    vndio.vnd_geom.vng_ncylinders);
			printf("\n");
		}

	}

	(void) close(fd);
	fflush(stdout);
	return (rv < 0);
}

int
getgeom(vng, cp)
	struct vndgeom *vng;
	char *cp;
{
	char *secsize, *nsectors, *ntracks, *ncylinders;

#define	GETARG(arg) \
	do { \
		if (cp == NULL || *cp == '\0') \
			return (1); \
		arg = strsep(&cp, "/"); \
		if (arg == NULL) \
			return (1); \
	} while (0)

	GETARG(secsize);
	GETARG(nsectors);
	GETARG(ntracks);
	GETARG(ncylinders);

#undef GETARG

	/* Too many? */
	if (cp != NULL)
		return (1);

#define	CVTARG(str, num) \
	do { \
		num = strtol(str, &cp, 10); \
		if (*cp != '\0') \
			return (1); \
	} while (0)

	CVTARG(secsize, vng->vng_secsize);
	CVTARG(nsectors, vng->vng_nsectors);
	CVTARG(ntracks, vng->vng_ntracks);
	CVTARG(ncylinders, vng->vng_ncylinders);

#undef CVTARG

	return (0);
}

void
usage()
{

	(void)fprintf(stderr, "%s%s%s",
	    "usage: vnconfig [-c] [-r] [-t typename] [-v] special-file"
		" regular-file [geomspec]\n",
	    "       vnconfig -u [-v] special-file\n",
	    "       vnconfig -d [-v] special-file [debug-setting]\n");
	exit(1);
}
