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
# Definitions for UXP/DS (Fujitsu/ICL DS/90 series)
# Diego R. Lopez, CICA (Seville). 1995
#

divert(0)
VERSIONID(`@(#)uxpds.m4	8.10 (Berkeley) 10/6/1998')

define(`confDEF_GROUP_ID', `6')
define(`ALIAS_FILE', ifdef(`_USE_ETC_MAIL_', `/etc/mail/aliases', `/usr/ucblib/aliases'))dnl
ifdef(`HELP_FILE',,`define(`HELP_FILE', ifdef(`_USE_ETC_MAIL_', `/etc/mail/helpfile', `/usr/ucblib/sendmail.hf'))')dnl
ifdef(`STATUS_FILE',,`define(`STATUS_FILE', ifdef(`_USE_ETC_MAIL_', `/etc/mail/statistics', `/usr/ucblib/sendmail.st'))')dnl
define(`LOCAL_MAILER_PATH', `/usr/ucblib/binmail')dnl
define(`LOCAL_MAILER_FLAGS', `rmn9')dnl
define(`LOCAL_SHELL_FLAGS', `ehuP')dnl
define(`UUCP_MAILER_ARGS', `uux - -r -a$f -gmedium $h!rmail ($u)')dnl
define(`confEBINDIR', `/usr/ucblib')dnl
