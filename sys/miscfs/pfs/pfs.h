#ifndef _PFS_H_f4ebe173_
#define _PFS_H_f4ebe173_

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

#include <sys/ucred.h>
#include <sys/vnode.h>

/*
 * Throughout this file, paragraphs prefixed with "HACKERS:" are
 *  written for people contemplating hacking on the implementation and
 *  may be ignored by people who are merely writing userland code to
 *  use the interface.
 */

/*
 * pfsa_major and pfsa_minor are major and minor protocol version
 *  numbers; these are here for userland to declare what protocol it
 *  expects to speak to the kernel.  If the kernel isn't prepared to
 *  speak this version, it will refuse the mount.  pfsa_data points to
 *  a version-dependent data structure containing any other data
 *  required.
 *
 * The protocol version numbers are not advertised via #defines because
 *  that would lead to userland programs containing code depending on a
 *  particular version, but that when compiled on a system using a
 *  different version would advertise the new version while still
 *  expecting to speak the old.
 */
struct pfs_args {
  int pfsa_major;
  int pfsa_minor;
  void *pfsa_data;
  } ;

/*
 * Major version 0 is somewhat special.  When attempting to mount major
 *  version 0, the minor version is taken as something akin to an
 *  opcode, and various version-independent operations can be
 *  performed.
 *
 * Presently only one is defined.  Minor version 0 can be used to find
 *  out what version the kernel prefers.  pfsa_data is ignored in this
 *  case; the mount will fail with error EINPROGRESS, but in the
 *  process the kernel will overwrite the pfsa_major and pfsa_minor
 *  values in the pfs_args with its preferred values, which userland
 *  can inspect and take whatever action seems appropriate.
 *
 * Attempts to mount major version 0 with a minor version the kernel
 *  does not understand will fail with EINVAL without doing anything.
 */

/*
 * The rest of this description corresponds to major version 1, minor
 *  version 3.
 *
 * pfsa_data points to a struct pfs_1_3_args.  pfsa_string is an
 *  arbitrary string for mnt_mntfromname; pfsa_socket is (a file
 *  descriptor on) a socket, which must already be connected, on the
 *  other end of which is assumed to be the server process(es).  The
 *  socket must be an AF_LOCAL SOCK_DGRAM socket; it also must already
 *  be connected.  (It will probably be one of the sockets returned by
 *  a socketpair(2) call.)  The socket passed in will be taken over by
 *  the kernel - the userland file descriptor that refers to it will be
 *  closed if the mount succeeds.
 *
 * The kernel will diddle socket buffers as necessary to make sure the
 *  sockets support transferring max-size requests and replies (see
 *  PFS_RE{P,Q}_MAX, below).
 *
 * Note that userland must be prepared to handle requests occurring
 *  *during* the mount() call that mounts the filesystem; if userland
 *  does not start handling requests until after mount() returns, it
 *  will deadlock.
 */
struct pfs_1_3_args {
  const char *pfsa_string;
  int pfsa_socket;
  } ;

/*
 * The protocol run between the kernel and the userland server is
 *  fairly simple.  Communication is carried out over the socket passed
 *  in the mount call, and is philosophically RPC, with the kernel as
 *  the RPC client and userland as the RPC server.
 *
 * The description below actually allows for requests in both
 *  directions.  At present they occur in only one direction.
 */

/*
 * When a request generator wants to make a request, it does a sendmsg
 *  (or equivalent, if the kernel).  The data in the message consists
 *  of a header, as described by a struct pfs_req, followed by zero or
 *  more bytes of data; if any reply is expected for the request, the
 *  receiver must/will do a sendmsg (or equivalent), with no
 *  destination address, sending a struct pfs_rep, also possibly
 *  followed by additional data.  However, note that the kernel's
 *  sendmsg-equivalents ignore the LOCAL_CREDS option, if set, on
 *  userland's sockets.  (Ignoring LOCAL_CREDS is a misfeature and
 *  should not be depended on.)
 *
 * Userland is permitted to reply to kernel requests out-of-order (for
 *  example, multiple servers can share a single socket to the kernel);
 *  every request includes an id field, which must be copied to the id
 *  field in any response generated to it.  Note that once a request
 *  has completed, its ID is considered freed and may be re-used for a
 *  future request.  Similar remarks apply in the other direction.
 *
 * HACKERS: At this writing, the kernel never has more than one request
 *  outstanding at once.  The kernel uses sequentially increasing
 *  request IDs.  It used to use 0 always, but after a locking bug got
 *  two requests interleaved, I changed it to limit the possible damage
 *  done by a future such bug.
 *
 * Between userland and the kernel, filesystem entities are named by
 *  simple ints.  These ints serve the purposes served by inumbers in
 *  FFS or filehandles in NFS.  They must be between PFS_MIN_ID and
 *  PFS_MAX_ID, inclusive, except that there are a few special values:
 *
 *  - PFS_ROOT_ID, which is pre-reserved to refer to the filesystem's
 *    root.
 *
 *  - PFS_NO_ID, which is used when an ID field is present but the
 *    conceptual value is `no object' (see PFSOP_RENAME for an
 *    example).
 *
 *  - PFS_WHITE_ID, which is used to represent whiteouts.  (Whiteouts
 *    are entirely optional; userland is never *required* to generate
 *    PFS_WHITE_ID.)
 *
 *  These special values are not in the PFS_MIN_ID..PFS_MAX_ID range.
 *  Except for them, which are pre-assigned, these IDs are assigned by
 *  userland and can be anything (in particular, they need not be
 *  densely clustered near zero).  For reasons explained under the
 *  READDIR request, they also must fit in 32 bits, which is dealt with
 *  by making corresponding choices for PFS_M{IN,AX}_ID.  We actually
 *  require them to fit in 31 bits, as a concession to signed ints on
 *  32-bit machines; we even restrict them a little more to help catch
 *  simple erroneous values like (~0U)>>1.
 *
 * As described above, userland has no way to tell when it's safe to
 *  reuse IDs; the kernel can hang onto them for arbitrary lengths of
 *  time.  To deal with this, the kernel reports to userland with a
 *  PFSOP_FREEID call when it drops its record of an association
 *  between an ID number and anything that it used to have associated
 *  with it.  After userland returns an ID in a lookup call and before
 *  a FREEID request frees it, it must refer to the same filesystem
 *  object.  In particular, it is a protocol error for its type as
 *  returned by the first lookup request to differ from its type as
 *  returned by another lookup request (absent, of course, an
 *  intervening FREEID on the id).
 */
#define PFS_NO_ID    1
#define PFS_ROOT_ID  2
#define PFS_WHITE_ID 3
#define PFS_MIN_ID   4
#define PFS_MAX_ID 0x7fff0000

/*
 * Request header.  op is the operation to be performed (one of the
 *  PFSOP_* values from below); id is an arbitrary number chosen by the
 *  request generator which is used for matching up responses and
 *  requests (see above).  Note that there is no length field; packets
 *  with variable-length data must have a length field in the
 *  request-specific part, or make it implicit in the packet length, or
 *  some such.
 */
struct pfs_req {
  unsigned int op;
  unsigned int id;
  } ;

/*
 * Reply header.  id must be copied from the request that this is
 *  responding to.  Note there is no op field; the operation is
 *  implicit in the id field.
 */
struct pfs_rep {
  unsigned int id;
  } ;

/*
 * To permit userland to deal with buffer sizing issues, hard limits
 *  are imposed on request and response sizes.  Requests are limited to
 *  PFS_REQ_MAX bytes, including the struct pfs_req header; similarly,
 *  PFS_REP_MAX is an upper bound on the size of a reply (including the
 *  header).  These limits are the same in both directions.
 *
 * HACKERS: Note that the readdir code assumes that PFS_REP_MAX,
 *  multiplied by the number of bytes of fixed overhead of a struct
 *  dirent, won't overflow an int.
 */
#define PFS_REQ_MAX 65700
#define PFS_REP_MAX 65700

/*
 * The PFSOP_* defines give values for the op field in struct pfs_req.
 *  For the most part, these correspond 1-1 to filesystem operations;
 *  exceptions are as noted.
 *
 * Those familiar with NFS will note marked similarities.  These are
 *  not entirely coincidental; the major differences between this and a
 *  slightly mutant NFS that speaks across an AF_LOCAL socket are that
 *  this exposes more of the VOP_* interface and (necessarily) calls
 *  for more state be kept by the server.
 *
 * Most requests have request-specific _req and _rep structs which are
 *  the request and reply data for the request.  Exceptions are
 *  specifically noted.
 *
 * Some requests call for a more complicated protocol than just single
 *  request/response packets.  A request may be accompanied by a
 *  SOCK_STREAM socket as SOL_SOCKET/SCM_RIGHTS auxiliary data with the
 *  request.  If generated by the kernel, this socket will be
 *  connected; if generated by userland, the socket must already be
 *  connected.  Such sockets are used to speak a (request-specific)
 *  subprotocol.  (The kernel makes no promises about what
 *  address/protocol family the sockets it generates will belong to;
 *  userland may use any family that supports connected SOCK_STREAM
 *  sockets.)  In particular, this allows more complicated protocols
 *  without breaking the paradigm that permits multiple responders to
 *  share a single protocol socket; it also permits transferring more
 *  than PFS_RE{P,Q}_MAX bytes of data.  When a socket is sent, the
 *  receiver assumes responsibility for it.  (If an invalid (eg, not
 *  connected) socket is passed in a userland-generated request, the
 *  received copy will be closed immediately and the request will be
 *  processed as if no socket had been passed - which may generate an
 *  error or may simply change the semantics of the request.)
 *
 * Some requests are marked "name follows" on the closing brace of the
 *  request struct.  These are requests that take a name, such as
 *  LOOKUP or CREATE; what this means is that the pfs_*_req struct is
 *  just a header, with the name following it in the request, extending
 *  to the end of the request packet.
 *
 * If the mount is read-only (MNT_RDONLY), userland will never get any
 *  PFSOP_LOOKUP request other than PFS_LKUP_LOOKUP, any PFSOP_WHITEOUT
 *  request other than PFS_WHITE_TEST, nor any of the operations that
 *  involve conceptually modifying the filesystem: PFSOP_CREATE,
 *  PFSOP_LINK, PFSOP_MKDIR, PFSOP_MKNOD, PFSOP_REMOVE, PFSOP_RENAME,
 *  PFSOP_RMDIR, PFSOP_SYMLINK, PFSOP_SETATTR, PFSOP_WRITE.
 */

/*
 * Many of these requests pass a credentials structure.  The interface
 *  permits a "no credentials supplied" possibility; the way this is
 *  done within the kernel does not map trivially to anything that can
 *  be used over the socket, so credentials are passed as a pfs_cred
 *  struct.  If the kernel call has no credentials, PFS_NOCRED is set
 *  in the flags and cred is uninteresting (presently, bzero()ed);
 *  otherwise, PFS_NOCRED is clear and cred is a copy of the
 *  credentials struct.
 */
struct pfs_cred {
  unsigned int flags;
#define PFS_NOCRED 0x00000001
  struct ucred cred;
  } ;

/*
 * Used to notify userland that the mount point is going away.  No data
 *  kernel->user.  Unlike most PFSOPs, this one does not call for any
 *  response; upon receiving it, userland is expected to exit (or at
 *  least close down the communication socket).
 *
 * Note that only one PFSOP_DONE is done.  If userland runs multiple
 *  daemons, it is up to them to arrange among themselves some way to
 *  cause them all to go away when one of them gets a PFSOP_DONE.
 *
 * The kernel promises that there will be no other requests outstanding
 *  when a PFSOP_DONE is issued; there is no need for userland to check
 *  for other operations in progress.  There is no request structure,
 *  since no additional information is needed, and no reply structure,
 *  since no reply is expected.
 */
#define PFSOP_DONE 1

/*
 * statfs().  No data kernel->user; the response data must be a struct
 *  statfs, mostly filled in.  The kernel will replace the values in
 *  f_type, f_flags, f_iosize, f_fsid, f_owner, f_fstypename,
 *  f_mntonname, and f_mntfromname; they need not contain anything in
 *  particular.
 */
#define PFSOP_STATFS 2

/*
 * getattr - roughly, stat().  The kernel will replace va_fsid with the
 *  correct value in a successful reply.
 */
#define PFSOP_GETATTR 3
struct pfs_getattr_req {
  int id;		/* id of object to be examined */
  struct pfs_cred cred;	/* credentials of requesting process */
  } ;
struct pfs_getattr_rep {
  int err;		/* 0 if OK, or errno if failure */
  struct vattr attr;	/* attributes of object */
  } ;

/*
 * open.  This corresponds roughly to open(), but is also called for
 *  pseudo-opens like what happens to the executable file as part of
 *  exec().
 */
#define PFSOP_OPEN 4
struct pfs_open_req {
  int id;		/* id of object being opened */
  int mode;		/* FREAD, FWRITE, O_* */
  struct pfs_cred cred;	/* credentials of opening process */
  } ;
struct pfs_open_rep {
  int err;		/* 0 if OK, or errno if failure */
  } ;

/*
 * seek.  This corresponds roughly to lseek().
 *
 * Note that the implementations of pread() and pwrite() are commented
 *
 *	XXX This works because no file systems actually
 *	XXX take any action on the seek operation.
 *
 *  so your handler for PFSOP_SEEK probably should not actually do
 *  much, if anything, beyond error-checking (eg, "is the newoff value
 *  minimally sane?").
 */
#define PFSOP_SEEK 5
struct pfs_seek_req {
  int id;		/* id of object being seeked */
  off_t oldoff;		/* offset before seek */
  off_t newoff;		/* attempted new offset */
  struct pfs_cred cred;	/* credentials of seeking process */
  } ;
struct pfs_seek_rep {
  int err;		/* 0 if OK, or errno if failure */
  } ;

/*
 * readdir.  kernel->user consists of a struct pfs_readdir_req, as
 *  usual, but struct pfs_readdir_rep is just a header for the
 *  user->kernel reply; it consists of a struct pfs_readdir_rep
 *  followed by data.  The data differs depending on whether cookies
 *  are wanted (PFS_READDIR_Q_COOKIES is set) or not.
 *
 * If no cookies: the data is a stream of "struct dirent"s, each one
 *  occupying a number of bytes given by its own d_reclen value.
 *  Unlike the readdir() syscall, there is no requirement that a
 *  terminating NUL be present, nor that entries be padded to any
 *  particular alignment boundary.  However, d_namlen and d_reclen must
 *  be accurate; any space between the end of the d_namlen bytes making
 *  up the name and the end of the d_reclen bytes making up the entry
 *  is padding, may have any value, and will be ignored.
 *
 * If cookies: the data is a stream of alternating "struct dirent"s and
 *  "off_t"s, such as might be obtained by taking a data stream as
 *  described above for the no-cookies case and inserting an off_t
 *  after each dirent.  No padding is inserted after the off_t and
 *  before the next (if any) dirent; padding may be supplied after a
 *  dirent and before the off_t, but if so, it must be counted as part
 *  of the dirent and included in its d_reclen.  If at the end of the
 *  buffer a dirent will fit but its associated off_t will not, the
 *  entry must not be returned; returning an entry without a following
 *  off_t is not permitted by the protocol.
 *
 * In either case, the data follows the struct pfs_readdir_rep with no
 *  padding in between, regardless of what that may mean for alignment.
 *  All bytes must be accounted for by dirents (and off_ts, if
 *  applicable); there must be no leftover bytes at the end of the
 *  reply.
 *
 * Regardless of the above note about alignment and padding of dirents
 *  in the data buffer, it is the DIRENT_SIZE() value of each dirent
 *  that must be charged against the maxsize limit.  (Note that if
 *  COOKIES is set, the space occupied by the off_ts has nothing to do
 *  with maxsize.)  In any case, the maximum reply size of PFS_REP_MAX
 *  must be observed, even if it means returning fewer entries than
 *  might otherwise be possible given maxsize.  (For userland to get
 *  DIRENT_SIZE when including <dirent.h> or <sys/dirent.h>, make sure
 *  DEFINE_DIRENT_SIZE is defined before the #include.)
 *
 * off_t values for directories are opaque cookies; they are not byte
 *  offsets unless your implementation chooses to make them so.  All
 *  that the protocol depends on is that (a) offset zero is the
 *  beginning of the directory and (b) an offset returned through the
 *  cookies mechanism can be used to seek to the spot immediately after
 *  the entry it's associated with (assuming the directory hasn't been
 *  modified in the interim, of course).  Seeking to an offset not
 *  returned by a previous readdir (except for offset zero) is an error
 *  and may produce garbage results.
 *
 * Even if cookies are not requested, the value corresponding to just
 *  after the last entry must be returned as lastoff in the response.
 *  (If cookies are requested, this must equal the last cookie.)
 *  However, if count is zero, lastoff is ignored.
 *
 * The PFS_READDIR_P_EOF bit is defined to mean not "I'm returning less
 *  than you asked for because of EOF" but rather "no more entries
 *  follow the last one I'm returning".  In particular, if EOF is
 *  reached at the same time as no more entries can be returned because
 *  of maxsize or PFS_REP_MAX, PFS_READDIR_P_EOF should be set.
 *
 * count in the reply is the number of entries in the list (dirents or
 *  <dirent,off_t> pairs, depending); the size in bytes is not
 *  explicitly present - it is implicit in the size of the reply.
 *  Getting the count value wrong is a protocol error.
 *
 * The d_fileno field of each dirent is the id of the object
 *  corresponding to that directory entry.  These will not actually be
 *  passed back in future requests; those are the ones returned by
 *  LOOKUP operations.  But enough code expects them to match that we
 *  do require that d_fileno fields be in-range for ids.  (Since
 *  d_fileno is u_int32_t, this means that object ids must fit in 32
 *  bits; see the comment above, where PFS_MIN_ID and PFS_MAX_ID are
 *  defined.)
 *
 * Whiteouts, if supported, are indicated by setting the d_type field
 *  in an entry to DT_WHT.
 */
#define PFSOP_READDIR 6
struct pfs_readdir_req {
  int id;		/* id of directory being read */
  off_t off;		/* seek offset before readdir */
  struct pfs_cred cred;	/* credentials of reading process */
  int maxsize;		/* max bytes of dirents to return */
  int flags;		/* flag bits; */
#define PFS_READDIR_Q_COOKIES 0x00000001 /* want cookies */
  } ;
struct pfs_readdir_rep {
  int err;		/* 0 if OK, or errno if failure */
  int flags;		/* flag bits: */
#define PFS_READDIR_P_EOF 0x00000001 /* reached EOF */
  int count;		/* number of entries returned */
  off_t lastoff;	/* offset after last entry returned */
  } ;

/*
 * close.  This is the converse of open.  This call will practically
 *  never actually do anything; about all you can do at this point is
 *  complain about errors like failure to flush delayed-write buffers.
 *  It is too late to keep the file open.
 */
#define PFSOP_CLOSE 7
struct pfs_close_req {
  int id;		/* id of object being closed */
  int flags;		/* FREAD, FWRITE, O_* */
  struct pfs_cred cred;	/* credentials of closing process */
  } ;
struct pfs_close_rep {
  int err;		/* 0 if OK, or errno if failure */
  } ;

/*
 * lookup.  Takes a directory and a name and looks up what (if
 *  anything) it corresponds to.
 *
 * The IDs returned here are how the kernel gets IDs other than the
 *  root to make requests with.
 *
 * Because of the funky locking issues involved, . and .. are handled
 *  directly by the kernel rather than being passed to userland.  Note
 *  that userland must not permit directory links to point upward in
 *  the tree; such can cause locking deadlock.  (This is not fixable
 *  without pervasive changes to vnode locking, and unfortunately is
 *  hard for the kernel to catch violations of, except by getting
 *  unlucky enough to deadlock.)
 *
 * Note that the type of the object is also returned.  If the type is
 *  VDIR, the parenting relationship is remembered for future ..
 *  lookups.
 *
 * If err is nonzero in the reply, id and type are ignored.  Note that
 *  id in the reply cannot be PFS_ROOT_ID.  (. is handled internally by
 *  the kernel and upward links are forbidden; we forbid only upward
 *  but non-downward links in the root, because it makes this easier
 *  and is probably not too restrictive.)
 *
 * Unfortunately, the in-kernel values that would normally go in the
 *  request's op field are #ifdef _KERNEL, and because of the namespace
 *  issues involved, they probably should be.  So we define our own.
 *
 * When looking up for CREATE or RENAME, an error return produces an
 *  error; to indicate "nonexistent but no reason to error out", return
 *  error 0 with id set to PFS_NO_ID.  If id is set to PFS_NO_ID in a
 *  non-error return for LOOKUP or DELETE, the kernel will convert it
 *  to an ENOENT error, so userland can just always return PFS_NO_ID
 *  for nonexistent entities.  (Pathname components other than the last
 *  always produce PFS_LKUP_LOOKUP requests; only the last component
 *  gets any other treatment.)
 *
 * type is always ignored when id is PFS_NO_ID.
 *
 * After a successful lookup for CREATE, DELETE, or RENAME, the kernel
 *  promises that userland either will get a following request for
 *	CREATE:	PFSOP_CREATE, PFSOP_MKNOD, PFSOP_WHITEOUT, PFSOP_LINK,
 *		PFSOP_SYMLINK, PFSOP_MKDIR
 *	DELETE: PFSOP_WHITEOUT, PFSOP_REMOVE, PFSOP_RENAME,
 *		PFSOP_RMDIR
 *	RENAME: PFSOP_RENAME
 *  or will get a PFSOP_ABORT request to abort the operation.
 *
 * Rename and link operations are a little odd, in that they involve
 *  two pathnames.  A link operation looks up the existing name for
 *  LOOKUP and the new name for CREATE, then either does a LINK call or
 *  an ABORT call to abort the pending create.  A rename operation does
 *  a DELETE lookup on the old name and a RENAME lookup on the new; if
 *  the second one, whichever that is, fails, it does an ABORT on the
 *  first, whereas if they both succeed, it then either does a RENAME
 *  or it does ABORTs on both.
 *
 * A whiteout is represented as a successful lookup (err set to 0)
 *  finding id PFS_WHITE_ID.  The contents of type are ignored in this
 *  case (there is no vtype value for whiteouts).
 *
 * The op field can also have certain flags |ed into it.  PFS_LKUP_OP
 *  is a mask for the operation value; other bits:
 *
 *	REQDIR
 *		Corresponds to cn_flags bit REQUIREDIR in the kernel;
 *		indicates that the component occurs in a context where
 *		it must name a directory (such as when followed by a
 *		slash in a pathname).
 */
#define PFSOP_LOOKUP 8
struct pfs_lookup_req {
  int id;		/* id of directory */
  unsigned int op;
#define PFS_LKUP_LOOKUP 0x00000001 /* just look up */
#define PFS_LKUP_CREATE 0x00000002 /* look up in preparation for create */
#define PFS_LKUP_DELETE 0x00000003 /* look up in preparation for delete */
#define PFS_LKUP_RENAME 0x00000004 /* look up in preparation for rename */
#define PFS_LKUP_OP     0x00000007 /* mask for operation */
#define PFS_LKUP_REQDIR 0x00000008 /* component must name a directory */
  struct pfs_cred cred;	/* credentials lookup is to be done as */
  } ;			/* name follows */
struct pfs_lookup_rep {
  int err;		/* 0 if OK, or errno if failure */
  int id;		/* id of looked-up object */
  enum vtype type;	/* type of looked-up object */
  } ;

/*
 * access.  Checks an object's accessibility.  mode is VREAD, VWRITE,
 *  or VEXEC, or occasionally more than one of them (|ed together).
 *  This is used by the code behind access(2), but is also used for
 *  other similar checking done internally, such as the checking that
 *  happens during a path walk.
 */
#define PFSOP_ACCESS 9
struct pfs_access_req {
  int id;		/* id of object */
  int mode;		/* mode of putative access */
  struct pfs_cred cred;	/* credentials to check access with respect to */
  } ;
struct pfs_access_rep {
  int err;		/* 0 if permitted, or errno if not */
  } ;

/*
 * read.  Reads data from an object.  This request is accompanied by a
 *  SOCK_STREAM socket, as outlined above.
 *
 * If PFS_READ_P_SOCKET is set in the reply's flags field, the reply
 *  must contain only the struct pfs_read_rep, and the data is sent
 *  through the socket (with no framing or delimiters of any kind; the
 *  number of bytes is given by len in the reply).  The request is done
 *  when the last byte of data is transferred.  It is a protocol error
 *  if userland closes the socket before sending enough data; if
 *  userland tries to send more data, it is likely to get a broken-pipe
 *  error when the kernel shuts down its end of the connection after
 *  receiving the given number of bytes.  However, userland must be
 *  prepared to deal with broken-pipe errors in any case, as the kernel
 *  may read only part of the data - there are error conditions that
 *  may be discovered only after reading part but not all of the data.
 *  It would be possible for the kernel to read and discard the rest of
 *  the data, but it seems like unnecessary overhead.
 *
 * If PFS_READ_P_SOCKET is not set, the reply contains the data
 *  concatenated after the struct pfs_read_rep, and the auxiliary
 *  socket is unused.  (Userland still must close its file descriptor,
 *  though in this case it may do so before sending the response.)
 *
 * But, in the reply, if err is nonzero, len and flags are completely
 *  ignored, and the auxiliary socket is not used.
 *
 * To return more than PFS_MAX_REP-sizeof(struct pfs_read_rep) bytes of
 *  data, the socket must be used; for amounts below that, it may or
 *  may not be used at userland's discretion.  (But it must be used for
 *  either all the data or none of it; it is not possible to return
 *  part of the data in the reply and part via the socket.)
 *
 * It is a protocol error for len in a reply to be greater than len in
 *  the corresponding request (unless err is nonzero - vide supra).
 *
 * It is a protocol error for the length of a reply to be other than
 *  sizeof(struct pfs_read_rep), if PFS_READ_P_SOCKET is clear, or
 *  sizeof(struct pfs_read_rep)+p.len, if PFS_READ_P_SOCKET is set.
 */
#define PFSOP_READ 10
struct pfs_read_req {
  int id;		/* id of object being read from */
  int flags;		/* bitmask, IO_* flags from <sys/vnode.h> */
  struct pfs_cred cred;	/* credentials to read as */
  size_t len;		/* max # bytes to read */
  off_t off;		/* offset to read at */
  } ;
struct pfs_read_rep {
  int err;		/* 0 if OK, errno if not */
  size_t len;		/* number of bytes to transfer */
  int flags;		/* flag bits: */
#define PFS_READ_P_SOCKET 0x00000001	/* data over socket, not in reply */
  } ;

/*
 * This reports to userland that an ID is now, as far as the kernel is
 *  concerned, no longer in use.  Userland is not expected to produce a
 *  reply; this request is purely advisory.  Filesystems that allocate
 *  their IDs with no consideration for which ones the kernel may be
 *  using can totally ignore it.
 */
#define PFSOP_FREEID 11
struct pfs_freeid_req {
  int id;		/* id that is no longer in use */
  } ;

/*
 * Read the link-to string of a symbolic link.  Note that, unlike
 *  PFSOP_READ, there is no provision for a data socket; the protocol
 *  does not support link-to strings too long to fit in the reply
 *  packet.
 *
 * If err is zero, the link-to string follows immediately after the
 *  struct pfs_readlink_rep in the reply and continues to the end of
 *  the reply.
 */
#define PFSOP_READLINK 12
struct pfs_readlink_req {
  int id;		/* id of symlink object */
  struct pfs_cred cred;	/* credentials to read it as */
  } ;
struct pfs_readlink_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Create a new file.  This is for plain files only; other entities are
 *  created with other calls (MKDIR, SYMLINK, etc).
 */
#define PFSOP_CREATE 13
struct pfs_create_req {
  int id;		/* id of dir to create in */
  struct vattr attr;	/* attributes to create it with */
  struct pfs_cred cred;	/* credentials to create as */
  } ;			/* name follows */
struct pfs_create_rep {
  int err;		/* 0 if OK, errno if not */
  int id;		/* id of newly-created file */
  } ;

/*
 * Link two files (or other entities) together.  This may fail if the
 *  filesystem doesn't permit hardlinking the relevant type of object,
 *  or doesn't permit it in the relevant place, or any such problem.
 *
 * Obviously, the kernel will have already filtered out link attempts
 *  where the link-from object and the link-to directory are not on the
 *  same mount point; otherwise, the two ids here wouldn't be in the
 *  same namespace.
 *
 * Note that the kernel will also filter out calls where the `old'
 *  object is a directory, because parent-directory issues mean that
 *  hardlinks to directories break the model.
 */
#define PFSOP_LINK 14
struct pfs_link_req {
  int old;		/* id of existing object */
  int dir;		/* id of dir to link into */
  struct pfs_cred cred;	/* credentials to perform link as */
  } ;			/* name follows */
struct pfs_link_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Make a new directory.  Note that userland should ignore many of the
 *  fields in attr, such as va_type and va_size.  We could pass only
 *  the relevant fields, but the incremental cost of passing the whole
 *  thing instead is very low, and this avoids having to create
 *  idiosyncratic structs for such things.
 */
#define PFSOP_MKDIR 15
struct pfs_mkdir_req {
  int id;		/* id of dir to create in */
  struct vattr attr;	/* attributes to create it with */
  struct pfs_cred cred;	/* credentials to make it as */
  } ;			/* name follows */
struct pfs_mkdir_rep {
  int err;		/* 0 if OK, errno if not */
  int id;		/* id of newly-created directory */
  } ;

/*
 * Make a new filesystem entity for which no other call exists, which
 *  currently means device nodes and FIFOs.  The type of the entity
 *  being created is carried in attr.va_type.
 */
#define PFSOP_MKNOD 16
struct pfs_mknod_req {
  int id;		/* id of dir to create in */
  struct vattr attr;	/* attributes to create it with */
  struct pfs_cred cred;	/* credentials to make it as */
  } ;			/* name follows */
struct pfs_mknod_rep {
  int err;		/* 0 if OK, errno if not */
  int id;		/* id of newly-created object */
  } ;

/*
 * Remove something.  This is used for removing everything but
 *  directories (for which there is rmdir).  I'm not sure why both the
 *  object's id and the name are passed; it may be just providing
 *  redundancy for convenience.
 */
#define PFSOP_REMOVE 17
struct pfs_remove_req {
  int dir;		/* id of dir removal is to happen in */
  int obj;		/* id of object to be removed */
  struct pfs_cred cred;	/* credentials to remove it as */
  } ;			/* name follows */
struct pfs_remove_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Rename something.  As with PFSOP_LINK, the kernel will have filtered
 *  out renames that are not entirely within the same mount point; as
 *  with PFSOP_REMOVE, there is some redundancy present.  to_id is
 *  PFS_NO_ID if the target of the rename doesn't exist, or a real id
 *  if it does.  The kernel promises that if to_id is not PFS_NO_ID,
 *  the filesystem entities named by to_id and from_id are either both
 *  directories or both non-directories - at least according to
 *  previous returns from userland.
 *
 * This request has *two* pathname components.  They follow the
 *  pfs_rename_req, in the "name follows" style, except that because
 *  there are two of them, some way to identify the boundary between
 *  them must be present.  That is what from_len is for: it gives the
 *  length of the rename-from component.  (The rename-from component
 *  comes first, followed by the rename-to component.  The sum of the
 *  two lengths is implicit in the size of the request packet, from
 *  which the length of the rename-to component can be obtained by
 *  subtraction.  The kernel promises that from_len will always be in
 *  the range 0 to the number of bytes from the pfs_rename_req to the
 *  end of the packet.)
 */
#define PFSOP_RENAME 18
struct pfs_rename_req {
  int from_dir;		/* id of dir containing rename-from link */
  int from_id;		/* id of object being renamed */
  int from_len;		/* length of rename-from name - see above */
  int to_dir;		/* id of dir being renamed into */
  int to_id;		/* id of object being renamed over */
  struct pfs_cred cred;	/* credentials to rename as */
  } ;
struct pfs_rename_rep {
  int err;		/* 0 for OK, errno if not */
  } ;

/*
 * Remove a directory.  This is just like PFSOP_REMOVE except that it's
 *  used only for directories.
 */
#define PFSOP_RMDIR 19
struct pfs_rmdir_req {
  int dir;		/* id of dir removal is to happen in */
  int obj;		/* id of directory to be removed */
  struct pfs_cred cred;	/* credentials to remove it as */
  } ;			/* name follows */
struct pfs_rmdir_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Make a symbolic link.
 *
 * This request has *two* strings: the last pathname component of the
 *  to-be-created link and the link-to string.  They follow the
 *  pfs_rename_req, in the "name follows" style, except that because
 *  there are two strings, some way to identify the boundary between
 *  them must be present.  That is what name_len is for: it gives the
 *  length of the pathname component.  (The pathname component comes
 *  first, followed by the link-to string.  The sum of the two lengths
 *  is implicit in the size of the request packet, from which the
 *  length of the link-to string can be obtained by subtraction.  The
 *  kernel promises that name_len will always be in the range 0 to the
 *  number of bytes from the pfs_rename_req to the end of the packet.
 *  The protocol does not support link-to strings long enough to make
 *  the request overflow PFS_REQ_MAX; attempts to create such links
 *  will produce errors without calling to userland.)
 *
 * As with mkdir, userland is expected to ignore some of the fields in
 *  attr.
 */
#define PFSOP_SYMLINK 20
struct pfs_symlink_req {
  int dir;		/* id of dir link is to be created in */
  struct vattr attr;	/* attributes to create it with */
  struct pfs_cred cred;	/* credentials to make it as */
  int name_len;		/* see above */
  } ;
struct pfs_symlink_rep {
  int err;		/* 0 if OK, errno if not */
  int id;		/* id of new symlink */
  } ;

/*
 * Whiteout operations.  Whiteouts are, conceptually, explicitly
 *  nonexistent names.  The difference between a non-present name and a
 *  name that's present as a whiteout is that the former allows the
 *  lower layer to show through when union-mounted atop something else;
 *  the latter doesn't.
 *
 * Whiteouts are entirely optional; it is perfectly acceptable for a
 *  filesystem to return EOPNOTSUPP for all whiteout operations.
 *
 * The op field describes what kind of whiteout operation is to be
 *  done.  PFS_WHITE_TEST ignores the dir field; it should return OK if
 *  whiteouts are supported or an error if not.  PFS_WHITE_CREATE
 *  creates a whiteout; PFS_WHITE_DELETE delets one.  (PFS_WHITE_TEST
 *  also has no name following, or, if you prefer, always has a
 *  zero-length name.)
 *
 * Note there is no test-for-existence request.  See the PFSOP_LOOKUP
 *  and PFSOP_READDIR requests for how existing whiteouts are
 *  discovered.
 */
#define PFSOP_WHITEOUT 21
struct pfs_whiteout_req {
  int dir;		/* directory to create/delete in */
  int op;		/* what operation to perform (see above) */
#define PFS_WHITE_TEST   1
#define PFS_WHITE_CREATE 2
#define PFS_WHITE_DELETE 3
  struct pfs_cred cred;	/* credentials to do it as */
  } ;			/* name follows */
struct pfs_whiteout_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Abort an operation.  This is used after a successful non-LOOKUP
 *  lookup, to indicate that the creation, renaming, or deletion will
 *  not happen after all.
 *
 * No reply is expected.  This is purely advisory.
 */
#define PFSOP_ABORT 22
struct pfs_abort_req {
  int dir;		/* directory to create/delete in */
  } ;			/* name follows */

/*
 * Set a vnode's attributes.  This is used for operations such as chmod
 *  and chown.  Fields of attr which are VNOVAL should not be changed.
 */
#define PFSOP_SETATTR 23
struct pfs_setattr_req {
  int id;		/* object to operate on */
  struct vattr attr;	/* attribtues to set */
  struct pfs_cred cred;	/* credentials to set it as */
  } ;
struct pfs_setattr_rep {
  int err;		/* 0 if OK, errno if not */
  } ;

/*
 * Write to an object.  This request may be accompanied by a
 *  SOCK_STREAM socket, as outlined above.
 *
 * There are two protocols possible.
 *
 * One is used when no socket is passed; in this case, it is very much
 *  like other requests.  The request is a struct pfs_write1_req
 *  followed by the data to be written; the reply is a struct
 *  pfs_write_rep.
 *
 * The other one is used when a socket is passed.  This one is more
 *  unusual.  The request is a struct pfs_write2_req; the data to be
 *  written are sent through the socket.  Userland's reply is still a
 *  struct pfs_write_rep, but it is sent not through the usual
 *  mechanisms but rather through the socket.  Userland is not required
 *  to read all, or indeed any, of the data presented; it is not even
 *  required to read as many bytes as it reports as written in the
 *  reply, though it would be slightly odd for it not to.  However,
 *  userland must read all the data it's going to before it writes its
 *  reply; it also must write nothing but that reply.  It also must
 *  either read all the data offered in the request, or do a
 *  shutdown(...,SHUT_RD) on the socket, before writing its reply.  It
 *  may, of course, do both.  (This requirement is a misfeature related
 *  to the way the current kernel architecture makes it slightly
 *  difficult to wait for two events at once.  It may get fixed
 *  someday.)
 *
 * Userland must also be prepared to see either premature EOF when
 *  reading from the socket or broken-pipe errors (SIGPIPE and/or
 *  EPIPE) when writing the reply; what appropriate recovery procedures
 *  are is a policy issue.  This condition arises when a signal
 *  interrupts the write operation while it is sending data or waiting
 *  for the reply.
 *
 * If err is nonzero in the reply, the len field is ignored; except for
 *  that case, it is a protocol error for len in the reply to be
 *  greater than len in the request.
 *
 * Note that the reply, when a socket is used, is just the struct
 *  pfs_write_rep, unlike the usual case, where the reply has a struct
 *  pfs_rep header prefixed to it.  This is because the struct
 *  pfs_rep's function is to link the reply to the corresponding
 *  request, which is unnecessary in this case (sending the reply over
 *  the auxiliary socket does that).
 */
#define PFSOP_WRITE 24
struct pfs_write1_req {		/* if no socket */
  int id;		/* id of object to write to */
  int flags;		/* bitmask, IO_* flags from <sys/vnode.h> */
  struct pfs_cred cred;	/* credentials to write as */
			/* data length implicit in request packet length */
  off_t off;		/* offset to write at */
  } ;
struct pfs_write2_req {		/* if socket */
  int id;		/* id of object to write to */
  int flags;		/* bitmask, IO_* flags from <sys/vnode.h> */
  struct pfs_cred cred;	/* credentials to write as */
  size_t len;		/* # of bytes to write */
  off_t off;		/* offset to write at */
  } ;
struct pfs_write_rep {
  int err;		/* 0 if OK, errno if not */
  size_t len;		/* # of bytes actually written */
  } ;

/*
 * This reports to userland that the filesystem should be synced.
 *  Userland is not expected to produce a reply; this request is purely
 *  advisory.  Filesystems that don't cache anything in nonpermanent
 *  storage can totally ignore it.
 *
 * There is no additional data kernel->user, and as mentioned above, no
 *  reply is expected, so there are no pfs_sync_re* structs.
 */
#define PFSOP_SYNC 25

/*
ioctl
fcntl
poll
revoke
mmap
fsync
abortop
bmap
print
pathconf
advlock
blkatoff
valloc
balloc
reallocblks
vfree
truncate
update
*/

#endif
