/*	$NetBSD: _def_monetary.c,v 1.4 1997/04/29 16:40:15 kleink Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/localedef.h>
#include <limits.h>
#include <locale.h>

const _MonetaryLocale _DefaultMonetaryLocale =
{
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX,
	CHAR_MAX
};

const _MonetaryLocale *_CurrentMonetaryLocale = &_DefaultMonetaryLocale;
