/*	$NetBSD: osf1_syscallargs.h,v 1.36 1999/05/10 06:00:11 cgd Exp $	*/

/*
 * System call argument lists.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	NetBSD: syscalls.master,v 1.30 1999/05/10 05:58:44 cgd Exp
 */

#ifndef _OSF1_SYS__SYSCALLARGS_H_
#define _OSF1_SYS__SYSCALLARGS_H_

#ifdef	syscallarg
#undef	syscallarg
#endif

#define	syscallarg(x)								\
		union {								\
			register_t pad;						\
			struct { x datum; } le;					\
			struct {						\
				int8_t pad[ (sizeof (register_t) < sizeof (x))	\
					? 0					\
					: sizeof (register_t) - sizeof (x)];	\
				x datum;					\
			} be;							\
		}

struct osf1_sys_wait4_args {
	syscallarg(int) pid;
	syscallarg(int *) status;
	syscallarg(int) options;
	syscallarg(struct osf1_rusage *) rusage;
};

struct osf1_sys_mknod_args {
	syscallarg(const char *) path;
	syscallarg(int) mode;
	syscallarg(int) dev;
};

struct osf1_sys_getfsstat_args {
	syscallarg(struct osf1_statfs *) buf;
	syscallarg(long) bufsize;
	syscallarg(int) flags;
};

struct osf1_sys_lseek_args {
	syscallarg(int) fd;
	syscallarg(off_t) offset;
	syscallarg(int) whence;
};

struct osf1_sys_mount_args {
	syscallarg(int) type;
	syscallarg(const char *) path;
	syscallarg(int) flags;
	syscallarg(caddr_t) data;
};

struct osf1_sys_unmount_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
};

struct osf1_sys_setuid_args {
	syscallarg(uid_t) uid;
};

struct osf1_sys_recvmsg_xopen_args {
	syscallarg(int) s;
	syscallarg(struct osf1_msghdr_xopen *) msg;
	syscallarg(int) flags;
};

struct osf1_sys_sendmsg_xopen_args {
	syscallarg(int) s;
	syscallarg(const struct osf1_msghdr_xopen *) msg;
	syscallarg(int) flags;
};

struct osf1_sys_access_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
};

struct osf1_sys_set_program_attributes_args {
	syscallarg(caddr_t) taddr;
	syscallarg(unsigned long) tsize;
	syscallarg(caddr_t) daddr;
	syscallarg(unsigned long) dsize;
};

struct osf1_sys_open_args {
	syscallarg(const char *) path;
	syscallarg(int) flags;
	syscallarg(int) mode;
};

struct osf1_sys_classcntl_args {
	syscallarg(int) opcode;
	syscallarg(int) arg1;
	syscallarg(int) arg2;
	syscallarg(int) arg3;
};

struct osf1_sys_ioctl_args {
	syscallarg(int) fd;
	syscallarg(int) com;
	syscallarg(caddr_t) data;
};

struct osf1_sys_reboot_args {
	syscallarg(int) opt;
};

struct osf1_sys_execve_args {
	syscallarg(const char *) path;
	syscallarg(char *const *) argp;
	syscallarg(char *const *) envp;
};

struct osf1_sys_stat_args {
	syscallarg(const char *) path;
	syscallarg(struct osf1_stat *) ub;
};

struct osf1_sys_lstat_args {
	syscallarg(const char *) path;
	syscallarg(struct osf1_stat *) ub;
};

struct osf1_sys_mmap_args {
	syscallarg(caddr_t) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
	syscallarg(int) flags;
	syscallarg(int) fd;
	syscallarg(off_t) pos;
};

struct osf1_sys_mprotect_args {
	syscallarg(void *) addr;
	syscallarg(size_t) len;
	syscallarg(int) prot;
};

struct osf1_sys_madvise_args {
	syscallarg(void *) addr;
	syscallarg(size_t) len;
	syscallarg(int) behav;
};

struct osf1_sys_setitimer_args {
	syscallarg(u_int) which;
	syscallarg(struct osf1_itimerval *) itv;
	syscallarg(struct osf1_itimerval *) oitv;
};

struct osf1_sys_fstat_args {
	syscallarg(int) fd;
	syscallarg(void *) sb;
};

struct osf1_sys_fcntl_args {
	syscallarg(int) fd;
	syscallarg(int) cmd;
	syscallarg(void *) arg;
};

struct osf1_sys_select_args {
	syscallarg(u_int) nd;
	syscallarg(fd_set *) in;
	syscallarg(fd_set *) ou;
	syscallarg(fd_set *) ex;
	syscallarg(struct osf1_timeval *) tv;
};

struct osf1_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};

struct osf1_sys_gettimeofday_args {
	syscallarg(struct osf1_timeval *) tp;
	syscallarg(struct osf1_timezone *) tzp;
};

struct osf1_sys_getrusage_args {
	syscallarg(int) who;
	syscallarg(struct osf1_rusage *) rusage;
};

struct osf1_sys_readv_args {
	syscallarg(int) fd;
	syscallarg(struct osf1_iovec *) iovp;
	syscallarg(u_int) iovcnt;
};

struct osf1_sys_writev_args {
	syscallarg(int) fd;
	syscallarg(struct osf1_iovec *) iovp;
	syscallarg(u_int) iovcnt;
};

struct osf1_sys_settimeofday_args {
	syscallarg(struct osf1_timeval *) tv;
	syscallarg(struct osf1_timezone *) tzp;
};

struct osf1_sys_truncate_args {
	syscallarg(const char *) path;
	syscallarg(off_t) length;
};

struct osf1_sys_ftruncate_args {
	syscallarg(int) fd;
	syscallarg(off_t) length;
};

struct osf1_sys_setgid_args {
	syscallarg(gid_t) gid;
};

struct osf1_sys_sendto_args {
	syscallarg(int) s;
	syscallarg(caddr_t) buf;
	syscallarg(size_t) len;
	syscallarg(int) flags;
	syscallarg(struct sockaddr *) to;
	syscallarg(int) tolen;
};

struct osf1_sys_socketpair_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
	syscallarg(int *) rsv;
};

struct osf1_sys_utimes_args {
	syscallarg(const char *) path;
	syscallarg(const struct osf1_timeval *) tptr;
};

struct osf1_sys_getrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct rlimit *) rlp;
};

struct osf1_sys_setrlimit_args {
	syscallarg(u_int) which;
	syscallarg(struct rlimit *) rlp;
};

struct osf1_sys_sigaction_args {
	syscallarg(int) signum;
	syscallarg(struct osf1_sigaction *) nsa;
	syscallarg(struct osf1_sigaction *) osa;
};

struct osf1_sys_statfs_args {
	syscallarg(const char *) path;
	syscallarg(struct osf1_statfs *) buf;
	syscallarg(int) len;
};

struct osf1_sys_fstatfs_args {
	syscallarg(int) fd;
	syscallarg(struct osf1_statfs *) buf;
	syscallarg(int) len;
};

struct osf1_sys_uname_args {
	syscallarg(struct osf1_uname *) name;
};

struct osf1_sys_shmat_args {
	syscallarg(int) shmid;
	syscallarg(const void *) shmaddr;
	syscallarg(int) shmflg;
};

struct osf1_sys_shmctl_args {
	syscallarg(int) shmid;
	syscallarg(int) cmd;
	syscallarg(struct osf1_shmid_ds *) buf;
};

struct osf1_sys_shmdt_args {
	syscallarg(const void *) shmaddr;
};

struct osf1_sys_shmget_args {
	syscallarg(osf1_key_t) key;
	syscallarg(size_t) size;
	syscallarg(int) flags;
};

struct osf1_sys_sigaltstack_args {
	syscallarg(struct osf1_sigaltstack *) nss;
	syscallarg(struct osf1_sigaltstack *) oss;
};

struct osf1_sys_sysinfo_args {
	syscallarg(int) cmd;
	syscallarg(char *) buf;
	syscallarg(long) len;
};

struct osf1_sys_pathconf_args {
	syscallarg(const char *) path;
	syscallarg(int) name;
};

struct osf1_sys_fpathconf_args {
	syscallarg(int) fd;
	syscallarg(int) name;
};

struct osf1_sys_usleep_thread_args {
	syscallarg(struct osf1_timeval *) sleep;
	syscallarg(struct osf1_timeval *) slept;
};

struct osf1_sys_setsysinfo_args {
	syscallarg(u_long) op;
	syscallarg(caddr_t) buffer;
	syscallarg(u_long) nbytes;
	syscallarg(caddr_t) arg;
	syscallarg(u_long) flag;
};

/*
 * System call prototypes.
 */

int	sys_nosys	__P((struct proc *, void *, register_t *));
int	sys_exit	__P((struct proc *, void *, register_t *));
int	sys_fork	__P((struct proc *, void *, register_t *));
int	sys_read	__P((struct proc *, void *, register_t *));
int	sys_write	__P((struct proc *, void *, register_t *));
int	sys_close	__P((struct proc *, void *, register_t *));
int	osf1_sys_wait4	__P((struct proc *, void *, register_t *));
int	sys_link	__P((struct proc *, void *, register_t *));
int	sys_unlink	__P((struct proc *, void *, register_t *));
int	sys_chdir	__P((struct proc *, void *, register_t *));
int	sys_fchdir	__P((struct proc *, void *, register_t *));
int	osf1_sys_mknod	__P((struct proc *, void *, register_t *));
int	sys_chmod	__P((struct proc *, void *, register_t *));
int	sys___posix_chown	__P((struct proc *, void *, register_t *));
int	sys_obreak	__P((struct proc *, void *, register_t *));
int	osf1_sys_getfsstat	__P((struct proc *, void *, register_t *));
int	osf1_sys_lseek	__P((struct proc *, void *, register_t *));
int	sys_getpid	__P((struct proc *, void *, register_t *));
int	osf1_sys_mount	__P((struct proc *, void *, register_t *));
int	osf1_sys_unmount	__P((struct proc *, void *, register_t *));
int	osf1_sys_setuid	__P((struct proc *, void *, register_t *));
int	sys_getuid	__P((struct proc *, void *, register_t *));
int	osf1_sys_recvmsg_xopen	__P((struct proc *, void *, register_t *));
int	osf1_sys_sendmsg_xopen	__P((struct proc *, void *, register_t *));
int	osf1_sys_access	__P((struct proc *, void *, register_t *));
int	sys_sync	__P((struct proc *, void *, register_t *));
int	sys_kill	__P((struct proc *, void *, register_t *));
int	sys_setpgid	__P((struct proc *, void *, register_t *));
int	sys_dup	__P((struct proc *, void *, register_t *));
int	sys_pipe	__P((struct proc *, void *, register_t *));
int	osf1_sys_set_program_attributes	__P((struct proc *, void *, register_t *));
int	osf1_sys_open	__P((struct proc *, void *, register_t *));
int	sys_getgid	__P((struct proc *, void *, register_t *));
int	compat_13_sys_sigprocmask	__P((struct proc *, void *, register_t *));
int	sys___getlogin	__P((struct proc *, void *, register_t *));
int	sys_setlogin	__P((struct proc *, void *, register_t *));
int	sys_acct	__P((struct proc *, void *, register_t *));
int	osf1_sys_classcntl	__P((struct proc *, void *, register_t *));
int	osf1_sys_ioctl	__P((struct proc *, void *, register_t *));
int	osf1_sys_reboot	__P((struct proc *, void *, register_t *));
int	sys_revoke	__P((struct proc *, void *, register_t *));
int	sys_symlink	__P((struct proc *, void *, register_t *));
int	sys_readlink	__P((struct proc *, void *, register_t *));
int	osf1_sys_execve	__P((struct proc *, void *, register_t *));
int	sys_umask	__P((struct proc *, void *, register_t *));
int	sys_chroot	__P((struct proc *, void *, register_t *));
int	sys_getpgrp	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getpagesize	__P((struct proc *, void *, register_t *));
int	sys_vfork	__P((struct proc *, void *, register_t *));
int	osf1_sys_stat	__P((struct proc *, void *, register_t *));
int	osf1_sys_lstat	__P((struct proc *, void *, register_t *));
int	osf1_sys_mmap	__P((struct proc *, void *, register_t *));
int	sys_munmap	__P((struct proc *, void *, register_t *));
int	osf1_sys_mprotect	__P((struct proc *, void *, register_t *));
int	osf1_sys_madvise	__P((struct proc *, void *, register_t *));
int	sys_getgroups	__P((struct proc *, void *, register_t *));
int	sys_setgroups	__P((struct proc *, void *, register_t *));
int	sys_setpgid	__P((struct proc *, void *, register_t *));
int	osf1_sys_setitimer	__P((struct proc *, void *, register_t *));
int	compat_43_sys_gethostname	__P((struct proc *, void *, register_t *));
int	compat_43_sys_sethostname	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getdtablesize	__P((struct proc *, void *, register_t *));
int	sys_dup2	__P((struct proc *, void *, register_t *));
int	osf1_sys_fstat	__P((struct proc *, void *, register_t *));
int	osf1_sys_fcntl	__P((struct proc *, void *, register_t *));
int	osf1_sys_select	__P((struct proc *, void *, register_t *));
int	sys_poll	__P((struct proc *, void *, register_t *));
int	sys_fsync	__P((struct proc *, void *, register_t *));
int	sys_setpriority	__P((struct proc *, void *, register_t *));
int	osf1_sys_socket	__P((struct proc *, void *, register_t *));
int	sys_connect	__P((struct proc *, void *, register_t *));
int	compat_43_sys_accept	__P((struct proc *, void *, register_t *));
int	sys_getpriority	__P((struct proc *, void *, register_t *));
int	compat_43_sys_send	__P((struct proc *, void *, register_t *));
int	compat_43_sys_recv	__P((struct proc *, void *, register_t *));
int	compat_13_sys_sigreturn	__P((struct proc *, void *, register_t *));
int	sys_bind	__P((struct proc *, void *, register_t *));
int	sys_setsockopt	__P((struct proc *, void *, register_t *));
int	sys_listen	__P((struct proc *, void *, register_t *));
int	compat_13_sys_sigsuspend	__P((struct proc *, void *, register_t *));
int	compat_43_sys_sigstack	__P((struct proc *, void *, register_t *));
int	osf1_sys_gettimeofday	__P((struct proc *, void *, register_t *));
int	osf1_sys_getrusage	__P((struct proc *, void *, register_t *));
int	sys_getsockopt	__P((struct proc *, void *, register_t *));
int	osf1_sys_readv	__P((struct proc *, void *, register_t *));
int	osf1_sys_writev	__P((struct proc *, void *, register_t *));
int	osf1_sys_settimeofday	__P((struct proc *, void *, register_t *));
int	sys___posix_fchown	__P((struct proc *, void *, register_t *));
int	sys_fchmod	__P((struct proc *, void *, register_t *));
int	compat_43_sys_recvfrom	__P((struct proc *, void *, register_t *));
int	sys___posix_rename	__P((struct proc *, void *, register_t *));
int	osf1_sys_truncate	__P((struct proc *, void *, register_t *));
int	osf1_sys_ftruncate	__P((struct proc *, void *, register_t *));
int	osf1_sys_setgid	__P((struct proc *, void *, register_t *));
int	osf1_sys_sendto	__P((struct proc *, void *, register_t *));
int	sys_shutdown	__P((struct proc *, void *, register_t *));
int	osf1_sys_socketpair	__P((struct proc *, void *, register_t *));
int	sys_mkdir	__P((struct proc *, void *, register_t *));
int	sys_rmdir	__P((struct proc *, void *, register_t *));
int	osf1_sys_utimes	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getpeername	__P((struct proc *, void *, register_t *));
int	compat_43_sys_gethostid	__P((struct proc *, void *, register_t *));
int	compat_43_sys_sethostid	__P((struct proc *, void *, register_t *));
int	osf1_sys_getrlimit	__P((struct proc *, void *, register_t *));
int	osf1_sys_setrlimit	__P((struct proc *, void *, register_t *));
int	sys_setsid	__P((struct proc *, void *, register_t *));
int	compat_43_sys_quota	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getsockname	__P((struct proc *, void *, register_t *));
int	osf1_sys_sigaction	__P((struct proc *, void *, register_t *));
int	compat_43_sys_getdirentries	__P((struct proc *, void *, register_t *));
int	osf1_sys_statfs	__P((struct proc *, void *, register_t *));
int	osf1_sys_fstatfs	__P((struct proc *, void *, register_t *));
int	compat_09_sys_getdomainname	__P((struct proc *, void *, register_t *));
int	compat_09_sys_setdomainname	__P((struct proc *, void *, register_t *));
int	osf1_sys_uname	__P((struct proc *, void *, register_t *));
int	sys___posix_lchown	__P((struct proc *, void *, register_t *));
int	osf1_sys_shmat	__P((struct proc *, void *, register_t *));
int	osf1_sys_shmctl	__P((struct proc *, void *, register_t *));
int	osf1_sys_shmdt	__P((struct proc *, void *, register_t *));
int	osf1_sys_shmget	__P((struct proc *, void *, register_t *));
int	sys_getsid	__P((struct proc *, void *, register_t *));
int	osf1_sys_sigaltstack	__P((struct proc *, void *, register_t *));
int	osf1_sys_sysinfo	__P((struct proc *, void *, register_t *));
int	osf1_sys_pathconf	__P((struct proc *, void *, register_t *));
int	osf1_sys_fpathconf	__P((struct proc *, void *, register_t *));
int	osf1_sys_usleep_thread	__P((struct proc *, void *, register_t *));
int	osf1_sys_setsysinfo	__P((struct proc *, void *, register_t *));
#endif /* _OSF1_SYS__SYSCALLARGS_H_ */
