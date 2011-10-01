/*
 * Copyright© University of Tromsø, Norway, 2002
 *
 * Redistribution and use in whatever form, for whatever purpose, with
 *  or without modification, is permitted, provided that this copyright
 *  notice is retained unmodified.
 */

/*
 * Written by mouse@rodents.montreal.qc.ca for the Pasta project at the
 *  Institutt for Informatikk, Universitetet i Tromsø, Tromsø, Norge.
 */

/*
 * Filesystem-level PFS operations.
 */

#include <sys/file.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/mount.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/unpcb.h>
#include <sys/malloc.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/filedesc.h>
#include <sys/socketvar.h>

#include <miscfs/pfs/pfs.h>
#include <miscfs/pfs/pfs-internal.h>

/*
 * Mount a PFS filesystem.  The interface is described in pfs.h.
 *
 * We arguably should require just PR_ATOMIC and PR_RIGHTS be set in
 *  so->so_proto->pr_flags, but we then have trouble locating the peer
 *  socket to set its socket buffer sizes (as finding the peer is a
 *  protocol-specific operation, and one for which there is no defined
 *  interface).  So we punt: we require that the sockets be AF_LOCAL
 *  SOCK_DGRAM, and then pry into the unpcb.
 */
static int pfs_mount(struct mount *mp, const char *path, void *data, struct nameidata *ndp, struct proc *p)
{
 struct pfs_args args;
 struct pfs_1_3_args args_1_3;
 struct file *fp;
 struct socket *so;
 struct pfs_mount *pmp;
 struct vnode *vn;
 int err;
 size_t len;

 printf("pfs_mount\n");
 err = copyin(data,&args,sizeof(args));
 if (err)
  { printf("pfs_mount: args copyin failed\n");
    return(err);
  }
 printf("pfs_mount: version %d.%d\n",args.pfsa_major,args.pfsa_minor);
 if (args.pfsa_major == 0)
  { switch (args.pfsa_minor)
     { case 0:
	  args.pfsa_major = MAJOR_VERSION;
	  args.pfsa_minor = MINOR_VERSION;
	  printf("pfs_mount: returning %d.%d\n",MAJOR_VERSION,MINOR_VERSION);
	  err = copyout(&args,data,sizeof(args));
	  return(EINPROGRESS);
	  break;
     }
    printf("pfs_mount: bad minor for major 0\n");
    return(EINVAL);
  }
 if ( (args.pfsa_major != MAJOR_VERSION) ||
      (args.pfsa_minor != MINOR_VERSION) )
  { printf("pfs_mount: don't like version\n");
    return(EPROGMISMATCH);
  }
 if (mp->mnt_flag & MNT_UPDATE)
  { printf("pfs_mount: MNT_UPDATE unsupported\n");
    return(EOPNOTSUPP);
  }
 err = copyin(args.pfsa_data,&args_1_3,sizeof(args_1_3));
 if (err)
  { printf("pfs_mount: args_1_3 copyin failed\n");
    return(err);
  }
 /* getsock() will use the descriptor for us */
 err = getsock(p->p_fd,args_1_3.pfsa_socket,&fp);
 if (err)
  { printf("pfs_mount: getsock failed\n");
    return(err);
  }
 if (fp->f_type != DTYPE_SOCKET)
  { FILE_UNUSE(fp,0);
    printf("pfs_mount: not a socket\n");
    return(ENOTSOCK);
  }
 so = fp->f_data;
 FILE_UNUSE(fp,0);
 if (so->so_proto->pr_domain->dom_family != AF_LOCAL)
  { printf("pfs_mount: socket isn't AF_LOCAL\n");
    return(ESOCKTNOSUPPORT);
  }
 if (so->so_proto->pr_type != SOCK_DGRAM)
  { printf("pfs_mount: socket isn't SOCK_DGRAM\n");
    return(ESOCKTNOSUPPORT);
  }
 if (! (so->so_state & SS_ISCONNECTED))
  { printf("pfs_mount: socket not connected\n");
    return(ENOTCONN);
  }
 err = soreserve(so,PFS_REQ_MAX,PFS_REP_MAX);
 if (err)
  { printf("pfs_mount: soreserve 1 failed\n");
    return(err);
  }
 err = soreserve(sotounpcb(so)->unp_conn->unp_socket,PFS_REQ_MAX,PFS_REP_MAX);
 if (err)
  { printf("pfs_mount: soreserve 2 failed\n");
    return(err);
  }
 mp->mnt_flag |= MNT_LOCAL;
 pmp = malloc(sizeof(struct pfs_mount),M_PFSMNT,M_WAITOK);
 pmp->sock = fp;
 mp->mnt_data = pmp;
 pmp->root = 0;
 pfs_vntbl_init(pmp);
 pmp->vnode_count = 0;
 err = pfs_get_vnode(mp,&vn,PFS_ROOT_ID);
 if (err)
  { printf("pfs_mount: pfs_get_vnode failed\n");
    pfs_vntbl_done(pmp);
    free(pmp,M_PFSMNT);
    return(err);
  }
 VOP_UNLOCK(vn,0);
 pmp->root = vn;
 pmp->cred = p->p_ucred;
 crhold(pmp->cred);
 vn->v_flag |= VROOT;
 pfs_set_type(vn,VDIR);
 fp->f_count ++;
 vfs_getnewfsid(mp,MOUNT_PFS);
 bzero(mp->mnt_stat.f_mntonname,MNAMELEN);
 bzero(mp->mnt_stat.f_mntfromname,MNAMELEN);
 copyinstr(path,&mp->mnt_stat.f_mntonname[0],MNAMELEN-1,&len);
 copyinstr(args_1_3.pfsa_string,&mp->mnt_stat.f_mntfromname[0],MNAMELEN-1,&len);
 fdrelease(p,args_1_3.pfsa_socket);
 printf("pfs_mount: success\n");
 return(0);
}

/*
 * Start method.  Nothing to do here.
 */
static int pfs_start(struct mount *mp __attribute__((__unused__)), int flags __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 printf("pfs_start\n");
 return(0);
}

/*
 * Unmount a PFS filesystem.
 *
 * Because we keep parent pointer references and take no steps to
 *  ensure that the order vflush will process nodes is leaves-first, it
 *  may take multiple passes to clean out everything that can be.  So
 *  when flushing, keep looping as long as we (a) keep getting EBUSY
 *  and (b) keep making progress.
 *
 * Once we've flushed and error-checked, just notify userland and then
 *  tear down the mount.
 */
static int pfs_unmount(struct mount *mp, int flags, struct proc *p)
{
 struct vnode *root;
 struct file *f;
 int err;
 int ocount;

 printf("pfs_unmount\n");
 root = VFSTOPFS(mp)->root;
 do
  { ocount = VFSTOPFS(mp)->vnode_count;
    err = vflush(mp,root,(flags&MNT_FORCE)?FORCECLOSE:0);
    if (err && ((err != EBUSY) || (VFSTOPFS(mp)->vnode_count) >= ocount))
     { printf("pfs_unmount: vflush failed\n");
       return(err);
     }
  } while (err);
 if (VFSTOPFS(mp)->vnode_count != 1)
  { printf("pfs_unmount: busy (vnode count %d)\n",VFSTOPFS(mp)->vnode_count);
    return(EBUSY);
  }
 if (root->v_usecount != 1)
  { printf("\nvnode count 1 but root usecount %ld\n\n",root->v_usecount);
  }
 VFSTOPFS(mp)->root = 0;
 pfs_call(mp,PFSOP_DONE,0,0,0,PFS_NO_REPLY,0,0);
 vrele(root);
 vgone(root);
 f = VFSTOPFS(mp)->sock;
 FILE_USE(f);
 soshutdown(f->f_data,SHUT_RDWR);
 closef(f,0);
 crfree(VFSTOPFS(mp)->cred);
 free(VFSTOPFS(mp),M_PFSMNT);
 mp->mnt_data = 0;
 printf("pfs_unmount: success\n");
 return(0);
}

/*
 * Return the (locked) root vnode for a PFS filesystem.
 */
static int pfs_root(struct mount *mp, struct vnode **rvp)
{
 struct vnode *v;

 printf("pfs_root\n");
 v = VFSTOPFS(mp)->root;
 VREF(v);
 vn_lock(v,LK_EXCLUSIVE|LK_RETRY);
 *rvp = v;
 return(0);
}

/*
 * We don't support quotas, so quotactl is a pretty boring.
 */
static int pfs_quotactl(struct mount *mp __attribute__((__unused__)), int cmd __attribute__((__unused__)), uid_t uid __attribute__((__unused__)), caddr_t arg __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 printf("pfs_quotactl\n");
 return(EOPNOTSUPP);
}

/*
 * statfs() for PFS filesystems.  Not much to do here; just call to
 *  userland.
 */
static int pfs_statfs(struct mount *mp, struct statfs *stbp, struct proc *p)
{
 int err;
 struct statfs stb;

 printf("pfs_statfs\n");
 err = pfs_call(mp,PFSOP_STATFS,0,0,&stb,sizeof(stb),0,0);
 if (err) return(err);
 stb.f_type = 0;
 stb.f_flags = stbp->f_flags;
 stb.f_iosize = DEV_BSIZE;
 stb.f_fsid = mp->mnt_stat.f_fsid;
 stb.f_owner = stbp->f_owner;
 strncpy(&stb.f_fstypename[0],mp->mnt_op->vfs_name,MFSNAMELEN);
 bcopy(&mp->mnt_stat.f_mntonname[0],&stb.f_mntonname[0],MNAMELEN);
 bcopy(&mp->mnt_stat.f_mntfromname[0],&stb.f_mntfromname[0],MNAMELEN);
 *stbp = stb;
 return(0);
}

/*
 * sync() for PFS filesystems.  Just notify userland; we don't keep
 *  anything cached ourselves.
 */
static int pfs_sync(struct mount *mp __attribute__((__unused__)), int w __attribute__((__unused__)), struct ucred *cred __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 printf("pfs_sync (root usecount %ld)\n",VFSTOPFS(mp)->root->v_usecount);
 pfs_call(mp,PFSOP_SYNC,0,0,0,PFS_NO_REPLY,0,0);
 return(0);
}

/*
 * I'm not entirely certain what vget is for; I suspect it's related to
 *  NFS support.  I haven't yet met anything that fails because of
 *  this; if someday someone does, a vget may have to be written.
 */
static int pfs_vget(struct mount *mp __attribute__((__unused__)), ino_t inum __attribute__((__unused__)), struct vnode **vpp __attribute__((__unused__)))
{
 printf("pfs_vget\n");
 return(EOPNOTSUPP);
}

/*
 * Like vget, I'm not entirely sure what this does, but I'm fairly
 *  sure, in this case, that it's NFS support.  We don't do NFS
 *  exporting.
 */
static int pfs_fhtovp(struct mount *mp __attribute__((__unused__)), struct fid *fh __attribute__((__unused__)), struct vnode **vpp __attribute__((__unused__)))
{
 printf("pfs_fhtovp\n");
 return(EOPNOTSUPP);
}

/*
 * Like vget, I'm not entirely sure what this does, but I'm fairly
 *  sure, in this case, that it's NFS support.  We don't do NFS
 *  exporting.
 */
static int pfs_vptofh(struct vnode *vp __attribute__((__unused__)), struct fid *fh __attribute__((__unused__)))
{
 printf("pfs_vptofh\n");
 return(EOPNOTSUPP);
}

/*
 * Initialize any mount-point-independent PFS structures.  Currently
 *  nothing to do here.
 */
static void pfs_init(void)
{
 printf("pfs_init\n");
}

/*
 * Check whether a PFS filesystem can be exported.  It can't.
 */
static int pfs_checkexp(struct mount *mp __attribute__((__unused__)), struct mbuf *m __attribute__((__unused__)), int *exflags __attribute__((__unused__)), struct ucred **anoncred __attribute__((__unused__)))
{
 printf("pfs_checkexp\n");
 return(EOPNOTSUPP);
}

/*
 * We don't export anything via sysctl.
 */
static int pfs_sysctl(int *mib __attribute__((__unused__)), u_int miblen __attribute__((__unused__)), void *old __attribute__((__unused__)), size_t *oldlenp __attribute__((__unused__)), void *new __attribute__((__unused__)), size_t newlen __attribute__((__unused__)), struct proc *p __attribute__((__unused__)))
{
 printf("pfs_sysctl\n");
 return(EOPNOTSUPP);
}

/*
 * We don't support root on PFS, of course; this is indicated by
 *  providing a nil pointer as the mountroot method.
 */
#define pfs_mountroot 0

/*
 * Infrastructure - glue between us and the generic kernel filesystem
 *  support code.
 */

extern struct vnodeopv_desc pfs_vnodeop_opv_desc;

static struct vnodeopv_desc *pfs_vnodeopv_descs[]
 = { &pfs_vnodeop_opv_desc, 0 };

struct vfsops pfs_vfsops
 = { MOUNT_PFS,
     pfs_mount,
     pfs_start,
     pfs_unmount,
     pfs_root,
     pfs_quotactl,
     pfs_statfs,
     pfs_sync,
     pfs_vget,
     pfs_fhtovp,
     pfs_vptofh,
     pfs_init,
     pfs_sysctl,
     pfs_mountroot,
     pfs_checkexp,
     &pfs_vnodeopv_descs[0] };
