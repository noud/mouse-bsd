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
 * Vnode-level PFS operations.
 *
 * A lot of these are uncommented; those would otherwise have nothing
 *  but comments saying something like "nothing notable here", which I
 *  don't bother with.
 */

#include <sys/vnode.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/malloc.h>
#include <sys/socketvar.h>
#include <uvm/uvm_extern.h>

#include <miscfs/genfs/genfs.h>

#include <miscfs/pfs/pfs.h>
#include <miscfs/pfs/pfs-internal.h>

/*
 * Set up by generic code based on pfs_vnodeop_desc, used by passing it
 *  to getnewvnode in pfs_get_vnode (pfs-subr.c).
 */
int (**pfs_vnodeop_p)(void *);

/* Convenience macro. */
#define ARGS(name) struct vop_##name##_args *ap __attribute__((__unused__))

/*
 * procfs comments that "[l]ocking isn't hard here, just poorly
 *  documented"; I'd say more like *un*documented.
 *
 * For CREATE and RENAME, it is possible to return a nil vnode pointer
 *  and still return no error - returning 0 does not work here, we have
 *  to return EJUSTRETURN.  (Why is this, and why isn't - where is - it
 *  documented?!)
 *
 * When doing lookups for other than LOOKUP, we can count on getting a
 *  followup call (ABORT if no other) whenever we return no error.
 *  (EJUSTRETURN counts as no error in this regard.)  Not that
 *  we-the-kernel care about this; we just pass aborts back to userland
 *  anyway.
 */
static int pfs_lookup(void *v)
{
 ARGS(lookup); /* struct vnode *a_dvp;
		  struct vnode **a_vpp;
		  struct componentname *a_cnp; */
 struct componentname *cn;
 int err;
 struct pfs_lookup_rep p;
 char *qbuf;
 struct pfs_lookup_req *q;
 struct vnode *vn;
 unsigned int qop;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 printf("pfs_lookup: dvp %s, op %ld, component %.*s\n",VTOPFS(ap->a_dvp)->pstr,(long int)cn->cn_nameiop,(int)cn->cn_namelen,cn->cn_nameptr);
 *ap->a_vpp = NULLVP;
 if ( (cn->cn_flags & ISLASTCN) &&
      (ap->a_dvp->v_mount->mnt_flag & MNT_RDONLY) &&
      (cn->cn_nameiop != LOOKUP) )
  { printf("pfs_lookup: write op, EROFS\n");
    return(EROFS);
  }
 if ((cn->cn_namelen == 1) && (cn->cn_nameptr[0] == '.'))
  { *ap->a_vpp = ap->a_dvp;
    VREF(ap->a_dvp);
    printf("pfs_lookup: . -> OK\n");
    return(0);
  }
 if (cn->cn_flags & ISDOTDOT)
  { struct pfsnode *pn;
    pn = VTOPFS(ap->a_dvp)->parent;
    if (! pn) panic("pfs_lookup: bogus .."); /* non-dir, or root */
    VOP_UNLOCK(ap->a_dvp,0);
    cn->cn_flags |= PDIRUNLOCK;
    err = vn_lock(pn->vnode,LK_EXCLUSIVE);
    if (err) return(err);
    if ((cn->cn_flags & (LOCKPARENT|ISLASTCN)) == (LOCKPARENT|ISLASTCN))
     { err = vn_lock(ap->a_dvp,LK_EXCLUSIVE);
       if (err == 0) cn->cn_flags &= ~PDIRUNLOCK;
     }
    if (err == 0)
     { VREF(pn->vnode);
       *ap->a_vpp = pn->vnode;
       printf("pfs_lookup: .. -> OK\n");
     }
    else
     { printf("pfs_lookup: .. -> err %d\n",err);
     }
    return(err);
  }
 qbuf = malloc(sizeof(struct pfs_lookup_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
 q = (void *) qbuf;
 q->id = VTOPFS(ap->a_dvp)->id;
 switch (cn->cn_nameiop & OPMASK)
  { case LOOKUP:
       qop = PFS_LKUP_LOOKUP;
       break;
    case CREATE:
       qop = (cn->cn_flags & ISLASTCN) ? PFS_LKUP_CREATE : PFS_LKUP_LOOKUP;
       break;
    case DELETE:
       qop = (cn->cn_flags & ISLASTCN) ? PFS_LKUP_DELETE : PFS_LKUP_LOOKUP;
       break;
    case RENAME:
       qop = (cn->cn_flags & ISLASTCN) ? PFS_LKUP_RENAME : PFS_LKUP_LOOKUP;
       break;
    default:
       panic("pfs_lookup: impossible cn_nameiop %#lx",(unsigned long int)cn->cn_nameiop);
       break;
  }
 if (cn->cn_flags & REQUIREDIR) qop |= PFS_LKUP_REQDIR;
 q->op = qop;
 SETCRED(&q->cred,cn->cn_cred);
 bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_lookup_req),cn->cn_namelen);
 err = pfs_call(ap->a_dvp->v_mount,PFSOP_LOOKUP,qbuf,sizeof(struct pfs_lookup_req)+cn->cn_namelen,&p,sizeof(p),0,0);
 free(qbuf,M_TEMP);
 if (err)
  { printf("pfs_lookup: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_lookup: userland returned error %d\n",p.err);
    return(p.err);
  }
 switch (p.id)
  { case PFS_NO_ID:
       printf("pfs_lookup returned PFS_NO_ID\n");
       vn = 0;
       switch (cn->cn_nameiop & OPMASK)
	{ case LOOKUP:
	  case DELETE:
	     return(ENOENT);
	     break;
	}
       break;
    case PFS_WHITE_ID:
       printf("pfs_lookup returned PFS_WHITE_ID\n");
       if (! (cn->cn_flags & DOWHITEOUT)) return(ENOENT);
       cn->cn_flags |= ISWHITEOUT;
       vn = 0;
       break;
    default:
       if ((p.id < PFS_MIN_ID) || (p.id > PFS_MAX_ID))
	{ printf("pfs_lookup: protocol error, id %d\n",p.id);
	  return(EIO);
	}
       printf("pfs_lookup returned id %d, type %d\n",p.id,(int)p.type);
       switch (p.type)
	{ case VREG: case VDIR: case VBLK: case VCHR:
	  case VLNK: case VSOCK: case VFIFO:
	     break;
	  default:
	     printf("pfs_lookup: protocol error, type %d\n",(int)p.type);
	     return(EIO);
	     break;
	}
       printf("pfs_lookup calling pfs_get_vnode\n");
       err = pfs_get_vnode(ap->a_dvp->v_mount,&vn,p.id);
       if (err)
	{ printf("pfs_lookup: pfs_get_vnode failed\n");
	  return(err);
	}
       printf("pfs_lookup got %p\n",(void *)vn);
       if (vn->v_type == VNON)
	{ pfs_set_type(vn,p.type);
	  if (p.type == VDIR)
	   { printf("pfs_lookup adding VDIR parent pointer, %s->%s\n",VTOPFS(vn)->pstr,VTOPFS(ap->a_dvp)->pstr);
	     VREF(ap->a_dvp);
	     VTOPFS(vn)->parent = VTOPFS(ap->a_dvp);
	   }
	}
       else if (vn->v_type != p.type)
	{ printf("pfs_lookup: protocol error: id %d changed type (%d to %d)\n",p.id,vn->v_type,p.type);
	  VOP_UNLOCK(vn,0);
	  return(EIO);
	}
       break;
  }
 /* don't lock vn here; pfs_get_vnode returns it locked */
 if ((cn->cn_flags & (LOCKPARENT|ISLASTCN)) != (LOCKPARENT|ISLASTCN))
  { printf("pfs_lookup unlocking parent %s\n",VTOPFS(ap->a_dvp)->pstr);
    VOP_UNLOCK(ap->a_dvp,0);
    cn->cn_flags |= PDIRUNLOCK;
  }
 *ap->a_vpp = vn;
 if ((qop & PFS_LKUP_OP) != PFS_LKUP_LOOKUP) cn->cn_flags |= SAVENAME;
 return(vn?0:EJUSTRETURN);
}

static int pfs_create(void *v)
{
 ARGS(create); /* struct vnode *a_dvp;
		  struct vnode **a_vpp;
		  struct componentname *a_cnp;
		  struct vattr *a_vap; */
 struct componentname *cn;
 int err;
 struct pfs_create_rep p;
 char *qbuf;
 struct pfs_create_req *q;
 struct vnode *vn;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_create: no name buffer");
 do <"ret">
  { printf("pfs_create: dvp %s, component %.*s\n",VTOPFS(ap->a_dvp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    *ap->a_vpp = NULLVP;
    qbuf = malloc(sizeof(struct pfs_create_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->id = VTOPFS(ap->a_dvp)->id;
    q->attr = *ap->a_vap;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_create_req),cn->cn_namelen);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_CREATE,qbuf,sizeof(struct pfs_create_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_create: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_create: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    if ((p.id < PFS_MIN_ID) || (p.id > PFS_MAX_ID))
     { printf("pfs_create: protocol error, id %d\n",p.id);
       err = EIO;
       break;
     }
    printf("pfs_create returned id %d, calling pfs_get_vnode\n",p.id);
    err = pfs_get_vnode(ap->a_dvp->v_mount,&vn,p.id);
    if (err)
     { printf("pfs_create: pfs_get_vnode failed\n");
       break;
     }
    printf("pfs_create got %p\n",(void *)vn);
    switch (vn->v_type)
     { case VNON:
	  pfs_set_type(vn,VREG);
	  break;
       case VREG:
	  break;
       default:
	  printf("pfs_create: protocol error: returned existing id %d of type %d\n",p.id,vn->v_type);
	  VOP_UNLOCK(vn,0);
	  err = EIO;
	  break <"ret">;
     }
    *ap->a_vpp = vn;
    printf("pfs_create: success\n");
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_mknod(void *v)
{
 ARGS(mknod); /* struct vnode *a_dvp;
		 struct vnode **a_vpp;
		 struct componentname *a_cnp;
		 struct vattr *a_vap; */
 struct componentname *cn;
 int err;
 struct pfs_mknod_rep p;
 char *qbuf;
 struct pfs_mknod_req *q;
 struct vnode *vn;
 enum vtype newtype;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_mknod: no name buffer");
 newtype = ap->a_vap->va_type;
 switch (newtype)
  { case VCHR:
    case VBLK:
    case VFIFO:
       break;
    default:
       panic("pfs_mknod: bad type %d",newtype);
       break;
  }
 do
  { printf("pfs_mknod: dvp %s, component %.*s\n",VTOPFS(ap->a_dvp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    *ap->a_vpp = NULLVP;
    qbuf = malloc(sizeof(struct pfs_mknod_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->id = VTOPFS(ap->a_dvp)->id;
    q->attr = *ap->a_vap;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_mknod_req),cn->cn_namelen);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_MKNOD,qbuf,sizeof(struct pfs_mknod_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_mknod: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_mknod: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    if ((p.id < PFS_MIN_ID) || (p.id > PFS_MAX_ID))
     { printf("pfs_mknod: protocol error, id %d\n",p.id);
       err = EIO;
       break;
     }
    printf("pfs_mknod returned id %d, calling pfs_get_vnode\n",p.id);
    err = pfs_get_vnode(ap->a_dvp->v_mount,&vn,p.id);
    if (err)
     { printf("pfs_mknod: pfs_get_vnode failed\n");
       break;
     }
    printf("pfs_mknod got %p\n",(void *)vn);
    if (vn->v_type == VNON)
     { pfs_set_type(vn,VREG);
     }
    else if (vn->v_type != newtype)
     { printf("pfs_mknod: protocol error: existing id %d of type %d returned, wanted type %d\n",p.id,vn->v_type,newtype);
       VOP_UNLOCK(vn,0);
       err = EIO;
       break;
     }
    *ap->a_vpp = vn;
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_open(void *v)
{
 ARGS(open); /* struct vnode *a_vp;
		int a_mode;
		struct ucred *a_cred;
		struct proc *a_p; */
 struct pfs_open_req q;
 struct pfs_open_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_open: vp %s, mode %#o, by %d\n",VTOPFS(ap->a_vp)->pstr,ap->a_mode,ap->a_p?ap->a_p->p_pid:0);
 q.id = VTOPFS(ap->a_vp)->id;
 q.mode = ap->a_mode;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_OPEN,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_open: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_open: userland returned error %d\n",p.err);
    return(p.err);
  }
 printf("pfs_open: success\n");
 return(0);
}

static int pfs_close(void *v)
{
 ARGS(close); /* struct vnode *a_vp;
		 int a_fflag;
		 struct ucred *a_cred;
		 struct proc *a_p; */
 struct pfs_close_req q;
 struct pfs_close_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_close: vp %s, flags %#o\n",VTOPFS(ap->a_vp)->pstr,ap->a_fflag);
 q.id = VTOPFS(ap->a_vp)->id;
 q.flags = ap->a_fflag;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_CLOSE,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_close: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_close: userland returned error %d\n",p.err);
    return(p.err);
  }
 printf("pfs_close: success\n");
 return(0);
}

static int pfs_access(void *v)
{
 ARGS(access); /* struct vnode *a_vp;
		  int a_mode;
		  struct ucred *a_cred;
		  struct proc *a_p; */
 struct pfs_access_req q;
 struct pfs_access_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_access: vp %s mode %d ucred %p\n",VTOPFS(ap->a_vp)->pstr,ap->a_mode,(void *)ap->a_cred);
 q.id = VTOPFS(ap->a_vp)->id;
 q.mode = ap->a_mode;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_ACCESS,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_access: pfs_call failed (%d)\n",err);
    return(err);
  }
 /* upon removing the debugging printfs, can become just return(p.err) */
 if (p.err)
  { printf("pfs_access: userland returned error %d\n",p.err);
    return(p.err);
  }
 printf("pfs_access: permitted\n");
 return(0);
}

static int pfs_getattr(void *v)
{
 ARGS(getattr); /* struct vnode *a_vp;
		   struct vattr *a_vap;
		   struct ucred *a_cred;
		   struct proc *a_p; */
 struct pfs_getattr_req q;
 struct pfs_getattr_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_getattr: vp %s, vap %p\n",VTOPFS(ap->a_vp)->pstr,(void *)ap->a_vap);
 q.id = VTOPFS(ap->a_vp)->id;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_GETATTR,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_getattr: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_getattr: userland returned error %d\n",p.err);
    return(p.err);
  }
 p.attr.va_fsid = ap->a_vp->v_mount->mnt_stat.f_fsid.val[0];
 *ap->a_vap = p.attr;
 printf("pfs_getattr: success\n");
 return(0);
}

static int pfs_setattr(void *v)
{
 ARGS(setattr); /* struct vnode *a_vp;
		   struct vattr *a_vap;
		   struct ucred *a_cred;
		   struct proc *a_p; */
 struct pfs_setattr_req q;
 struct pfs_setattr_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 ROFS_CHECK(ap->a_vp->v_mount);
 printf("pfs_setattr: vp %s, vap %p\n",VTOPFS(ap->a_vp)->pstr,(void *)ap->a_vap);
 q.id = VTOPFS(ap->a_vp)->id;
 q.attr = *ap->a_vap;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_SETATTR,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_setattr: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_setattr: userland returned error %d\n",p.err);
    return(p.err);
  }
 printf("pfs_setattr: success\n");
 return(0);
}

static int pfs_read(void *v)
{
 ARGS(read); /* struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		struct ucred *a_cred; */
 struct pfs_read_req q;
 struct pfs_read_rep p;
 int err;
 struct uio *uio;
 int len;
 struct socket *s;
 char *replybuf;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 uio = ap->a_uio;
 printf("pfs_read: vp %s, ioflag %#x, %lu@%qu\n",VTOPFS(ap->a_vp)->pstr,ap->a_ioflag,(unsigned long int)uio->uio_resid,(unsigned long long int)uio->uio_offset);
 q.id = VTOPFS(ap->a_vp)->id;
 q.flags = ap->a_ioflag;
 SETCRED(&q.cred,ap->a_cred);
 q.len = uio->uio_resid;
 q.off = uio->uio_offset;
 replybuf = malloc(PFS_REP_MAX,M_TEMP,M_WAITOK);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_READ,&q,sizeof(q),&replybuf[0],PFS_REP_MAX,&len,&s);
 if (err)
  { printf("pfs_read: pfs_call failed (%d)\n",err);
    free(replybuf,M_TEMP);
    return(err);
  }
 if (len < sizeof(struct pfs_read_rep))
  { printf("pfs_read: reply too short (%d < %d)\n",len,(int)sizeof(struct pfs_read_rep));
    soclose(s);
    free(replybuf,M_TEMP);
    return(EIO);
  }
 bcopy(&replybuf[0],&p,sizeof(struct pfs_read_rep));
 if (p.err)
  { printf("pfs_read: userland returned error %d\n",p.err);
    soclose(s);
    free(replybuf,M_TEMP);
    return(p.err);
  }
 if (p.len < 1)
  { printf("pfs_read: no data\n");
    soclose(s);
    free(replybuf,M_TEMP);
    return(0);
  }
 if (p.len > uio->uio_resid)
  { printf("pfs_read: protocol error: read more (%ld) than requested (%ld)\n",(long int)p.len,(long int)uio->uio_resid);
    soclose(s);
    free(replybuf,M_TEMP);
    return(EIO);
  }
 if (p.flags & PFS_READ_P_SOCKET)
  { size_t oldresid;
    if (len != sizeof(struct pfs_read_rep))
     { printf("pfs_read: protocol error: SOCKET reply has data (%d != %d)\n",len,(int)sizeof(struct pfs_read_rep));
       soclose(s);
       free(replybuf,M_TEMP);
       return(EIO);
     }
    oldresid = uio->uio_resid;
    uio->uio_resid = p.len;
    while (uio->uio_resid > 0)
     { err = soreceive(s,0,uio,0,0,0);
       if (err) break;
     }
    soclose(s);
    uio->uio_resid += oldresid - p.len;
  }
 else
  { soclose(s);
    if (len != p.len+sizeof(struct pfs_read_rep))
     { printf("pfs_read: protocol error: !SOCKET reply size wrong (%d != %d)\n",len,(int)(p.len+sizeof(struct pfs_read_rep)));
       free(replybuf,M_TEMP);
       return(EIO);
     }
    err = uiomove(&replybuf[sizeof(struct pfs_read_rep)],p.len,uio);
  }
 free(replybuf,M_TEMP);
 return(err);
}

/*
 * This function has a potential problem, in that it can uiomove() more
 *  data from the uio than it actually writes.  We do reset uio_resid
 *  and uio_offset afterwards, but we depend on the rest of the kernel
 *  to not break if we advance the rest of the uio's variables more
 *  than implied by our (net) changes to uio_resid and uio_offset.
 */
static int pfs_write(void *v)
{
 ARGS(write); /* struct vnode *a_vp;
		 struct uio *a_uio;
		 int a_ioflag;
		 struct ucred *a_cred; */
 int err;
 struct uio *uio;
 struct pfs_write_rep p;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 ROFS_CHECK(ap->a_vp->v_mount);
 uio = ap->a_uio;
 printf("pfs_write: vp %s, ioflag %#x, %lu@%qu\n",VTOPFS(ap->a_vp)->pstr,ap->a_ioflag,(unsigned long int)uio->uio_resid,(unsigned long long int)uio->uio_offset);
 uvm_vnp_uncache(ap->a_vp); /* really should be at a higher level! */
 /* Eventually, this should be the former.  It's the latter
    for now to make it easier to test the socket code. */
#if 0
 if (uio->uio_resid < PFS_REQ_MAX-sizeof(struct pfs_req)-sizeof(struct pfs_write_req))
#else
 if (uio->uio_resid < 64)
#endif
  { struct pfs_write1_req *q;
    char *qbuf;
    int oldresid;
    oldresid = uio->uio_resid;
    qbuf = malloc(sizeof(struct pfs_write1_req)+oldresid,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->id = VTOPFS(ap->a_vp)->id;
    q->flags = ap->a_ioflag;
    SETCRED(&q->cred,ap->a_cred);
    q->off = uio->uio_offset;
    err = uiomove(qbuf+sizeof(struct pfs_write1_req),oldresid,uio);
    if (err)
     { printf("pfs_write: uiomove failed (%d)\n",err);
       free(qbuf,M_TEMP);
       return(err);
     }
    err = pfs_call(ap->a_vp->v_mount,PFSOP_WRITE,qbuf,sizeof(struct pfs_write1_req)+oldresid,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_write: pfs_call failed (%d)\n",err);
       return(err);
     }
    if (p.err)
     { printf("pfs_write: userland returned error %d\n",p.err);
       return(p.err);
     }
    if (p.len > oldresid)
     { printf("pfs_write: protocol error, wrote %d but requested %d\n",p.len,oldresid);
       return(EIO);
     }
    uio->uio_resid = oldresid - p.len;
    uio->uio_offset -= uio->uio_resid;
  }
 else
  { struct pfs_write2_req q;
    struct socket *s;
    struct uio repuio;
    struct iovec repiov;
    size_t oldresid;
    oldresid = uio->uio_resid;
    q.id = VTOPFS(ap->a_vp)->id;
    q.flags = ap->a_ioflag;
    SETCRED(&q.cred,ap->a_cred);
    q.len = uio->uio_resid;
    q.off = uio->uio_offset;
    err = pfs_call(ap->a_vp->v_mount,PFSOP_WRITE,&q,sizeof(q),0,PFS_NO_REPLY,0,&s);
    if (err)
     { printf("pfs_write: pfs_call failed (%d)\n",err);
       return(err);
     }
    while <"sendloop"> (uio->uio_resid)
     { err = sosend(s,0,uio,0,0,0);
       switch (err)
	{ case 0:
	     break;
	  case EPIPE:
	     printf("pfs_write: sosend got EPIPE\n");
	     break <"sendloop">;
	  default:
	     soclose(s);
	     printf("pfs_write: sosend failed (%d)\n",err);
	     return(err);
	     break;
	}
     }
    repiov.iov_base = &p;
    repiov.iov_len = sizeof(p);
    repuio.uio_iov = &repiov;
    repuio.uio_iovcnt = 1;
    repuio.uio_offset = 0;
    repuio.uio_resid = sizeof(p);
    repuio.uio_segflg = UIO_SYSSPACE;
    repuio.uio_rw = UIO_READ;
    repuio.uio_procp = 0;
    while (repuio.uio_resid > 0)
     { int prevresid;
       prevresid = repuio.uio_resid;
       err = soreceive(s,0,&repuio,0,0,0);
       if (err)
	{ soclose(s);
	  printf("pfs_write: soreceive failed (%d), returning EIO\n",err);
	  return(EIO);
	}
       if (prevresid == repuio.uio_resid)
	{ soclose(s);
	  printf("pfs_write: soreceive failed EOF, returning EIO\n");
	  return(EIO);
	}
     }
    if (p.err)
     { printf("pfs_write: userland returned error %d\n",p.err);
       soclose(s);
       return(p.err);
     }
    if (p.len > oldresid)
     { printf("pfs_write: protocol error, wrote %d but requested %d\n",p.len,oldresid);
       soclose(s);
       return(EIO);
     }
    uio->uio_offset -= oldresid - uio->uio_resid - p.len;
    uio->uio_resid = oldresid - p.len;
    soclose(s);
  }
 printf("pfs_write: success\n");
 return(0);
}

static int pfs_ioctl(void *v)
{
 ARGS(ioctl); /* struct vnode *a_vp;
		 u_long a_command;
		 caddr_t a_data;
		 int a_fflag;
		 struct ucred *a_cred;
		 struct proc *a_p; */

 /* vp unlocked on entry and on all exits */
 panic("pfs_ioctl");
}

static int pfs_fcntl(void *v)
{
 ARGS(fcntl); /* struct vnode *a_vp;
		 u_int a_command;
		 caddr_t a_data;
		 int a_fflag;
		 struct ucred *a_cred;
		 struct proc *a_p; */

 /* vp locked on entry and on all exits */
 panic("pfs_fcntl");
}

static int pfs_poll(void *v)
{
 ARGS(poll); /* struct vnode *a_vp;
		int a_events;
		struct proc *a_p; */

 /* vp unlocked on entry and on all exits */
 panic("pfs_poll");
}

static int pfs_revoke(void *v)
{
 ARGS(revoke); /* struct vnode *a_vp;
		  int a_flags; */

 /* vp unlocked on entry and on all exits */
 panic("pfs_revoke");
}

static int pfs_mmap(void *v)
{
 ARGS(mmap); /* struct vnode *a_vp;
		int a_fflags;
		struct ucred *a_cred;
		struct proc *a_p; */

 panic("pfs_mmap");
}

static int pfs_fsync(void *v)
{
 ARGS(fsync); /* struct vnode *a_vp;
		 struct ucred *a_cred;
		 int a_flags;
		 struct proc *a_p; */

 /* vp locked on entry and on all exits */
 panic("pfs_fsync");
}

static int pfs_seek(void *v)
{
 ARGS(seek); /* struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		struct ucred *a_cred; */
 struct pfs_seek_req q;
 struct pfs_seek_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_seek: vp %s, from %qd to %qd\n",VTOPFS(ap->a_vp)->pstr,(quad_t)ap->a_oldoff,(quad_t)ap->a_newoff);
 q.id = VTOPFS(ap->a_vp)->id;
 q.oldoff = ap->a_oldoff;
 q.newoff = ap->a_newoff;
 SETCRED(&q.cred,ap->a_cred);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_SEEK,&q,sizeof(q),&p,sizeof(p),0,0);
 if (err)
  { printf("pfs_seek: pfs_call failed (%d)\n",err);
    return(err);
  }
 if (p.err)
  { printf("pfs_seek: userland returned error %d\n",p.err);
    return(p.err);
  }
 printf("pfs_seek: success\n");
 return(0);
}

static int pfs_remove(void *v)
{
 ARGS(remove); /* struct vnode *a_dvp;
		  struct vnode *a_vp;
		  struct componentname *a_cnp; */
 struct componentname *cn;
 int err;
 struct pfs_remove_rep p;
 char *qbuf;
 struct pfs_remove_req *q;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_remove: no name buffer");
 do
  { printf("pfs_remove: dvp %s, object %s, %.*s\n",VTOPFS(ap->a_dvp)->pstr,VTOPFS(ap->a_vp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    qbuf = malloc(sizeof(struct pfs_remove_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->dir = VTOPFS(ap->a_dvp)->id;
    q->obj = VTOPFS(ap->a_vp)->id;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_remove_req),cn->cn_namelen);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_REMOVE,qbuf,sizeof(struct pfs_remove_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_remove: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_remove: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    err = 0;
  } while (0);
 ((ap->a_vp==ap->a_dvp)?vrele:vput)(ap->a_vp);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_link(void *v)
{
 ARGS(link); /* struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp; */
 struct componentname *cn;
 char *qbuf;
 struct pfs_link_req *q;
 struct pfs_link_rep p;
 int err;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 if (ap->a_vp->v_mount != ap->a_dvp->v_mount) return(EXDEV);
 do
  { cn = ap->a_cnp;
    if (! (cn->cn_flags & HASBUF)) panic("pfs_link: no name buffer");
    printf("pfs_link: vp %s, into %s as %.*s\n",VTOPFS(ap->a_vp)->pstr,VTOPFS(ap->a_dvp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    if (ap->a_vp->v_type == VDIR)
     { err = EISDIR;
       break;
     }
    qbuf = malloc(sizeof(struct pfs_link_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->old = VTOPFS(ap->a_vp)->id;
    q->dir = VTOPFS(ap->a_dvp)->id;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_link_req),cn->cn_namelen);
    err = pfs_call(ap->a_vp->v_mount,PFSOP_LINK,qbuf,sizeof(struct pfs_link_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    if (err)
     { printf("pfs_link: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_link: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    printf("pfs_link: success\n");
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_rename(void *v)
{
 ARGS(rename); /* struct vnode *a_fdvp;
		  struct vnode *a_fvp;
		  struct componentname *a_fcnp;
		  struct vnode *a_tdvp;
		  struct vnode *a_tvp;
		  struct componentname *a_tcnp; */
 char *qbuf;
 struct pfs_rename_req *q;
 struct pfs_rename_rep p;
 int err;
 struct componentname *fcn;
 struct componentname *tcn;

 ap = v;
 fcn = ap->a_fcnp;
 tcn = ap->a_tcnp;
 if (! (fcn->cn_flags & HASBUF)) panic("pfs_rename: no from name buffer");
 if (! (tcn->cn_flags & HASBUF)) panic("pfs_rename: no to name buffer");
 do
  { if ( (ap->a_fvp->v_mount != ap->a_tdvp->v_mount) ||
	 (ap->a_tvp && (ap->a_tvp->v_mount != ap->a_tdvp->v_mount)) )
     { err = EXDEV;
       break;
     }
    ROOT_USECOUNT_CHECK(ap->a_fdvp->v_mount);
    ROFS_CHECK(ap->a_fdvp->v_mount);
    printf("pfs_rename: %s (%.*s in %s) to %s (%.*s in %s)\n",VTOPFS(ap->a_fvp)->pstr,(int)fcn->cn_namelen,fcn->cn_nameptr,VTOPFS(ap->a_fdvp)->pstr,ap->a_tvp?VTOPFS(ap->a_tvp)->pstr:"(null)",(int)tcn->cn_namelen,tcn->cn_nameptr,VTOPFS(ap->a_tdvp)->pstr);
    qbuf = malloc(sizeof(struct pfs_rename_req)+fcn->cn_namelen+tcn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->from_dir = VTOPFS(ap->a_fdvp)->id;
    q->from_id = VTOPFS(ap->a_fvp)->id;
    q->from_len = fcn->cn_namelen;
    q->to_dir = VTOPFS(ap->a_tdvp)->id;
    q->to_id = ap->a_tvp ? VTOPFS(ap->a_tvp)->id : PFS_NO_ID;
    /* XXX can we count on fcn->cn_cred matching tcn->cn_cred? */
    SETCRED(&q->cred,fcn->cn_cred);
    bcopy(fcn->cn_nameptr,qbuf+sizeof(struct pfs_rename_req),fcn->cn_namelen);
    bcopy(tcn->cn_nameptr,qbuf+sizeof(struct pfs_rename_req)+fcn->cn_namelen,tcn->cn_namelen);
    err = pfs_call(ap->a_fdvp->v_mount,PFSOP_RENAME,qbuf,sizeof(struct pfs_rename_req)+fcn->cn_namelen+tcn->cn_namelen,&p,sizeof(p),0,0);
    if (err)
     { printf("pfs_rename: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_rename: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    printf("pfs_rename: success\n");
    err = 0;
  } while (0);
 vrele(ap->a_fvp);
 vrele(ap->a_fdvp);
 ((ap->a_tdvp==ap->a_tvp)?vrele:vput)(ap->a_tdvp);
 if (ap->a_tvp) vput(ap->a_tvp);
 if (! (fcn->cn_flags & SAVESTART)) free(fcn->cn_pnbuf,M_NAMEI);
 if (! (tcn->cn_flags & SAVESTART)) free(tcn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_mkdir(void *v)
{
 ARGS(mkdir); /* struct vnode *a_dvp;
		 struct vnode **a_vpp;
		 struct componentname *a_cnp;
		 struct vattr *a_vap; */
 struct componentname *cn;
 int err;
 struct pfs_mkdir_rep p;
 char *qbuf;
 struct pfs_mkdir_req *q;
 struct vnode *vn;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_mkdir: no name buffer");
 do <"ret">
  { printf("pfs_mkdir: dvp %s, component %.*s\n",VTOPFS(ap->a_dvp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    *ap->a_vpp = NULLVP;
    qbuf = malloc(sizeof(struct pfs_mkdir_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->id = VTOPFS(ap->a_dvp)->id;
    q->attr = *ap->a_vap;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_mkdir_req),cn->cn_namelen);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_MKDIR,qbuf,sizeof(struct pfs_mkdir_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_mkdir: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_mkdir: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    if ((p.id < PFS_MIN_ID) || (p.id > PFS_MAX_ID))
     { printf("pfs_mkdir: protocol error, id %d\n",p.id);
       err = EIO;
       break;
     }
    printf("pfs_create returned id %d, calling pfs_get_vnode\n",p.id);
    err = pfs_get_vnode(ap->a_dvp->v_mount,&vn,p.id);
    if (err)
     { printf("pfs_create: pfs_get_vnode failed\n");
       break;
     }
    printf("pfs_create got %p\n",(void *)vn);
    switch (vn->v_type)
     { case VNON:
	  pfs_set_type(vn,VDIR);
	  printf("pfs_mkdir adding VDIR parent pointer, %s->%s\n",VTOPFS(vn)->pstr,VTOPFS(ap->a_dvp)->pstr);
	  VREF(ap->a_dvp);
	  VTOPFS(vn)->parent = VTOPFS(ap->a_dvp);
	  break;
       case VDIR:
	  break;
       default:
	  printf("pfs_mkdir: protocol error: returned existing id %d of type %d\n",p.id,vn->v_type);
	  VOP_UNLOCK(vn,0);
	  err = EIO;
	  break <"ret">;
     }
    *ap->a_vpp = vn;
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_rmdir(void *v)
{
 ARGS(rmdir); /* struct vnode *a_dvp;
		 struct vnode *a_vp;
		 struct componentname *a_cnp; */
 struct componentname *cn;
 int err;
 struct pfs_rmdir_rep p;
 char *qbuf;
 struct pfs_rmdir_req *q;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_rmdir: no name buffer");
 do
  { printf("pfs_rmdir: dvp %s, dir %s, %.*s\n",VTOPFS(ap->a_dvp)->pstr,VTOPFS(ap->a_vp)->pstr,(int)cn->cn_namelen,cn->cn_nameptr);
    qbuf = malloc(sizeof(struct pfs_rmdir_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->dir = VTOPFS(ap->a_dvp)->id;
    q->obj = VTOPFS(ap->a_vp)->id;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_rmdir_req),cn->cn_namelen);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_RMDIR,qbuf,sizeof(struct pfs_rmdir_req)+cn->cn_namelen,&p,sizeof(p),0,0);
    free(qbuf,M_TEMP);
    if (err)
     { printf("pfs_rmdir: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_rmdir: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 vput(ap->a_vp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_symlink(void *v)
{
 ARGS(symlink); /* struct vnode *a_dvp;
		   struct vnode **a_vpp;
		   struct componentname *a_cnp;
		   struct vattr *a_vap;
		   char *a_target; */
 char *qbuf;
 struct pfs_symlink_req *q;
 struct pfs_symlink_rep p;
 int err;
 int l;
 struct componentname *cn;
 struct vnode *vn;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 ROFS_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 if (! (cn->cn_flags & HASBUF)) panic("pfs_symlink: no name buffer");
 do <"ret">
  { printf("pfs_symlink: %.*s in %s -> %s\n",(int)cn->cn_namelen,cn->cn_nameptr,VTOPFS(ap->a_dvp)->pstr,ap->a_target);
    *ap->a_vpp = NULLVP;
    l = strlen(ap->a_target);
    if (sizeof(struct pfs_req)+sizeof(struct pfs_symlink_req)+cn->cn_namelen+l > PFS_REQ_MAX)
     { printf("pfs_symlink: too long\n");
       return(EINVAL);
     }
    qbuf = malloc(sizeof(struct pfs_symlink_req)+cn->cn_namelen+l,M_TEMP,M_WAITOK);
    q = (void *) qbuf;
    q->dir = VTOPFS(ap->a_dvp)->id;
    q->attr = *ap->a_vap;
    q->name_len = cn->cn_namelen;
    SETCRED(&q->cred,cn->cn_cred);
    bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_symlink_req),cn->cn_namelen);
    bcopy(ap->a_target,qbuf+sizeof(struct pfs_symlink_req)+cn->cn_namelen,l);
    err = pfs_call(ap->a_dvp->v_mount,PFSOP_SYMLINK,qbuf,sizeof(struct pfs_symlink_req)+cn->cn_namelen+l,&p,sizeof(p),0,0);
    if (err)
     { printf("pfs_symlink: pfs_call failed (%d)\n",err);
       break;
     }
    if (p.err)
     { printf("pfs_symlink: userland returned error %d\n",p.err);
       err = p.err;
       break;
     }
    if ((p.id < PFS_MIN_ID) || (p.id > PFS_MAX_ID))
     { printf("pfs_symlink: protocol error, id %d\n",p.id);
       err = EIO;
       break;
     }
    printf("pfs_symlink returned id %d, calling pfs_get_vnode\n",p.id);
    err = pfs_get_vnode(ap->a_dvp->v_mount,&vn,p.id);
    if (err)
     { printf("pfs_symlink: pfs_get_vnode failed\n");
       break;
     }
    printf("pfs_symlink got %p\n",(void *)vn);
    switch (vn->v_type)
     { case VNON:
	  pfs_set_type(vn,VLNK);
	  break;
       case VLNK:
	  break;
       default:
	  printf("pfs_symlink: protocol error: returned existing id %d of type %d\n",p.id,vn->v_type);
	  VOP_UNLOCK(vn,0);
	  err = EIO;
	  break <"ret">;
     }
    *ap->a_vpp = vn;
    vrele(vn); /* XXX - see XXX at vop_symlink in vnode_if.src */
    printf("pfs_symlink: success\n");
    err = 0;
  } while (0);
 vput(ap->a_dvp);
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

static int pfs_readdir(void *v)
{
 ARGS(readdir); /* struct vnode *a_vp;
		   struct uio *a_uio;
		   struct ucred *a_cred;
		   int *a_eofflag;
		   off_t **a_cookies;
		   int *a_ncookies; */
 struct pfs_readdir_req q;
 struct pfs_readdir_rep p;
 int err;
 struct uio *uio;
 int len;
 off_t *cookies;
 int eleft;
 int o;
 int left;
 int cookiesize;
 int minentry;
 int entsize;
 off_t lastcookie;
 struct dirent ent;
 char *replybuf;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_readdir: vp %s\n",VTOPFS(ap->a_vp)->pstr);
 uio = ap->a_uio;
 q.id = VTOPFS(ap->a_vp)->id;
 q.off = uio->uio_offset;
 SETCRED(&q.cred,ap->a_cred);
 q.maxsize = uio->uio_resid;
 q.flags = ap->a_ncookies ? PFS_READDIR_Q_COOKIES : 0;
 /* extra sizeof() space is so the bcopy in the loop can't overrun replybuf
    even if the reply is garbage. */
 replybuf = malloc(PFS_REP_MAX+sizeof(struct dirent),M_TEMP,M_WAITOK);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_READDIR,&q,sizeof(q),replybuf,PFS_REP_MAX,&len,0);
 if (err)
  { printf("pfs_readdir: pfs_call failed (%d)\n",err);
    free(replybuf,M_TEMP);
    return(err);
  }
 if (len < sizeof(struct pfs_readdir_rep))
  { printf("pfs_readdir: reply too short (%d < %d)\n",len,(int)sizeof(struct pfs_readdir_rep));
    free(replybuf,M_TEMP);
    return(EIO);
  }
 bcopy(&replybuf[0],&p,sizeof(struct pfs_readdir_rep));
 if (p.err)
  { printf("pfs_readdir: userland returned error %d\n",p.err);
    free(replybuf,M_TEMP);
    return(p.err);
  }
 if (p.count < 0)
  { printf("pfs_readdir: protocol error, count %d is negative\n",p.count);
    free(replybuf,M_TEMP);
    return(EIO);
  }
 cookiesize = ap->a_ncookies ? sizeof(off_t) : 0;
 minentry = sizeof(struct dirent) - (MAXNAMLEN+1) + cookiesize;
 /*
  * The first check is in case p.count is so large the multiply
  *  overflows.  We know len is bounded by PFS_REP_MAX, so if the first
  *  check passes, the multiply can't overflow.  The reason we need to
  *  check p.count at all is so that we don't try to malloc insane
  *  amounts of data if our caller asked for cookies and userland gave
  *  us a garbage p.count.  We could compare p.count > len/minentry
  *  instead, but the extra compare is a price worth paying to be able
  *  to do a multiply instead of a divide.
  */
 if ( (p.count > len) ||
      (p.count*minentry > len) )
  { printf("pfs_readdir: protocol error, count %d is impossible for len %d\n",p.count,len);
    free(replybuf,M_TEMP);
    return(EIO);
  }
 *ap->a_eofflag = (p.flags & PFS_READDIR_P_EOF) ? 1 : 0;
 if (ap->a_ncookies)
  { cookies = malloc(p.count*sizeof(off_t),M_TEMP,M_WAITOK);
    *ap->a_cookies = cookies;
    *ap->a_ncookies = p.count;
  }
 else
  { cookies = 0;
  }
 printf("pfs_readdir: p.count %d uio_resid %llu\n",p.count,(unsigned long long int)uio->uio_resid);
 o = sizeof(struct pfs_readdir_rep);
 eleft = p.count;
 left = uio->uio_resid;
 while (1)
  { /*
     * Error-check thoroughly.  Sigh, but we really don't want the
     *  kernel to crash if userland wigs out.  The userland process
     *	must be highly trusted, of course, but we don't want to trust
     *	it _that_ much.
     */
    if (o >= len)
     { if (eleft == 0) break;
       printf("pfs_readdir: protocol error, end of data but not out of entries (%d left)\n",eleft);
       err = EIO;
       break;
     }
    if (eleft < 1)
     { printf("pfs_readdir: protocol error, out of entries but data remains (%d left)\n",len-o);
       err = EIO;
       break;
     }
    if (len-o < minentry)
     { printf("pfs_readdir: protocol error, leftover data (%d left)\n",len-o);
       err = EIO;
       break;
     }
    bcopy(&replybuf[o],&ent,sizeof(struct dirent)-(MAXNAMLEN+1));
    if (ent.d_reclen > sizeof(struct dirent))
     { printf("pfs_readdir: protocol error, impossible d_reclen %d\n",ent.d_reclen);
       err = EIO;
       break;
     }
    /*
     * At present, d_namlen is u_int8_t and MAXNAMLEN is 255, which
     *  means this truly can't happen.  Leave it in as a firewall
     *	against future changes; let the optimizer deal in the meantime.
     */
    if (ent.d_namlen > MAXNAMLEN)
     { printf("pfs_readdir: protocol error, impossible d_namlen %d\n",ent.d_namlen);
       err = EIO;
       break;
     }
    if (sizeof(struct dirent)-(MAXNAMLEN+1)+ent.d_namlen > ent.d_reclen)
     { printf("pfs_readdir: protocol error, name overruns record (%ld > %ld)\n",(long int)(sizeof(struct dirent)-(MAXNAMLEN+1)+ent.d_namlen),(long int)ent.d_reclen);
       err = EIO;
       break;
     }
    entsize = DIRENT_SIZE(&ent);
    if (entsize > left)
     { printf("pfs_readdir: protocol error, entry overruns maxsize (%d > %d)\n",entsize,left);
       err = EIO;
       break;
     }
    if (o+ent.d_reclen+cookiesize > len)
     { printf("pfs_readdir: protocol error, entry overruns reply (%d > %d)\n",o+ent.d_reclen+cookiesize,len);
       err = EIO;
       break;
     }
    if ( (ent.d_fileno < PFS_MIN_ID) &&
	 (ent.d_fileno > PFS_MAX_ID) &&
	 (ent.d_fileno != PFS_ROOT_ID) )
     { printf("pfs_readdir: protocol error, bad d_fileno %lu\n",(unsigned long int)ent.d_fileno);
       err = EIO;
       break;
     }
    /*
     * Okay, it looks valid; go for it.  Almost anticlimactic, after
     *  all those error checks.
     *
     * Copies the beginning again, and copies <=3 bytes extra, but
     *  those don't hurt anything.
     */
    bcopy(&replybuf[o],&ent,entsize);
    printf("pfs_readdir: entry: d_fileno %lu type %d name %.*s\n",(unsigned long int)ent.d_fileno,ent.d_type,ent.d_namlen,&ent.d_name[0]);
    o += ent.d_reclen;
    ent.d_reclen = entsize;
    bzero(&ent.d_name[ent.d_namlen],(((char *)&ent)+ent.d_reclen)-&ent.d_name[ent.d_namlen]);
    err = uiomove(&ent,entsize,uio);
    if (err)
     { printf("pfs_readdir: uiomove for entry failed (%d)\n",err);
       break;
     }
    left -= entsize;
    if (cookies)
     { bcopy(&replybuf[o],&lastcookie,sizeof(off_t));
       *cookies++ = lastcookie;
       o += sizeof(off_t);
     }
    eleft --;
  }
 if (cookies && p.count && (lastcookie != p.lastoff))
  { printf("pfs_readdir: protocol error, last cookie doesn't match (%qu != %qu)\n",(unsigned long long int)lastcookie,(unsigned long long int)p.lastoff);
    err = EIO;
  }
 if (cookies && err)
  { free(*ap->a_cookies,M_TEMP);
    *ap->a_cookies = 0;
    *ap->a_ncookies = 0;
  }
 if (p.count) uio->uio_offset = p.lastoff;
 free(replybuf,M_TEMP);
 return (err);
}

static int pfs_readlink(void *v)
{
 ARGS(readlink); /* struct vnode *a_vp;
		    struct uio *a_uio;
		    struct ucred *a_cred; */
 struct pfs_readlink_req q;
 struct pfs_readlink_rep p;
 int err;
 int len;
 char *replybuf;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_readlink: vp %s\n",VTOPFS(ap->a_vp)->pstr);
 q.id = VTOPFS(ap->a_vp)->id;
 SETCRED(&q.cred,ap->a_cred);
 replybuf = malloc(PFS_REP_MAX,M_TEMP,M_WAITOK);
 err = pfs_call(ap->a_vp->v_mount,PFSOP_READLINK,&q,sizeof(q),&replybuf[0],PFS_REP_MAX,&len,0);
 if (err)
  { printf("pfs_readlink: pfs_call failed (%d)\n",err);
    free(replybuf,M_TEMP);
    return(err);
  }
 if (len < sizeof(struct pfs_readlink_rep))
  { printf("pfs_readlink: reply too short (%d < %d)\n",len,(int)sizeof(struct pfs_readlink_rep));
    free(replybuf,M_TEMP);
    return(EIO);
  }
 bcopy(&replybuf[0],&p,sizeof(struct pfs_readlink_rep));
 if (p.err)
  { printf("pfs_readlink: userland returned error %d\n",p.err);
    free(replybuf,M_TEMP);
    return(p.err);
  }
 err = uiomove(&replybuf[sizeof(struct pfs_readlink_rep)],len-sizeof(struct pfs_readlink_rep),ap->a_uio);
 free(replybuf,M_TEMP);
 return(err);
}

static int pfs_abortop(void *v)
{
 ARGS(abortop); /* struct vnode *a_dvp;
		   struct componentname *a_cnp; */
 char *qbuf;
 struct pfs_abort_req *q;
 struct componentname *cn;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 printf("pfs_abortop: %.*s in %s\n",(int)cn->cn_namelen,cn->cn_nameptr,VTOPFS(ap->a_dvp)->pstr);
 qbuf = malloc(sizeof(struct pfs_abort_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
 q = (void *) qbuf;
 q->dir = VTOPFS(ap->a_dvp)->id;
 bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_abort_req),cn->cn_namelen);
 pfs_call(ap->a_dvp->v_mount,PFSOP_ABORT,qbuf,sizeof(struct pfs_abort_req)+cn->cn_namelen,0,PFS_NO_REPLY,0,0);
 if ((cn->cn_flags & (HASBUF|SAVESTART)) == HASBUF) free(cn->cn_pnbuf,M_NAMEI);
 return(0);
}

static int pfs_inactive(void *v)
{
 ARGS(inactive); /* struct vnode *a_vp;
		    struct proc *a_p; */

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_inactive: vp %s\n",VTOPFS(ap->a_vp)->pstr);
 VOP_UNLOCK(ap->a_vp,0);
 /* is this really all we need to do? */
 /* what is VOP_INACTIVE's interface contract? */
 return(0);
}

static int pfs_reclaim(void *v)
{
 ARGS(reclaim); /* struct vnode *a_vp;
		   struct proc *a_p; */
 struct pfs_freeid_req q;
 struct pfsnode *pn;
 struct pfs_mount *m;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_vp->v_mount);
 printf("pfs_reclaim: vp %s\n",VTOPFS(ap->a_vp)->pstr);
 m = VFSTOPFS(ap->a_vp->v_mount);
 q.id = VTOPFS(ap->a_vp)->id;
 pfs_call(ap->a_vp->v_mount,PFSOP_FREEID,&q,sizeof(q),0,PFS_NO_REPLY,0,0);
 m->vnode_count --;
 if (m->vnode_count < 0) panic("pfs_reclaim: vnode_count underflow");
 pfs_vntbl_remove(ap->a_vp);
 pn = VTOPFS(ap->a_vp)->parent;
 if (pn) vrele(pn->vnode);
 free(ap->a_vp->v_data,M_PFSNODE);
 ap->a_vp->v_data = 0;
 return(0);
}

static int pfs_lock(void *v)
{
 /*
  * pfs_lock is called from within pfs_get_vnode, before pstr is set,
  *  so we can't print the vp prettily here (at least not easily).
  */
 printf("pfs_lock: vp %p\n",(void *)((struct vop_lock_args *)v)->a_vp);
 ROOT_USECOUNT_CHECK(((struct vop_lock_args *)v)->a_vp->v_mount);
 return(genfs_lock(v));
}

static int pfs_unlock(void *v)
{
 /*
  * I don't think pfs_unlock is ever called before the vnode's pstr
  *  gets set, but for symmetry with pfs_lock, we do it the same way.
  */
 printf("pfs_unlock: vp %p\n",(void *)((struct vop_unlock_args *)v)->a_vp);
 ROOT_USECOUNT_CHECK(((struct vop_unlock_args *)v)->a_vp->v_mount);
 return(genfs_unlock(v));
}

static int pfs_bmap(void *v)
{
 ARGS(bmap); /* struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp; */

 /* vp locked on entry and on all exits */
 /* *vpp unlocked on ok exit */
 panic("pfs_bmap");
}

static int pfs_print(void *v)
{
 ARGS(print); /* struct vnode *a_vp; */

 /* vp locking status untouched */
 panic("pfs_print");
}

static int pfs_islocked(void *v)
{
 printf("pfs_islocked: vp %s\n",VTOPFS(((struct vop_islocked_args *)v)->a_vp)->pstr);
 ROOT_USECOUNT_CHECK(((struct vop_islocked_args *)v)->a_vp->v_mount);
 return(genfs_islocked(v));
}

static int pfs_pathconf(void *v)
{
 ARGS(pathconf); /* struct vnode *a_vp;
		    int a_name;
		    register_t *a_retval; */

 /* vp locked on entry and all exits */
 panic("pfs_pathconf");
}

static int pfs_advlock(void *v)
{
 ARGS(advlock); /* struct vnode *a_vp;
		   caddr_t a_id;
		   int a_op;
		   struct flock *a_fl;
		   int a_flags; */

 /* vp unlocked on entry and all exits */
 panic("pfs_advlock");
}

static int pfs_blkatoff(void *v)
{
 ARGS(blkatoff); /* struct vnode *a_vp;
		    off_t a_offset;
		    char **a_res;
		    struct buf **a_bpp; */

 /* vp locked on entry and all exits */
 panic("pfs_blkatoff");
}

static int pfs_valloc(void *v)
{
 ARGS(valloc); /* struct vnode *a_vp;
		  int a_mode;
		  struct ucred *a_cred;
		  struct vnode **a_vpp; */

 /* vp locked on entry and all exits */
 panic("pfs_valloc");
}

static int pfs_balloc(void *v)
{
 ARGS(balloc); /* struct vnode *a_vp;
		  off_t a_startoffset;
		  int a_size;
		  struct ucred *a_cred;
		  int a_flags;
		  struct buf **a_bpp; */

 /* vp locked on entry and all exits */
 panic("pfs_balloc");
}

static int pfs_reallocblks(void *v)
{
 ARGS(reallocblks); /* struct vnode *a_vp;
		       struct cluster_save *a_buflist; */

 /* vp locked on entry and all exits */
 panic("pfs_reallocblks");
}

static int pfs_vfree(void *v)
{
 ARGS(vfree); /* struct vnode *a_pvp;
		 ino_t a_ino; */

 /* vp locked on entry and all exits */
 panic("pfs_vfree");
}

static int pfs_truncate(void *v)
{
 ARGS(truncate); /* struct vnode *a_pvp;
		    off_t a_length;
		    int a_flags;
		    struct ucred *a_cred;
		    struct proc *a_p; */

 /* vp locked on entry and all exits */
 panic("pfs_truncate");
}

static int pfs_update(void *v)
{
 ARGS(update); /* struct vnode *a_pvp;
		  struct timespec *a_access;
		  struct timespec *a_modify;
		  int a_waitfor; */

 /* vp locked on entry and all exits */
 panic("pfs_update");
}

/* I'm not entirely sure what this op is for, but nothing seems to do
   anything with its return value, so I assume it's purely advisory.
   The genfs version (genfs_lease_check) supports this theory. */
static int pfs_lease(void *v)
{
 ARGS(lease); /* struct vnode *a_vp;
		 struct proc *a_p;
		 struct ucred *a_cred;
		 int a_flag; */

 /* vp locking status untouched */
 ROOT_USECOUNT_CHECK(((struct vop_lease_args *)v)->a_vp->v_mount);
 return(0);
}

static int pfs_whiteout(void *v)
{
 ARGS(whiteout); /* struct vnode *a_dvp;
		    struct componentname *a_cnp;
		    int a_flags; */
 struct pfs_symlink_rep p;
 int err;
 struct componentname *cn;

 ap = v;
 ROOT_USECOUNT_CHECK(ap->a_dvp->v_mount);
 cn = ap->a_cnp;
 switch (ap->a_flags)
  { case LOOKUP:
	{ struct pfs_whiteout_req q;
	  printf("pfs_whiteout: TEST in %s\n",VTOPFS(ap->a_dvp)->pstr);
	  q.dir = VTOPFS(ap->a_dvp)->id;
	  q.op = PFS_WHITE_TEST;
	  SETCRED(&q.cred,NOCRED);
	  err = pfs_call(ap->a_dvp->v_mount,PFSOP_WHITEOUT,&q,sizeof(q),&p,sizeof(p),0,0);
	  if (err)
	   { printf("pfs_whiteout: pfs_call failed (%d)\n",err);
	     return(err);
	   }
	  if (p.err)
	   { printf("pfs_whiteout: userland returned error %d\n",p.err);
	     return(p.err);
	   }
	  printf("pfs_whiteout: success\n");
	  err = 0;
	}
       break;
    case CREATE:
	{ int op;
	  const char *opstr;
	  char *qbuf;
	  struct pfs_whiteout_req *q;
	  op = PFS_WHITE_CREATE;
	  opstr = "CREATE";
	  if (0)
	   {
    case DELETE:
	     op = PFS_WHITE_DELETE;
	     opstr = "DELETE";
	   }
	  ROFS_CHECK(ap->a_dvp->v_mount);
	  if (! (cn->cn_flags & HASBUF)) panic("pfs_whiteout: no name buffer");
	  do
	   { printf("pfs_whiteout: %s %.*s in %s\n",opstr,(int)cn->cn_namelen,cn->cn_nameptr,VTOPFS(ap->a_dvp)->pstr);
	     qbuf = malloc(sizeof(struct pfs_whiteout_req)+cn->cn_namelen,M_TEMP,M_WAITOK);
	     q = (void *) qbuf;
	     q->dir = VTOPFS(ap->a_dvp)->id;
	     q->op = op;
	     SETCRED(&q->cred,cn->cn_cred);
	     bcopy(cn->cn_nameptr,qbuf+sizeof(struct pfs_whiteout_req),cn->cn_namelen);
	     err = pfs_call(ap->a_dvp->v_mount,PFSOP_WHITEOUT,qbuf,sizeof(struct pfs_whiteout_req)+cn->cn_namelen,&p,sizeof(p),0,0);
	     if (err)
	      { printf("pfs_whiteout: pfs_call failed (%d)\n",err);
		break;
	      }
	     if (p.err)
	      { printf("pfs_whiteout: userland returned error %d\n",p.err);
		err = p.err;
		break;
	      }
	     printf("pfs_whiteout: success\n");
	     err = 0;
	   } while (0);
	  return(err);
	}
       break;
    default:
       panic("pfs_whiteout: bad op");
       break;
  }
 if (! (cn->cn_flags & SAVESTART)) free(cn->cn_pnbuf,M_NAMEI);
 return(err);
}

struct vnodeopv_entry_desc pfs_vnodeop_entries[]
 = { { &vop_default_desc, vn_default_error },
     { &vop_lookup_desc, pfs_lookup },
     { &vop_create_desc, pfs_create },
     { &vop_mknod_desc, pfs_mknod },
     { &vop_open_desc, pfs_open },
     { &vop_close_desc, pfs_close },
     { &vop_access_desc, pfs_access },
     { &vop_getattr_desc, pfs_getattr },
     { &vop_setattr_desc, pfs_setattr },
     { &vop_read_desc, pfs_read },
     { &vop_write_desc, pfs_write },
     { &vop_ioctl_desc, pfs_ioctl },
     { &vop_fcntl_desc, pfs_fcntl },
     { &vop_poll_desc, pfs_poll },
     { &vop_revoke_desc, pfs_revoke },
     { &vop_mmap_desc, pfs_mmap },
     { &vop_fsync_desc, pfs_fsync },
     { &vop_seek_desc, pfs_seek },
     { &vop_remove_desc, pfs_remove },
     { &vop_link_desc, pfs_link },
     { &vop_rename_desc, pfs_rename },
     { &vop_mkdir_desc, pfs_mkdir },
     { &vop_rmdir_desc, pfs_rmdir },
     { &vop_symlink_desc, pfs_symlink },
     { &vop_readdir_desc, pfs_readdir },
     { &vop_readlink_desc, pfs_readlink },
     { &vop_abortop_desc, pfs_abortop },
     { &vop_inactive_desc, pfs_inactive },
     { &vop_reclaim_desc, pfs_reclaim },
     { &vop_lock_desc, pfs_lock },
     { &vop_unlock_desc, pfs_unlock },
     { &vop_bmap_desc, pfs_bmap },
     { &vop_print_desc, pfs_print },
     { &vop_islocked_desc, pfs_islocked },
     { &vop_pathconf_desc, pfs_pathconf },
     { &vop_advlock_desc, pfs_advlock },
     { &vop_blkatoff_desc, pfs_blkatoff },
     { &vop_valloc_desc, pfs_valloc },
     { &vop_balloc_desc, pfs_balloc },
     { &vop_reallocblks_desc, pfs_reallocblks },
     { &vop_vfree_desc, pfs_vfree },
     { &vop_truncate_desc, pfs_truncate },
     { &vop_update_desc, pfs_update },
     { &vop_lease_desc, pfs_lease },
     { &vop_whiteout_desc, pfs_whiteout },
     { 0, 0 } };
struct vnodeopv_desc pfs_vnodeop_opv_desc
 = { &pfs_vnodeop_p, &pfs_vnodeop_entries[0] };
