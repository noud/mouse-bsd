/*	$NetBSD: ealloc.h,v 1.1 1999/11/23 05:28:20 mrg Exp $	*/

void	*emalloc __P((size_t len));
char	*estrdup __P((const char *str));
void	*erealloc __P((void *ptr, size_t size));
void	*ecalloc __P((size_t nmemb, size_t size));
