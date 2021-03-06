/*	$NetBSD: apmd.c,v 1.13 2000/01/13 16:04:53 jlam Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <machine/apmvar.h>
#include <err.h>
#include "pathnames.h"
#include "apm-proto.h"

#define MAX(a,b) (a > b ? a : b)
#define TRUE 1
#define FALSE 0

const char apmdev[] = _PATH_APM_CTLDEV;
const char sockfile[] = _PATH_APM_SOCKET;

static int debug = 0;

extern char *__progname;

void usage (void);
int power_status (int fd, int force, struct apm_power_info *pinfo);
int bind_socket (const char *sn, mode_t mode, uid_t uid, gid_t gid);
enum apm_state handle_client(int sock_fd, int ctl_fd);
void suspend(int ctl_fd);
void stand_by(int ctl_fd);
void resume(int ctl_fd);
void sigexit(int signo);
void make_noise(int howmany);
void do_etc_file(const char *file);
void do_ac_state(int state);

void
sigexit(int signo)
{
    exit(1);
}

void
usage(void)
{
    fprintf(stderr,"usage: %s [-d] [-s] [-a] [-l] [-q] [-t seconds] [-S sockname]\n\t[-m sockmode] [-o sockowner:sockgroup] [-f devname]\n", __progname);
    exit(1);
}


int
power_status(int fd, int force, struct apm_power_info *pinfo)
{
    struct apm_power_info bstate;
    static struct apm_power_info last;
    int acon = 0;

    if (ioctl(fd, APM_IOC_GETPOWER, &bstate) == 0) {
	/* various conditions under which we report status:  something changed
	   enough since last report, or asked to force a print */
	if (bstate.ac_state == APM_AC_ON)
	    acon = 1;
	if (force ||
	    bstate.ac_state != last.ac_state ||
	    bstate.battery_state != last.battery_state ||
	    (bstate.minutes_left && bstate.minutes_left < 15) ||
	    abs(bstate.battery_life - last.battery_life) > 20) {
	    if (bstate.minutes_left)
		syslog(LOG_NOTICE,
		       "battery status: %s. external power status: %s. "
		       "estimated battery life %d%% (%d minutes)",
		       battstate(bstate.battery_state),
		       ac_state(bstate.ac_state), bstate.battery_life,
		       bstate.minutes_left);
	    else
		syslog(LOG_NOTICE,
		       "battery status: %s. external power status: %s. "
		       "estimated battery life %d%%",
		       battstate(bstate.battery_state),
		       ac_state(bstate.ac_state), bstate.battery_life);
	    last = bstate;
	}
	if (pinfo)
	    *pinfo = bstate;
    } else
	syslog(LOG_ERR, "cannot fetch power status: %m");
    return acon;
}

static char *socketname;

static void sockunlink(void);

static void
sockunlink(void)
{
    if (socketname)
	(void) remove(socketname);
}

int
bind_socket(const char *sockname, mode_t mode, uid_t uid, gid_t gid)
{
    int sock;
    struct sockaddr_un s_un;

    sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock == -1)
	err(1, "cannot create local socket");

    s_un.sun_family = AF_LOCAL;
    strncpy(s_un.sun_path, sockname, sizeof(s_un.sun_path));
    s_un.sun_len = SUN_LEN(&s_un);
    /* remove it if present, we're moving in */
    (void) remove(sockname);
    if (bind(sock, (struct sockaddr *)&s_un, s_un.sun_len) == -1)
	err(1, "cannot create APM socket");
    if (chmod(sockname, mode) == -1 || chown(sockname, uid, gid) == -1)
	err(1, "cannot set socket mode/owner/group to %o/%d/%d",
	    mode, uid, gid);
    listen(sock, 1);
    socketname = strdup(sockname);
    atexit(sockunlink);
    return sock;
}

enum apm_state
handle_client(int sock_fd, int ctl_fd)
{
    /* accept a handle from the client, process it, then clean up */
    int cli_fd;
    struct sockaddr_un from;
    int fromlen = sizeof(from);
    struct apm_command cmd;
    struct apm_reply reply;

    cli_fd = accept(sock_fd, (struct sockaddr *)&from, &fromlen);
    if (cli_fd == -1) {
	syslog(LOG_INFO, "client accept failure: %m");
	return NORMAL;
    }
    if (recv(cli_fd, &cmd, sizeof(cmd), 0) != sizeof(cmd)) {
	(void) close(cli_fd);
	syslog(LOG_INFO, "client size botch");
	return NORMAL;
    }
    if (cmd.vno != APMD_VNO) {
	close(cli_fd);			/* terminate client */
	/* no error message, just drop it. */
	return NORMAL;
    }
    power_status(ctl_fd, 0, &reply.batterystate);
    switch (cmd.action) {
    default:
	reply.newstate = NORMAL;
	break;
    case SUSPEND:
	reply.newstate = SUSPENDING;
	break;
    case STANDBY:
	reply.newstate = STANDING_BY;
	break;
    }
    reply.vno = APMD_VNO;
    if (send(cli_fd, &reply, sizeof(reply), 0) != sizeof(reply)) {
	syslog(LOG_INFO, "client reply botch");
    }
    close(cli_fd);
    return reply.newstate;
}

static int speaker_ok = TRUE;

void
make_noise(howmany)
int howmany;
{
    int spkrfd;
    int trycnt;

    if (!speaker_ok)		/* don't bother after sticky errors */
	return;

    for (trycnt = 0; trycnt < 3; trycnt++) {
	spkrfd = open(_PATH_DEV_SPEAKER, O_WRONLY);
	if (spkrfd == -1) {
	    switch (errno) {
	    case EBUSY:
		usleep(500000);
		errno = EBUSY;
		continue;
	    case ENOENT:
	    case ENODEV:
	    case ENXIO:
	    case EPERM:
	    case EACCES:
		syslog(LOG_INFO,
		       "speaker device " _PATH_DEV_SPEAKER " unavailable: %m");
		speaker_ok = FALSE;
		return;
	    }
	} else
	    break;
    }
    if (spkrfd == -1) {
	syslog(LOG_WARNING, "cannot open " _PATH_DEV_SPEAKER ": %m");
	return;
    }
    syslog(LOG_DEBUG, "sending %d tones to speaker\n", howmany);
    write (spkrfd, "o4cc", 2 + howmany);
    close(spkrfd);
    return;
}


void
suspend(int ctl_fd)
{
    do_etc_file(_PATH_APM_ETC_SUSPEND);
    sync();
    make_noise(2);
    sync();
    sync();
    sleep(1);
    ioctl(ctl_fd, APM_IOC_SUSPEND, 0);
}

void
stand_by(int ctl_fd)
{
    do_etc_file(_PATH_APM_ETC_STANDBY);
    sync();
    make_noise(1);
    sync();
    sync();
    sleep(1);
    ioctl(ctl_fd, APM_IOC_STANDBY, 0);
}

#define TIMO (10*60)			/* 10 minutes */

void
resume(int ctl_fd)
{
    do_etc_file(_PATH_APM_ETC_RESUME);
}

int
main(int argc, char *argv[])
{
    const char *fname = apmdev;
    int ctl_fd, sock_fd, ch, ready;
    int statonly = 0;
    fd_set devfds;
    fd_set selcopy;
    struct apm_event_info apmevent;
    int suspends, standbys, resumes;
    int noacsleep = 0;
    int lowbattsleep = 0;
    mode_t mode = 0660;
    struct timeval tv = {TIMO, 0}, stv;
    const char *sockname = sockfile;
    char *user, *group;
    char *scratch;
    uid_t uid = 0;
    gid_t gid = 0;
    struct passwd *pw;
    struct group *gr;

    while ((ch = getopt(argc, argv, "qaldsf:t:S:m:o:")) != -1)
	switch(ch) {
	case 'q':
	    speaker_ok = FALSE;
	    break;
	case 'a':
	    noacsleep = 1;
	    break;
	case 'l':
	    lowbattsleep = 1;
	    break;
	case 'd':
	    debug = 1;
	    break;
	case 'f':
	    fname = optarg;
	    break;
	case 'S':
	    sockname = optarg;
	    break;
	case 't':
	    tv.tv_sec = strtoul(optarg, 0, 0);
	    if (tv.tv_sec == 0)
		usage();
	    break;
	case 'm':
	    mode = strtoul(optarg, 0, 8);
	    if (mode == 0)
		usage();
	    break;
	case 'o':
	    /* (user):(group) */
	    user = optarg;
	    group = strchr(user, ':');
	    if (group)
		*group++ = '\0';
	    if (*user) {
		uid = strtoul(user, &scratch, 0);
		if (*scratch != '\0') {
		    pw = getpwnam(user);
		    if (pw)
			uid = pw->pw_uid;
		    else
			errx(1, "user name `%s' unknown", user);
		}
	    }
	    if (group && *group) {
		gid = strtoul(group, &scratch, 0);
		if (*scratch != '\0') {
		    gr = getgrnam(group);
		    if (gr)
			gid = gr->gr_gid;
		    else
			errx(1, "group name `%s' unknown", group);
		}
	    }
	    break;
	case 's':			/* status only */
	    statonly = 1;
	    break;
	case '?':

	default:
	    usage();
	}
    argc -= optind;
    argv += optind;
    if ((ctl_fd = open(fname, O_RDWR)) == -1) {
	(void)err(1, "cannot open device file `%s'", fname);
    }
    if (debug) {
	openlog(__progname, LOG_CONS, LOG_LOCAL1);
    } else {
	openlog(__progname, LOG_CONS, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_NOTICE));
	daemon(0, 0);
	pidfile(NULL);
    }
    if (statonly) {
        power_status(ctl_fd, 1, 0);
	exit(0);
    } else {
	struct apm_power_info pinfo;
	power_status(ctl_fd, 1, &pinfo);
	do_ac_state(pinfo.ac_state);
    }

    (void) signal(SIGTERM, sigexit);
    (void) signal(SIGHUP, sigexit);
    (void) signal(SIGINT, sigexit);
    (void) signal(SIGPIPE, SIG_IGN);


    sock_fd = bind_socket(sockname, mode, uid, gid);

    FD_ZERO(&devfds);
    FD_SET(ctl_fd, &devfds);
    FD_SET(sock_fd, &devfds);



    for (selcopy = devfds, errno = 0, stv = tv;
	 (ready = select(MAX(ctl_fd,sock_fd)+1, &selcopy, 0, 0, &stv)) >= 0 ||
	     errno == EINTR;
	 selcopy = devfds, errno = 0, stv = tv) {
	if (errno == EINTR)
	    continue;
	if (ready == 0) {
	    /* wakeup for timeout: take status */
	    power_status(ctl_fd, 0, 0);
	}
	if (FD_ISSET(ctl_fd, &selcopy)) {
	    suspends = standbys = resumes = 0;
	    while (ioctl(ctl_fd, APM_IOC_NEXTEVENT, &apmevent) == 0) {
		syslog(LOG_DEBUG, "apmevent %04x index %d", apmevent.type,
		       apmevent.index);
		switch (apmevent.type) {
		case APM_SUSPEND_REQ:
		case APM_USER_SUSPEND_REQ:
		case APM_CRIT_SUSPEND_REQ:
		    suspends++;
		    break;
		case APM_BATTERY_LOW:
		    if (lowbattsleep)
			suspends++;
		    break;
		case APM_USER_STANDBY_REQ:
		case APM_STANDBY_REQ:
		    standbys++;
		    break;
#if 0
		case APM_CANCEL:
		    suspends = standbys = 0;
		    break;
#endif
		case APM_NORMAL_RESUME:
		case APM_CRIT_RESUME:
		case APM_SYS_STANDBY_RESUME:
		    resumes++;
		    break;
		case APM_POWER_CHANGE:
		{
		    struct apm_power_info pinfo;
		    power_status(ctl_fd, 0, &pinfo);
		    do_ac_state(pinfo.ac_state);
		    break;
		}
		default:
		    break;
		}
	    }
	    if ((standbys || suspends) && noacsleep &&
		power_status(ctl_fd, 0, 0)) {
		syslog(LOG_DEBUG, "not sleeping cuz AC is connected");
	    } else if (suspends) {
		suspend(ctl_fd);
	    } else if (standbys) {
		stand_by(ctl_fd);
	    } else if (resumes) {
		resume(ctl_fd);
		syslog(LOG_NOTICE, "system resumed from APM sleep");
	    }
	    ready--;
	}
	if (ready == 0)
	    continue;
	if (FD_ISSET(sock_fd, &selcopy)) {
	    switch (handle_client(sock_fd, ctl_fd)) {
	    case NORMAL:
		break;
	    case SUSPENDING:
		suspend(ctl_fd);
		break;
	    case STANDING_BY:
		stand_by(ctl_fd);
		break;
	    }
	}
    }
    syslog(LOG_ERR, "select failed: %m");
    exit(1);
}

void
do_etc_file(const char *file)
{
    pid_t pid;
    int status;
    const char *prog;

    /* If file doesn't exist, do nothing. */
    if (access(file, X_OK|R_OK)) {
	syslog(LOG_DEBUG, "do_etc_file(): cannot access file %s", file);
	return;
    }

    prog = strrchr(file, '/');
    if (prog)
	prog++;
    else
	prog = file;

    pid = fork();
    switch (pid) {
    case -1:
	syslog(LOG_ERR, "failed to fork(): %m");
	return;
    case 0:
	/* We are the child. */
	execl(file, prog, NULL);
	_exit(-1);
	/* NOTREACHED */
    default:
	/* We are the parent. */
	wait4(pid, &status, 0, 0);
	if (WIFEXITED(status))
	    syslog(LOG_DEBUG, "%s exited with status %d", file,
		   WEXITSTATUS(status));
	else {
	    syslog(LOG_ERR, "%s exited abnormally.", file);
	}
	break;
    }
}

void
do_ac_state(int state)
{
	switch (state) {
	case APM_AC_OFF:
		do_etc_file(_PATH_APM_ETC_BATTERY);
		break;
	case APM_AC_ON:
	case APM_AC_BACKUP:
		do_etc_file(_PATH_APM_ETC_LINE);
		break;
	default:
		/* Silently ignore */
	}
}
