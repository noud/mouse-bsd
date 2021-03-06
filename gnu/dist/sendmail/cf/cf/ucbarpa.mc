divert(-1)
#
# Copyright (c) 1998 Sendmail, Inc.  All rights reserved.
# Copyright (c) 1983 Eric P. Allman.  All rights reserved.
# Copyright (c) 1988, 1993
#	The Regents of the University of California.  All rights reserved.
#
# By using this file, you agree to the terms and conditions set
# forth in the LICENSE file which can be found at the top level of
# the sendmail distribution.
#
#

#
#  This machine has been decommissioned at Berkeley, and hence should
#  not be considered to be tested.  This file is provided as an example
#  only, of how you might set up a joint SMTP/UUCP configuration.  At
#  this point I recommend using `FEATURE(mailertable)' instead of
#  `SITECONFIG'.  See also ucbvax.mc.
#

divert(0)dnl
VERSIONID(`@(#)ucbarpa.mc	8.9 (Berkeley) 5/19/1998')
DOMAIN(CS.Berkeley.EDU)dnl
OSTYPE(bsd4.4)dnl
MAILER(local)dnl
MAILER(smtp)dnl
MAILER(uucp)dnl
SITECONFIG(uucp.ucbarpa, ucbarpa, U)
