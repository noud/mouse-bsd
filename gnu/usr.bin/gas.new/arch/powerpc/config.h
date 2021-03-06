/* config.h.  Generated automatically by configure.  */
/* config.in.  Generated automatically from configure.in by autoheader.  */

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if lex declares yytext as a char * by default, not a char[].  */
#define YYTEXT_POINTER 1

/* Name of package.  */
#define PACKAGE "gas"

/* Version of package.  */
#define VERSION "2.9.1"

/* Should gas use high-level BFD interfaces?  */
#define BFD_ASSEMBLER 1

/* Some assert/preprocessor combinations are incapable of handling
   certain kinds of constructs in the argument of assert.  For example,
   quoted strings (if requoting isn't done right) or newlines.  */
/* #undef BROKEN_ASSERT */

/* If we aren't doing cross-assembling, some operations can be optimized,
   since byte orders and value sizes don't need to be adjusted.  */
/* #undef CROSS_COMPILE */

/* Some gas code wants to know these parameters.  */
#define TARGET_ALIAS "powerpc--netbsd"
#define TARGET_CPU "powerpc"
#define TARGET_CANONICAL "powerpc--netbsd"
#define TARGET_OS "netbsd"
#define TARGET_VENDOR ""

/* Sometimes the system header files don't declare strstr.  */
/* #undef NEED_DECLARATION_STRSTR */

/* Sometimes the system header files don't declare malloc and realloc.  */
/* #undef NEED_DECLARATION_MALLOC */

/* Sometimes the system header files don't declare free.  */
/* #undef NEED_DECLARATION_FREE */

/* Sometimes the system header files don't declare sbrk.  */
/* #undef NEED_DECLARATION_SBRK */

/* Sometimes errno.h doesn't declare errno itself.  */
/* #undef NEED_DECLARATION_ERRNO */

/* #undef MANY_SEGMENTS */

/* The configure script defines this for some targets based on the
   target name used.  It is not always defined.  */
#define TARGET_BYTES_BIG_ENDIAN 1

/* Needed only for some configurations that can produce multiple output
   formats.  */
#define DEFAULT_EMULATION ""
#define EMULATIONS
/* #undef USE_EMULATIONS */
/* #undef OBJ_MAYBE_AOUT */
/* #undef OBJ_MAYBE_BOUT */
/* #undef OBJ_MAYBE_COFF */
/* #undef OBJ_MAYBE_ECOFF */
/* #undef OBJ_MAYBE_ELF */
/* #undef OBJ_MAYBE_GENERIC */
/* #undef OBJ_MAYBE_HP300 */
/* #undef OBJ_MAYBE_IEEE */
/* #undef OBJ_MAYBE_SOM */
/* #undef OBJ_MAYBE_VMS */

/* Used for some of the COFF configurations, when the COFF code needs
   to select something based on the CPU type before it knows it... */
/* #undef I386COFF */
/* #undef M68KCOFF */
/* #undef M88KCOFF */

/* Using cgen code?  */
/* #undef USING_CGEN */

/* Needed only for sparc configuration.  */
/* #undef DEFAULT_ARCH */

/* Needed only for PowerPC Solaris.  */
/* #undef TARGET_SOLARIS_COMMENT */

/* Needed only for SCO 5.  */
/* #undef SCO_ELF */

/* Define if you have the remove function.  */
/* #undef HAVE_REMOVE */

/* Define if you have the sbrk function.  */
#define HAVE_SBRK 1

/* Define if you have the unlink function.  */
#define HAVE_UNLINK 1

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <varargs.h> header file.  */
#define HAVE_VARARGS_H 1
