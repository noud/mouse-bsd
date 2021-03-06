/*	$NetBSD: ibcs2_sysent.c,v 1.25 2000/01/10 03:12:19 matt Exp $	*/

/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.21 2000/01/10 03:10:15 matt Exp
 */

#include "opt_sysv.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_syscallargs.h>
#include <compat/ibcs2/ibcs2_statfs.h>

#define	s(type)	sizeof(type)

struct sysent ibcs2_sysent[] = {
	{ 0, 0,
	    sys_nosys },			/* 0 = syscall */
	{ 1, s(struct sys_exit_args),
	    sys_exit },				/* 1 = exit */
	{ 0, 0,
	    sys_fork },				/* 2 = fork */
	{ 3, s(struct ibcs2_sys_read_args),
	    ibcs2_sys_read },			/* 3 = read */
	{ 3, s(struct sys_write_args),
	    sys_write },			/* 4 = write */
	{ 3, s(struct ibcs2_sys_open_args),
	    ibcs2_sys_open },			/* 5 = open */
	{ 1, s(struct sys_close_args),
	    sys_close },			/* 6 = close */
	{ 3, s(struct ibcs2_sys_waitsys_args),
	    ibcs2_sys_waitsys },		/* 7 = waitsys */
	{ 2, s(struct ibcs2_sys_creat_args),
	    ibcs2_sys_creat },			/* 8 = creat */
	{ 2, s(struct sys_link_args),
	    sys_link },				/* 9 = link */
	{ 1, s(struct ibcs2_sys_unlink_args),
	    ibcs2_sys_unlink },			/* 10 = unlink */
	{ 2, s(struct ibcs2_sys_execv_args),
	    ibcs2_sys_execv },			/* 11 = execv */
	{ 1, s(struct ibcs2_sys_chdir_args),
	    ibcs2_sys_chdir },			/* 12 = chdir */
	{ 1, s(struct ibcs2_sys_time_args),
	    ibcs2_sys_time },			/* 13 = time */
	{ 3, s(struct ibcs2_sys_mknod_args),
	    ibcs2_sys_mknod },			/* 14 = mknod */
	{ 2, s(struct ibcs2_sys_chmod_args),
	    ibcs2_sys_chmod },			/* 15 = chmod */
	{ 3, s(struct ibcs2_sys_chown_args),
	    ibcs2_sys_chown },			/* 16 = chown */
	{ 1, s(struct sys_obreak_args),
	    sys_obreak },			/* 17 = obreak */
	{ 2, s(struct ibcs2_sys_stat_args),
	    ibcs2_sys_stat },			/* 18 = stat */
	{ 3, s(struct compat_43_sys_lseek_args),
	    compat_43_sys_lseek },		/* 19 = lseek */
	{ 0, 0,
	    sys_getpid },			/* 20 = getpid */
	{ 6, s(struct ibcs2_sys_mount_args),
	    ibcs2_sys_mount },			/* 21 = mount */
	{ 1, s(struct ibcs2_sys_umount_args),
	    ibcs2_sys_umount },			/* 22 = umount */
	{ 1, s(struct ibcs2_sys_setuid_args),
	    ibcs2_sys_setuid },			/* 23 = setuid */
	{ 0, 0,
	    sys_getuid },			/* 24 = getuid */
	{ 1, s(struct ibcs2_sys_stime_args),
	    ibcs2_sys_stime },			/* 25 = stime */
	{ 0, 0,
	    sys_nosys },			/* 26 = unimplemented ibcs2_ptrace */
	{ 1, s(struct ibcs2_sys_alarm_args),
	    ibcs2_sys_alarm },			/* 27 = alarm */
	{ 2, s(struct ibcs2_sys_fstat_args),
	    ibcs2_sys_fstat },			/* 28 = fstat */
	{ 0, 0,
	    ibcs2_sys_pause },			/* 29 = pause */
	{ 2, s(struct ibcs2_sys_utime_args),
	    ibcs2_sys_utime },			/* 30 = utime */
	{ 0, 0,
	    sys_nosys },			/* 31 = unimplemented was stty */
	{ 0, 0,
	    sys_nosys },			/* 32 = unimplemented was gtty */
	{ 2, s(struct ibcs2_sys_access_args),
	    ibcs2_sys_access },			/* 33 = access */
	{ 1, s(struct ibcs2_sys_nice_args),
	    ibcs2_sys_nice },			/* 34 = nice */
	{ 4, s(struct ibcs2_sys_statfs_args),
	    ibcs2_sys_statfs },			/* 35 = statfs */
	{ 0, 0,
	    sys_sync },				/* 36 = sync */
	{ 2, s(struct ibcs2_sys_kill_args),
	    ibcs2_sys_kill },			/* 37 = kill */
	{ 4, s(struct ibcs2_sys_fstatfs_args),
	    ibcs2_sys_fstatfs },		/* 38 = fstatfs */
	{ 4, s(struct ibcs2_sys_pgrpsys_args),
	    ibcs2_sys_pgrpsys },		/* 39 = pgrpsys */
	{ 0, 0,
	    sys_nosys },			/* 40 = unimplemented ibcs2_xenix */
	{ 1, s(struct sys_dup_args),
	    sys_dup },				/* 41 = dup */
	{ 0, 0,
	    sys_pipe },				/* 42 = pipe */
	{ 1, s(struct ibcs2_sys_times_args),
	    ibcs2_sys_times },			/* 43 = times */
	{ 0, 0,
	    sys_nosys },			/* 44 = unimplemented profil */
	{ 1, s(struct ibcs2_sys_plock_args),
	    ibcs2_sys_plock },			/* 45 = plock */
	{ 1, s(struct ibcs2_sys_setgid_args),
	    ibcs2_sys_setgid },			/* 46 = setgid */
	{ 0, 0,
	    sys_getgid },			/* 47 = getgid */
	{ 2, s(struct ibcs2_sys_sigsys_args),
	    ibcs2_sys_sigsys },			/* 48 = sigsys */
#ifdef SYSVMSG
	{ 6, s(struct ibcs2_sys_msgsys_args),
	    ibcs2_sys_msgsys },			/* 49 = msgsys */
#else
	{ 0, 0,
	    sys_nosys },			/* 49 = unimplemented msgsys */
#endif
	{ 2, s(struct ibcs2_sys_sysmachine_args),
	    ibcs2_sys_sysmachine },		/* 50 = sysmachine */
	{ 0, 0,
	    sys_nosys },			/* 51 = unimplemented ibcs2_acct */
#ifdef SYSVSHM
	{ 4, s(struct ibcs2_sys_shmsys_args),
	    ibcs2_sys_shmsys },			/* 52 = shmsys */
#else
	{ 0, 0,
	    sys_nosys },			/* 52 = unimplemented shmsys */
#endif
#ifdef SYSVSEM
	{ 5, s(struct ibcs2_sys_semsys_args),
	    ibcs2_sys_semsys },			/* 53 = semsys */
#else
	{ 0, 0,
	    sys_nosys },			/* 53 = unimplemented semsys */
#endif
	{ 3, s(struct ibcs2_sys_ioctl_args),
	    ibcs2_sys_ioctl },			/* 54 = ioctl */
	{ 3, s(struct ibcs2_sys_uadmin_args),
	    ibcs2_sys_uadmin },			/* 55 = uadmin */
	{ 0, 0,
	    sys_nosys },			/* 56 = unimplemented */
	{ 3, s(struct ibcs2_sys_utssys_args),
	    ibcs2_sys_utssys },			/* 57 = utssys */
	{ 1, s(struct sys_fsync_args),
	    sys_fsync },			/* 58 = fsync */
	{ 3, s(struct ibcs2_sys_execve_args),
	    ibcs2_sys_execve },			/* 59 = execve */
	{ 1, s(struct sys_umask_args),
	    sys_umask },			/* 60 = umask */
	{ 1, s(struct sys_chroot_args),
	    sys_chroot },			/* 61 = chroot */
	{ 3, s(struct ibcs2_sys_fcntl_args),
	    ibcs2_sys_fcntl },			/* 62 = fcntl */
	{ 2, s(struct ibcs2_sys_ulimit_args),
	    ibcs2_sys_ulimit },			/* 63 = ulimit */
	{ 0, 0,
	    sys_nosys },			/* 64 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 65 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 66 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 67 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 68 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 69 = unimplemented reserved for unix/pc */
	{ 0, 0,
	    sys_nosys },			/* 70 = obsolete rfs_advfs */
	{ 0, 0,
	    sys_nosys },			/* 71 = obsolete rfs_unadvfs */
	{ 0, 0,
	    sys_nosys },			/* 72 = obsolete rfs_rmount */
	{ 0, 0,
	    sys_nosys },			/* 73 = obsolete rfs_rumount */
	{ 0, 0,
	    sys_nosys },			/* 74 = obsolete rfs_rfstart */
	{ 0, 0,
	    sys_nosys },			/* 75 = obsolete rfs_sigret */
	{ 0, 0,
	    sys_nosys },			/* 76 = obsolete rfs_rdebug */
	{ 0, 0,
	    sys_nosys },			/* 77 = obsolete rfs_rfstop */
	{ 0, 0,
	    sys_nosys },			/* 78 = unimplemented rfs_rfsys */
	{ 1, s(struct ibcs2_sys_rmdir_args),
	    ibcs2_sys_rmdir },			/* 79 = rmdir */
	{ 2, s(struct ibcs2_sys_mkdir_args),
	    ibcs2_sys_mkdir },			/* 80 = mkdir */
	{ 3, s(struct ibcs2_sys_getdents_args),
	    ibcs2_sys_getdents },		/* 81 = getdents */
	{ 0, 0,
	    sys_nosys },			/* 82 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 83 = unimplemented */
	{ 3, s(struct ibcs2_sys_sysfs_args),
	    ibcs2_sys_sysfs },			/* 84 = sysfs */
	{ 4, s(struct ibcs2_sys_getmsg_args),
	    ibcs2_sys_getmsg },			/* 85 = getmsg */
	{ 4, s(struct ibcs2_sys_putmsg_args),
	    ibcs2_sys_putmsg },			/* 86 = putmsg */
	{ 3, s(struct sys_poll_args),
	    sys_poll },				/* 87 = poll */
	{ 0, 0,
	    sys_nosys },			/* 88 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 89 = unimplemented */
	{ 2, s(struct ibcs2_sys_symlink_args),
	    ibcs2_sys_symlink },		/* 90 = symlink */
	{ 2, s(struct ibcs2_sys_lstat_args),
	    ibcs2_sys_lstat },			/* 91 = lstat */
	{ 3, s(struct ibcs2_sys_readlink_args),
	    ibcs2_sys_readlink },		/* 92 = readlink */
	{ 2, s(struct sys_fchmod_args),
	    sys_fchmod },			/* 93 = fchmod */
	{ 3, s(struct sys___posix_fchown_args),
	    sys___posix_fchown },		/* 94 = fchown */
	{ 0, 0,
	    sys_nosys },			/* 95 = unimplemented */
	{ 1, s(struct sys___sigreturn14_args),
	    sys___sigreturn14 },		/* 96 = sigreturn */
	{ 2, s(struct ibcs2_sys_sigaltstack_args),
	    ibcs2_sys_sigaltstack },		/* 97 = sigaltstack */
	{ 0, 0,
	    sys_nosys },			/* 98 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 99 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 100 = unimplemented getcontext/setcontext/sigsetjmp */
	{ 0, 0,
	    sys_nosys },			/* 101 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 102 = unimplemented */
	{ 2, s(struct ibcs2_sys_statvfs_args),
	    ibcs2_sys_statvfs },		/* 103 = statvfs */
	{ 2, s(struct ibcs2_sys_fstatvfs_args),
	    ibcs2_sys_fstatvfs },		/* 104 = fstatvfs */
	{ 0, 0,
	    sys_nosys },			/* 105 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 106 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 107 = unimplemented waitid */
	{ 0, 0,
	    sys_nosys },			/* 108 = unimplemented sigsendset */
	{ 0, 0,
	    sys_nosys },			/* 109 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 110 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 111 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 112 = unimplemented priocntl */
	{ 0, 0,
	    sys_nosys },			/* 113 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 114 = unimplemented */
	{ 6, s(struct ibcs2_sys_mmap_args),
	    ibcs2_sys_mmap },			/* 115 = mmap */
	{ 3, s(struct sys_mprotect_args),
	    sys_mprotect },			/* 116 = mprotect */
	{ 2, s(struct sys_munmap_args),
	    sys_munmap },			/* 117 = munmap */
	{ 0, 0,
	    sys_nosys },			/* 118 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 119 = unimplemented vfork */
	{ 1, s(struct sys_fchdir_args),
	    sys_fchdir },			/* 120 = fchdir */
	{ 3, s(struct sys_readv_args),
	    sys_readv },			/* 121 = readv */
	{ 3, s(struct sys_writev_args),
	    sys_writev },			/* 122 = writev */
	{ 0, 0,
	    sys_nosys },			/* 123 = unimplemented xstat */
	{ 0, 0,
	    sys_nosys },			/* 124 = unimplemented lxstat */
	{ 0, 0,
	    sys_nosys },			/* 125 = unimplemented fxstat */
	{ 0, 0,
	    sys_nosys },			/* 126 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 127 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 128 = unimplemented setrlimit */
	{ 0, 0,
	    sys_nosys },			/* 129 = unimplemented getrlimit */
	{ 0, 0,
	    sys_nosys },			/* 130 = unimplemented lchown */
	{ 6, s(struct ibcs2_sys_memcntl_args),
	    ibcs2_sys_memcntl },		/* 131 = memcntl */
	{ 0, 0,
	    sys_nosys },			/* 132 = unimplemented getpmsg */
	{ 0, 0,
	    sys_nosys },			/* 133 = unimplemented putpmsg */
	{ 0, 0,
	    sys_nosys },			/* 134 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 135 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 136 = unimplemented setegid */
	{ 0, 0,
	    sys_nosys },			/* 137 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 138 = unimplemented adjtime */
	{ 0, 0,
	    sys_nosys },			/* 139 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 140 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 141 = unimplemented seteuid */
	{ 0, 0,
	    sys_nosys },			/* 142 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 143 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 144 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 145 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 146 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 147 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 148 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 149 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 150 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 151 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 152 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 153 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 154 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 155 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 156 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 157 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 158 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 159 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 160 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 161 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 162 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 163 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 164 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 165 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 166 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 167 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 168 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 169 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 170 = unimplemented */
	{ 1, s(struct ibcs2_sys_gettimeofday_args),
	    ibcs2_sys_gettimeofday },		/* 171 = gettimeofday */
	{ 1, s(struct ibcs2_sys_settimeofday_args),
	    ibcs2_sys_settimeofday },		/* 172 = settimeofday */
	{ 0, 0,
	    sys_nosys },			/* 173 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 174 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 175 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 176 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 177 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 178 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 179 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 180 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 181 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 182 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 183 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 184 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 185 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 186 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 187 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 188 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 189 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 190 = unimplemented */
	{ 2, s(struct compat_43_sys_truncate_args),
	    compat_43_sys_truncate },		/* 191 = truncate */
	{ 2, s(struct compat_43_sys_ftruncate_args),
	    compat_43_sys_ftruncate },		/* 192 = ftruncate */
	{ 0, 0,
	    sys_nosys },			/* 193 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 194 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 195 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 196 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 197 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 198 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 199 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 200 = unimplemented */
	{ 3, s(struct xenix_sys_locking_args),
	    xenix_sys_locking },		/* 201 = locking */
	{ 0, 0,
	    sys_nosys },			/* 202 = unimplemented xenix_creatsem */
	{ 0, 0,
	    sys_nosys },			/* 203 = unimplemented xenix_opensem */
	{ 0, 0,
	    sys_nosys },			/* 204 = unimplemented xenix_sigsem */
	{ 0, 0,
	    sys_nosys },			/* 205 = unimplemented xenix_waitsem */
	{ 0, 0,
	    sys_nosys },			/* 206 = unimplemented xenix_nbwaitsem */
	{ 1, s(struct xenix_sys_rdchk_args),
	    xenix_sys_rdchk },			/* 207 = rdchk */
	{ 0, 0,
	    sys_nosys },			/* 208 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 209 = unimplemented */
	{ 2, s(struct xenix_sys_chsize_args),
	    xenix_sys_chsize },			/* 210 = chsize */
	{ 1, s(struct xenix_sys_ftime_args),
	    xenix_sys_ftime },			/* 211 = ftime */
	{ 1, s(struct xenix_sys_nap_args),
	    xenix_sys_nap },			/* 212 = nap */
	{ 0, 0,
	    sys_nosys },			/* 213 = unimplemented xenix_sdget */
	{ 0, 0,
	    sys_nosys },			/* 214 = unimplemented xenix_sdfree */
	{ 0, 0,
	    sys_nosys },			/* 215 = unimplemented xenix_sdenter */
	{ 0, 0,
	    sys_nosys },			/* 216 = unimplemented xenix_sdleave */
	{ 0, 0,
	    sys_nosys },			/* 217 = unimplemented xenix_sdgetv */
	{ 0, 0,
	    sys_nosys },			/* 218 = unimplemented xenix_sdwaitv */
	{ 0, 0,
	    sys_nosys },			/* 219 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 220 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 221 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 222 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 223 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 224 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 225 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 226 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 227 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 228 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 229 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 230 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 231 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 232 = unimplemented xenix_proctl */
	{ 0, 0,
	    sys_nosys },			/* 233 = unimplemented xenix_execseg */
	{ 0, 0,
	    sys_nosys },			/* 234 = unimplemented xenix_unexecseg */
	{ 0, 0,
	    sys_nosys },			/* 235 = unimplemented */
	{ 5, s(struct sys_select_args),
	    sys_select },			/* 236 = select */
	{ 2, s(struct ibcs2_sys_eaccess_args),
	    ibcs2_sys_eaccess },		/* 237 = eaccess */
	{ 0, 0,
	    sys_nosys },			/* 238 = unimplemented xenix_paccess */
	{ 3, s(struct ibcs2_sys_sigaction_args),
	    ibcs2_sys_sigaction },		/* 239 = sigaction */
	{ 3, s(struct ibcs2_sys_sigprocmask_args),
	    ibcs2_sys_sigprocmask },		/* 240 = sigprocmask */
	{ 1, s(struct ibcs2_sys_sigpending_args),
	    ibcs2_sys_sigpending },		/* 241 = sigpending */
	{ 1, s(struct ibcs2_sys_sigsuspend_args),
	    ibcs2_sys_sigsuspend },		/* 242 = sigsuspend */
	{ 2, s(struct ibcs2_sys_getgroups_args),
	    ibcs2_sys_getgroups },		/* 243 = getgroups */
	{ 2, s(struct ibcs2_sys_setgroups_args),
	    ibcs2_sys_setgroups },		/* 244 = setgroups */
	{ 1, s(struct ibcs2_sys_sysconf_args),
	    ibcs2_sys_sysconf },		/* 245 = sysconf */
	{ 2, s(struct ibcs2_sys_pathconf_args),
	    ibcs2_sys_pathconf },		/* 246 = pathconf */
	{ 2, s(struct ibcs2_sys_fpathconf_args),
	    ibcs2_sys_fpathconf },		/* 247 = fpathconf */
	{ 2, s(struct ibcs2_sys_rename_args),
	    ibcs2_sys_rename },			/* 248 = rename */
	{ 0, 0,
	    sys_nosys },			/* 249 = unimplemented */
	{ 2, s(struct ibcs2_sys_scoinfo_args),
	    ibcs2_sys_scoinfo },		/* 250 = scoinfo */
	{ 0, 0,
	    sys_nosys },			/* 251 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 252 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 253 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 254 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 255 = unimplemented getitimer */
	{ 0, 0,
	    sys_nosys },			/* 256 = unimplemented setitimer */
	{ 0, 0,
	    sys_nosys },			/* 257 = unimplemented */
	{ 0, 0,
	    sys_nosys },			/* 258 = unimplemented setreuid */
	{ 0, 0,
	    sys_nosys },			/* 259 = unimplemented setregid */
};
