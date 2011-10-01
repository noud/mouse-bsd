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

#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/unpcb.h>
#include <sys/filedesc.h>
#include <sys/socketvar.h>

#include <miscfs/pfs/pfs.h>
#include <miscfs/pfs/pfs-internal.h>

/*
 * Three functions in this file construct struct uios by hand for I/O.
 *  They store a nil pointer in uio_procp; this depends on uiomove()
 *  not using it.  We aren't really a process and don't have a proc
 *  pointer to put there - we must be running in a process context, and
 *  could conceivably use its proc pointer, but that ends up with the
 *  process apparently doing I/O it really isn't doing - the I/O is
 *  actually internal to the filesystem.  The only process I think it
 *  would be reasonable to use here is the userland daemon, but (a)
 *  that's not well-defined (there can be multiple daemons) and (b) we
 *  don't have a proc pointer handy for it in any case.
 *
 * The commenting here assumes you've read the interface contracts in
 *  pfs-internal.h for the externally-visible functions.
 */

/*
 * If PFS_DUMP_DATA is defined ("options PFS_DUMP_DATA" in your kernel
 *  config), we print the full contents of all requests and replies.
 *  This routine is used to dump out the actual data in this case.
 */
#ifdef PFS_DUMP_DATA
static void dump_hex(const void *data, int len)
{
 const unsigned char *dp;

 for (dp=data;len>0;dp++,len--) printf(" %02x",*dp);
}
#endif

/*
 * Send a request to userland.  mp is the mount point; iov and niov
 *  describe the data to send; and psock is the socket to accompany it,
 *  or nil if none should.
 *
 * This is a heavily stripped down version of sendit() from
 *  kern/uipc_syscalls.c, with called routines inlined and also
 *  stripped down.  Some return values that we might someday want to
 *  handle specially are:
 *
 *	EPIPE
 *		indicates userland has gone away
 *	EINTR
 *		as usual
 *	ERESTART
 *		signal received, but action is set to restart
 *	EWOULDBLOCK
 *		userland set the socket nonblocking, and it's full; we
 *		might want to retry, but I'm more inclined to just let
 *		userland reap what userland has sown.
 *
 * This code goes under the hood of AF_LOCAL quite a bit, to arrange to
 *  pass the socket as SCM_RIGHTS.  We could avoid much of this, but
 *  only by arranging for some user-level file descriptor to refer to
 *  the socket we want to pass, so that unp_internalize can find it
 *  where it expects it.  It's admittedly not a clear-cut decision
 *  which is less gross, but we choose this way, even though it means
 *  duplicating pieces of sosend(), uipc_usrreq(), unp_internalize(),
 *  and unp_output().  (Really, the user-level fd contained in
 *  SCM_RIGHTS should be converted to a struct file * much earlier in
 *  the sendmsg() call chain, which would allow us to duplicate a good
 *  deal less code here.  But that would mean hacking on much code
 *  elsewhere, instead of just adding code.)
 *
 * If psock is non-nil, then on error return the caller is still
 *  responsible for it; on success, it has been passed off to userland
 *  (or at least it's sitting in a socket buffer) and the caller should
 *  just throw away its pointer.
 */
static int pfs_sosend(struct mount *mp, struct iovec *iov, int niov, struct socket *psock)
{
 struct uio u;
 struct socket *sock;
 struct socket *usock;
 int len;
 int err;
 struct mbuf *ctl;
 int s;
 struct mbuf *data;
 struct mbuf **dtail;
 struct mbuf *m;
 int mlen;
 struct file *f;

 switch (niov)
  { case 1:
       len = iov[0].iov_len;
       break;
    case 2:
       len = iov[0].iov_len + iov[1].iov_len;
       break;
    default:
       panic("pfs_sosend niov");
       break;
  }
#ifdef PFS_DUMP_DATA
 printf("pfs_sosend: > %d:",len);
 dump_hex(iov[0].iov_base,iov[0].iov_len);
 if (niov > 1) dump_hex(iov[1].iov_base,iov[1].iov_len);
 printf("\n");
#endif
 /* Convert the data to an mbuf chain.  Lifted mostly from sosend().
    First, do some locking and checking. */
 sock = VFSTOPFS(mp)->sock->f_data;
 err = sblock(&sock->so_snd,M_WAITOK);
 if (err) return(err);
 /* do we want to incremement anyone's p_stats->p_ru.ru_msgsnd? */
restart:;
 s = splsoftnet();
 if ((sock->so_state & (SS_CANTSENDMORE|SS_ISCONNECTED)) != SS_ISCONNECTED)
  { splx(s);
    sbunlock(&sock->so_snd);
    printf("pfs_sosend: EPIPE\n");
    return(EPIPE);
  }
 /*
  * Don't worry about len > sb_hiwat.  We made sure the socket buffer
  *  sizes were large enough at mount time, and took over the fd; while
  *  it is possible that userland kept another fd on the socket and
  *  shrank the buffers later, that counts as userland reaping what
  *  userland sowed, and it doesn't break us in any case; it just means
  *  that the only way out of this loop may be EINTR, or growing the
  *  socket buffers again.
  */
 if (len > sbspace(&sock->so_snd))
  { sbunlock(&sock->so_snd);
    err = sbwait(&sock->so_snd);
    splx(s);
    if (err) return(err);
    goto restart;
  }
 splx(s);
 /* See the file comment header re uio_procp. */
 u.uio_iov = iov;
 u.uio_iovcnt = niov;
 u.uio_segflg = UIO_SYSSPACE;
 u.uio_rw = UIO_WRITE;
 u.uio_procp = 0;
 u.uio_offset = 0;
 u.uio_resid = len;
 /* Allocate mbufs and copy into them. */
 data = 0;
 dtail = &data;
 while (u.uio_resid > 0)
  { if (! data)
     { MGETHDR(m,M_WAIT,MT_DATA);
       mlen = MHLEN;
       m->m_pkthdr.len = 0;
       m->m_pkthdr.rcvif = 0;
     }
    else
     { MGET(m,M_WAIT,MT_DATA);
       mlen = MLEN;
     }
    if ( (u.uio_resid >= MINCLSIZE) &&
	 (({MCLGET(m,M_WAIT);}), (m->m_flags & M_EXT)) )
     { mlen = MCLBYTES;
       if (! data)
	{ mlen -= max_hdr;
	  m->m_data += max_hdr;
	}
     }
    else
     { if (!data && (u.uio_resid < mlen)) MH_ALIGN(m,u.uio_resid);
     }
    m->m_len = min(u.uio_resid,mlen);
    err = uiomove(mtod(m,void *),m->m_len,&u);
    *dtail = m;
    dtail = &m->m_next;
    data->m_pkthdr.len += m->m_len;
    if (err)
     { m_freem(data);
       return(err);
     }
  }
 /*
  * Create an mbuf holding the control stuff, if any.  This is largely
  *  stolen from unp_internalize(), with the struct file allocation
  *  code lifted from falloc().
  */
 if (psock)
  { struct cmsghdr cmh;
    if (nfiles >= maxfiles)
     { printf("pfs_sosend: nfiles (%d) >= maxfiles (%d) -> ENFILE\n",nfiles,maxfiles);
       m_freem(data);
       return(ENFILE);
     }
    nfiles ++;
    /* XXX why aren't file_pool and socketops in .h files? */
     { extern struct pool file_pool;
       extern struct fileops socketops;
       f = pool_get(&file_pool,PR_WAITOK);
       /*
	* All pointer elements of f are set below; we assume a pointer
	*  can be bzero()ed and then set to a valid value, though I'm
	*  not certain that's guaranteed.
	*/
       bzero(f,sizeof(*f));
       LIST_INSERT_HEAD(&filehead,f,f_list);
       f->f_count = 1;
       f->f_cred = VFSTOPFS(mp)->cred;
       crhold(f->f_cred);
       f->f_flag = FREAD | FWRITE;
       f->f_type = DTYPE_SOCKET;
       f->f_ops = &socketops;
       f->f_data = psock;
       f->f_msgcount = 1;
     }
    cmh.cmsg_len = ALIGN(sizeof(cmh)) + sizeof(struct file *);
    cmh.cmsg_level = SOL_SOCKET;
    cmh.cmsg_type = SCM_RIGHTS;
    ctl = m_get(M_WAIT,MT_CONTROL);
    if (cmh.cmsg_len > MLEN) MEXTMALLOC(ctl,cmh.cmsg_len,M_WAITOK);
    ctl->m_len = cmh.cmsg_len;
    bcopy(&cmh,mtod(ctl,char *),sizeof(cmh));
    bcopy(&f,mtod(ctl,char *)+ALIGN(sizeof(cmh)),sizeof(struct file *));
  }
 else
  { ctl = 0;
    f = 0;
  }
 /*
  * We now have the mbufs.  Check that the connection hasn't gone poof
  *  (at numerous points above, we could have slept for resources).
  */
 s = splsoftnet();
 if ((sock->so_state & (SS_CANTSENDMORE|SS_ISCONNECTED)) != SS_ISCONNECTED)
  { splx(s);
    sbunlock(&sock->so_snd);
    printf("pfs_sosend: EPIPE (2)\n");
    m_freem(data);
    m_freem(ctl);
    /*
     * closef() would do the wrong thing here.  Mostly, it would
     *  soclose() the socket, which our caller isn't expecting on error
     *	returns.
     */
    if (f) ffree(f);
    return(EPIPE);
  }
 /* Actually send it.  Snitched from unp_output(). */
 usock = sotounpcb(sock)->unp_conn->unp_socket;
 /* XXX why isn't sun_noname in a .h file? */
 if (! sbappendaddr(&usock->so_rcv,({ extern struct sockaddr_un sun_noname;
				      (void *)&sun_noname;
				    }),data,ctl))
  { splx(s);
    sbunlock(&sock->so_snd);
    printf("pfs_sosend: EINVAL\n");
    m_freem(data);
    m_freem(ctl);
    if (f) ffree(f);
    return(EINVAL);
  }
 splx(s);
 /* Whee!  Everything worked. */
 sorwakeup(usock);
 sbunlock(&sock->so_snd);
 return(0);
}

/*
 * Receive a response from userland.
 *
 * This is a heavily stripped down version of recvit() from
 *  kern/uipc_syscalls.c, with some called routines inlined and
 *  stripped down.  Some return values that we might someday want to
 *  handle specially are:
 *
 *	EINTR
 *		as usual
 *	ERESTART
 *		signal received, but action is set to restart
 *	EWOULDBLOCK
 *		userland set the socket nonblocking, and it's empty; we
 *		might want to retry, but I'm more inclined to just let
 *		userland reap what userland has sown.
 */
static int pfs_sorecv(struct mount *mp, struct iovec *iov, int niov, int *lenp)
{
 struct uio u;
 struct socket *s;
 int len;
 int err;
 int flags;
 struct mbuf *from;
 int got;
 struct iovec iovsave[2];

 iovsave[0] = iov[0];
 switch (niov)
  { case 1:
       len = iov[0].iov_len;
       if (lenp) panic("pfs_sorecv: lenp but only one iov");
       break;
    case 2:
       len = iov[0].iov_len + iov[1].iov_len;
       iovsave[1] = iov[1];
       break;
    default:
       panic("pfs_sorecv niov");
       break;
  }
 /* See the file comment header re uio_procp. */
 u.uio_iov = iov;
 u.uio_iovcnt = niov;
 u.uio_segflg = UIO_SYSSPACE;
 u.uio_rw = UIO_READ;
 u.uio_procp = 0;
 u.uio_offset = 0;
 u.uio_resid = len;
 s = VFSTOPFS(mp)->sock->f_data;
 flags = 0;
 from = 0;
 err = (*s->so_receive)(s,&from,&u,0,0,&flags);
 if (from) m_freem(from);
 if (err)
  { printf("pfs_sorecv: error %d\n",err);
    return(err);
  }
 got = len - u.uio_resid;
#ifdef PFS_DUMP_DATA
 printf("pfs_sorecv: < %d:",got);
 dump_hex(iovsave[0].iov_base,(got<iovsave[0].iov_len)?got:iovsave[0].iov_len);
 if ((niov > 1) && (got > iovsave[0].iov_len)) dump_hex(iovsave[1].iov_base,got-iovsave[0].iov_len);
 printf("\n");
#endif
 if (u.uio_resid == len) return(EPIPE); /* EOF (= userland vanished?) */
 if (lenp)
  { if (got < iovsave[0].iov_len)
     { printf("pfs_sorecv: short receive (got %ld, min %ld)\n",(long int)got,(long int)iovsave[0].iov_len);
       return(EIO);
     }
    *lenp = got - iovsave[0].iov_len;
  }
 else
  { if (u.uio_resid != 0)
     { printf("pfs_sorecv: short receive (len %ld, resid %ld)\n",(long int)len,(long int)u.uio_resid);
       return(EIO);
     }
  }
 return(0);
}

/*
 * Drain anything queued in the socket.
 *
 * Because we single-thread requests to userland, there normally will
 *  be nothing there.  However, if a previous call got a signal and
 *  EINTRed out of pfs_sorecv, there may be a reply to that request
 *  there.  Unfortunately we can't say it _will_ be there, as the
 *  daemon may not have gotten around to sending it yet.  We really
 *  need to rework this mechanism; the current paradigm can't
 *  simultaneously ensure that pending requests stay pending until
 *  they're satisfied and that you can interrupt out of a request that
 *  isn't responding.  But if the userland daemon is still running,
 *  draining here allows you to recover by leaving the filesystem idle
 *  long enough for the daemon to respond to any pending request.
 */
static void pfs_sodrain(struct mount *mp)
{
 struct uio u;
 struct socket *s;
 int err;
 int flags;
 struct mbuf *from;
 struct iovec iov;
 char junk;

 s = VFSTOPFS(mp)->sock->f_data;
 while (1)
  { iov.iov_base = &junk;
    iov.iov_len = 1;
    /* See the file comment header re uio_procp. */
    u.uio_iov = &iov;
    u.uio_iovcnt = 1;
    u.uio_segflg = UIO_SYSSPACE;
    u.uio_rw = UIO_READ;
    u.uio_procp = 0;
    u.uio_offset = 0;
    u.uio_resid = 1;
    flags = MSG_DONTWAIT;
    from = 0;
    err = (*s->so_receive)(s,&from,&u,0,0,&flags);
    if (from) m_freem(from);
    if (err == EWOULDBLOCK) return;
  }
}

/*
 * Return a kernel ID.  Arguably we should check which IDs are
 *  outstanding and make sure we don't clash, though I'm not sure how
 *  necessary that actually is.
 */
static unsigned int id_alloc(void)
{
 static volatile unsigned int n;
 int id;
 int s;

 s = splhigh();
 id = n ++;
 splx(s);
 return(id);
}

/*
 * Make a call to the userland server.
 *
 * mp is the mount point in question.  op is the operation we are
 *  asking userland to perform (one of the PFSOP_* values).  reqdata
 *  and reqdlen describe the additional data to accompany the request;
 *  if reqdlen is zero, no data is to accompany, and reqdata will not
 *  be used.  repdata and repdlen describe the data we expect back.  If
 *  repdlen is zero, no data is expected back, but we still require a
 *  response; if repdlen is PFS_NO_REPLY, no reply at all is expected
 *  back, and pfs_call returns once the request is formulated and sent.
 *  (If repdlen is zero or PFS_NO_REPLY, repdata will not be used.)
 *  sockp is a nil pointer if no socket is to be sent with the request;
 *  otherwise, a socket pair will be created, one end will be sent with
 *  the request, and the other end will be stored into *sockp.
 *
 * Return value is 0 if all went well, or an errno otherwise.  If sockp
 *  is non-nil, *sockp is meaningful only on success.  It's not clear
 *  how useful the return value is for PFS_NO_REPLY calls, but it's
 *  provided just the same.
 *
 * Eventually this will support multiple requests simultaneously
 *  pending.  For the moment it's all done synchronously.
 */
int pfs_call(struct mount *mp, int op, const void *reqdata, int reqdlen, void *repdata, int repdlen, int *replenp, struct socket **sockp)
{
 static volatile char busy = 0;
 static volatile char want = 0;
 static char sleepchan; /* grrr, tsleep's arg should volatile const void * */
 int s;
 int err;
 unsigned int id;
 struct pfs_req req;
 struct pfs_rep rep;
 struct iovec iov[2];
 struct socket *so_user;
 struct socket *so_kernel;

 id = id_alloc();
 printf("pfs_call %u: mp %p, %d@%p->%d->%d@%p (%s socket)\n",id,(void *)mp,reqdlen,reqdata,op,repdlen,repdata,sockp?"with":"no");
 so_user = 0;
 so_kernel = 0;
 do <"reterr">
  { do <"relret">
     { do <"success">
	{ if (sockp)
	   { err = socreate(AF_LOCAL,&so_user,SOCK_STREAM,0);
	     if (err) break <"reterr">;
	     err = socreate(AF_LOCAL,&so_kernel,SOCK_STREAM,0);
	     if (err) break <"reterr">;
	     err = soconnect2(so_user,so_kernel);
	     if (err) break <"reterr">;
	   }
	  s = splhigh();
	  while (busy)
	   { want = 1;
	     printf("pfs_call %u: busy, sleeping\n",id);
	     err = tsleep(&sleepchan,PZERO|PCATCH,"pfslock",0);
	     if (err)
	      { splx(s);
		break <"reterr">;
	      }
	   }
	  busy = 1;
	  splx(s);
	  /* See the comment header for pfs_sodrain. */
	  pfs_sodrain(mp);
	  req.op = op;
	  req.id = id;
	  iov[0].iov_base = &req;
	  iov[0].iov_len = sizeof(struct pfs_req);
	  if (reqdlen > 0)
	   { /* Grrr.  It seems that using iovecs is inconsistent with
		strict thorough const poisoning. */
	     iov[1].iov_base = (void *) reqdata;
	     iov[1].iov_len = reqdlen;
	     err = pfs_sosend(mp,&iov[0],2,so_user);
	   }
	  else
	   { err = pfs_sosend(mp,&iov[0],1,so_user);
	   }
	  if (err)
	   { printf("pfs_call %u: pfs_sosend failed (%d)\n",id,err);
	     break <"relret">;
	   }
	  so_user = 0;
	  if (repdlen == PFS_NO_REPLY)
	   { printf("pfs_call %u: no reply expected\n",id);
	     break <"success">;
	   }
	  iov[0].iov_base = &rep;
	  iov[0].iov_len = sizeof(struct pfs_rep);
	  if (repdlen > 0)
	   { iov[1].iov_base = repdata;
	     iov[1].iov_len = repdlen;
	     err = pfs_sorecv(mp,&iov[0],2,replenp);
	   }
	  else
	   { err = pfs_sorecv(mp,&iov[0],1,replenp);
	   }
	  if (err)
	   { printf("pfs_call %u: pfs_sorecv failed (%d)\n",id,err);
	     break <"relret">;
	   }
	  if ((err == 0) && (rep.id != req.id))
	   { printf("pfs_call %u: request id %u, reply id %u\n",id,req.id,rep.id);
	     err = EIO;
	   }
	  printf("pfs_call %u: success\n",id);
	} while (0);
       if (sockp) *sockp = so_kernel;
       so_kernel = 0;
     } while (0);
    s = splhigh();
    if (want)
     { wakeup(&sleepchan);
       want = 0;
     }
    busy = 0;
    splx(s);
  } while (0);
 if (so_user) soclose(so_user);
 if (so_kernel) soclose(so_kernel);
 return(err);
}

/*
 * Initialize the vnode table for a mount point.  Since our current
 *  data structure consists of just a doubly-linked list, all we have
 *  to do here is initialize the root pointer.
 */
void pfs_vntbl_init(struct pfs_mount *m)
{
 m->vnodetbl = 0;
}

/*
 * Clean up the vnode table for a mount point that's about to go away.
 *  With our current data structure, there's nothing to do here.
 */
void pfs_vntbl_done(struct pfs_mount *m)
{
 m=m; /* shut up -Wunused */
}

/*
 * Set a vnode's type.  This would be just an assignment to v->v_type
 *  except that we also want to set the vnode's pstr, for debugging
 *  output.
 */
void pfs_set_type(struct vnode *v, enum vtype t)
{
 const char *typestr;

 v->v_type = t;
 switch (t)
  { case VNON:  typestr = "VNON";  break;
    case VREG:  typestr = "VREG";  break;
    case VDIR:  typestr = "VDIR";  break;
    case VBLK:  typestr = "VBLK";  break;
    case VCHR:  typestr = "VCHR";  break;
    case VLNK:  typestr = "VLNK";  break;
    case VSOCK: typestr = "VSOCK"; break;
    case VFIFO: typestr = "VFIFO"; break;
    case VBAD:  typestr = "VBAD";  break;
    default:
       sprintf(VTOPFS(v)->pstr,"%d(?%d,%p)",VTOPFS(v)->id,(int)t,(void *)v);
       return;
       break;
  }
 sprintf(VTOPFS(v)->pstr,"%d(%s,%p)",VTOPFS(v)->id,typestr,(void *)v);
}

/*
 * This is pretty simple.  Just look for an existing vnode; if found,
 *  lock and return.  If not, create it and set it up.
 */
int pfs_get_vnode(struct mount *mp, struct vnode **vpp, int id)
{
 struct pfs_mount *m;
 struct pfsnode *n;
 int err;
 struct vnode *v;

 m = VFSTOPFS(mp);
 for (n=m->vnodetbl;n;n=n->vtbl_f)
  { if (n->id == id)
     { err = vget(n->vnode,LK_EXCLUSIVE|LK_RECURSEFAIL);
       if (err == 0)
	{ *vpp = n->vnode;
	  return(0);
	}
       else
	{ return(err);
	}
     }
  }
 err = getnewvnode(VT_PFS,mp,pfs_vnodeop_p,&v);
 if (err)
  { printf("pfs_get_vnode: getnewvnode failed\n");
    return(err);
  }
 n = malloc(sizeof(struct pfsnode)+64,M_PFSNODE,M_WAITOK);
 v->v_data = n;
 n->id = id;
 n->vnode = v;
 n->vtbl_f = m->vnodetbl;
 m->vnodetbl = n;
 n->vtbl_b = 0;
 if (n->vtbl_f) n->vtbl_f->vtbl_b = n;
 n->parent = 0;
 n->pstr = (void *) (n+1);
 *vpp = v;
 vn_lock(v,LK_EXCLUSIVE|LK_RETRY);
 pfs_set_type(v,VNON);
 m->vnode_count ++;
 if (m->vnode_count < 0) panic("pfs_get_vnode: vnode_count overflow");
 return(0);
}

/*
 * Remove a vnode from the mount point's table of vnodes.  Called when
 *  a vnode is being destroyed (or, more precisely, cleaned out for
 *  potential allocation by another filesystem).
 */
void pfs_vntbl_remove(struct vnode *v)
{
 struct pfsnode *n;

 n = v->v_data;
 if (n->vtbl_f) n->vtbl_f->vtbl_b = n->vtbl_b;
 (n->vtbl_b ? n->vtbl_b->vtbl_f : VFSTOPFS(v->v_mount)->vnodetbl) = n->vtbl_f;
}

/*
 * Construct a struct pfs_cred to correspond to a struct ucred *.  See
 *  pfs.h for why pfs_cred exists; there's not much to add here to
 *  what's said there.
 */
void pfs_setcred(struct pfs_cred *p, const struct ucred *u)
{
 if (u == NOCRED)
  { p->flags = PFS_NOCRED;
    bzero(&p->cred,sizeof(p->cred));
  }
 else
  { p->flags = 0;
    p->cred = *u;
    p->cred.cr_ref = 0;
  }
}
