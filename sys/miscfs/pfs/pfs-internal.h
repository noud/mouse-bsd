#ifndef _PFS_INTERNAL_H_5059cff8_
#define _PFS_INTERNAL_H_5059cff8_

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
 * Defines and structs and such that are internal to pfs.  Nobody but
 *  pfs kernel source should include this file.
 */

#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/socket.h>

/*
 * Current major and minor versions.  See pfs.h for why these are here
 *  rather than there (ie, why they're an unexported interface).
 */
#define MAJOR_VERSION 1
#define MINOR_VERSION 3

/*
 * root is the root vnode.  sock is our end of the connection to
 *  userland.  vnodetbl is a list of all pfsnodes that exist.  cred is
 *  the struct ucred from the mount; we hang onto this so that we have
 *  something to use when creating open file structs for auxiliary
 *  sockets for requests.  vnode_count is a count of the number of
 *  vnodes belonging to this mount; we keep this so we can tell whether
 *  it's safe to unmount.
 */
struct pfs_mount {
  struct vnode *root;
  struct file *sock;
  struct pfsnode *vnodetbl;
  struct ucred *cred;
  int vnode_count;
  } ;

/*
 * In order to handle .. lookups internally (ie, without calling to
 *  userland), every directory node includes a pointer to its parent.
 */
struct pfsnode {
  struct pfsnode *vtbl_f;	/* list of all pfsnodes for this mount */
  struct pfsnode *vtbl_b;
  int id;			/* userland interface id */
  struct vnode *vnode;		/* backpointer to vnode */
  struct pfsnode *parent;	/* nil if non-directory, or if root */
  char *pstr;			/* print string, for debug messages */
  } ;

/*
 * Routine to deal with converting a struct ucred * into a struct
 *  pfs_cred.  See pfs.h for more on struct pfs_cred and why we don't
 *  just use the ucred itself.
 */
extern void pfs_setcred(struct pfs_cred *, const struct ucred *);
#define SETCRED pfs_setcred

/*
 * Convert PFS mount structs or vnodes to pfs_mount or pfsnode structs.
 */
#define VFSTOPFS(mp) ((struct pfs_mount *)((mp)->mnt_data))
#define VTOPFS(vp) ((struct pfsnode *)(vp)->v_data)

/*
 * The PFS vnode and vfs operations vectors.  (The vnode operations
 *  vector is actually more complicated than a simple operations
 *  vector, like all vnode operations vectors.)
 */
extern int (**pfs_vnodeop_p)(void *);
extern struct vfsops pfs_vfsops;

/*
 * Make a call to userland.  Args, in order:
 *
 *	The relevant mount point.
 *	The operation (PFSOP_*)
 *	The pointer/length for the additional data to follow the struct
 *	  pfs_req, if any.  (If the length is zero, the pointer is not
 *	  used and may be anything, including nil.)
 *	The pointer/length describing the data expected back following
 *	  the struct pfs_rep, if any.  If no reply at all is expected,
 *	  specify a nil pointer with PFS_NO_REPLY for the length;
 *	  specifying a zero length indicates that a reply is expected
 *	  but that nothing is expected following the struct pfs_rep.
 *	A pointer which, if non-nil, has the actual reply length stored
 *	  through it.  If a variable-size response is expected, specify
 *	  the maximum expected length in the previous argument and give
 *	  a non-nil pointer here to receive the actual length; if this
 *	  pointer is nil, a reply with other than the length indicated
 *	  by the previous argument is an error.
 *	A pointer which, if non-nil, indicates that an auxiliary socket
 *	  should be created and passed with the request; the kernel's
 *	  end of the connection is stored through here.  If nil, no
 *	  socket is passed.  (What use, if any, is made of the
 *	  connection is between userland and our caller.)
 */
extern int pfs_call(struct mount *, int, const void *, int, void *, int, int *, struct socket **);
#define PFS_NO_REPLY (-1)

/*
 * Init, done, and remove routines for the PFS vnode table.
 */
extern void pfs_vntbl_init(struct pfs_mount *);
extern void pfs_vntbl_done(struct pfs_mount *);
extern void pfs_vntbl_remove(struct vnode *);

/*
 * Get a vnode, given the mount point and kernel ID.  The vnode is
 *  returned through the second argument and is always returned locked.
 *  Return value is an errno, or 0 for success.  (On error, the second
 *  argument may or may not be stored through; if it is, the value
 *  stored is not good for anything and must be ignored.)
 */
extern int pfs_get_vnode(struct mount *, struct vnode **, int);

/*
 * Set the type of a PFS vnode.  This would be just an assignment to
 *  the v_type member except that we want to do a little more than that
 *  when we set a vnode's type.
 */
extern void pfs_set_type(struct vnode *, enum vtype);

/*
 * Someday we may have our own M_ values, but for now...
 */
#define M_PFSNODE M_MISCFSNODE
#define M_PFSMNT M_MISCFSMNT

/*
 * Debugging check.  This is called at the beginning of vnode
 *  operations as paranoia against the possibility of our using a
 *  dangling pointer to a now-dead PFS mount...or bugs that end up
 *  decrementing the root vnode's v_usecount too far.
 */
#define ROOT_USECOUNT_CHECK(mp) do \
	{ if (VFSTOPFS(mp)->root && (VFSTOPFS(mp)->root->v_usecount < 1)) \
	   { panic("%s: root usecount %ld\n",__FUNCTION__,VFSTOPFS(mp)->root->v_usecount);\
	   } \
	} while (0)

/*
 * Centralize checking for modificatory requests.  This should be
 *  called early in vnops that should error out with EROFS if the
 *  filesystem is mounted read-only.
 */
#define ROFS_CHECK(mp) do \
	{ if ((mp)->mnt_flag & MNT_RDONLY) \
	   { printf("%s: write op, RO mount, EROFS\n",__FUNCTION__); \
	     return(EROFS); \
	   } \
	} while (0)

#endif
