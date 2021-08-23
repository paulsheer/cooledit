/* my_string.h - compatability for any system
   Copyright (C) 1996-2018 Paul Sheer
 */


#ifndef _MY_STRING_H
#define _MY_STRING_H

#include "global.h"

#ifdef MSWIN
#include <config-mswin.h>
#else
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdarg.h>

#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <locale.h>

#include "gettext.h"

#define _(String) gettext (String)

#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
# define N_(String) (String)
#endif


#define MAX_PATH_LEN 1024

/* string include, hopefully works across all unixes */

#ifndef INHIBIT_STRING_HEADER
# if defined (HAVE_STRING_H) || defined (STDC_HEADERS) || defined (_LIBC)
#  include <string.h>
# else
#  include <strings.h>
# endif
#endif

#ifndef STDC_HEADERS
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif

    size_t strnlen (const char *s, size_t count);

# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
# ifndef HAVE_MEMCMP
    int memcmp (const void *cs, const void *ct, size_t count);
# endif
# ifndef HAVE_MEMCHR
    void *memchr (const void *s, int c, size_t n);
# endif
#ifndef HAVE_MEMMOVE
    void *memmove (void *dest, const void *src, size_t n);
# endif
# ifndef HAVE_MEMSET
    void *memset (void *dest, int c, size_t n);
# endif
# ifndef HAVE_STRSPN
    size_t strspn (const char *s, const char *accept);
# endif
# ifndef HAVE_STRSTR
    char *strstr (const char *s1, const char *s2);
# endif
# ifndef HAVE_VPRINTF
    int vsprintf (char *buf, const char *fmt, va_list args);
# endif
#endif

#ifndef S_IFMT
#define	S_IFMT	0170000
#endif
#ifndef S_IFDIR
#define	S_IFDIR	0040000
#endif
#ifndef S_IFCHR
#define	S_IFCHR	0020000
#endif
#ifndef S_IFBLK
#define	S_IFBLK	0060000
#endif
#ifndef S_IFREG
#define	S_IFREG	0100000
#endif
#ifndef S_IFIFO
#define	S_IFIFO	0010000
#endif
#ifndef S_IFLNK
#define	S_IFLNK		0120000
#endif
#ifndef S_IFSOCK
#define	S_IFSOCK	0140000
#endif
#ifndef S_ISUID
#define	S_ISUID		04000
#endif
#ifndef S_ISGID
#define	S_ISGID		02000
#endif
#ifndef S_ISVTX
#define	S_ISVTX		01000
#endif
#ifndef S_IREAD
#define	S_IREAD		0400
#endif
#ifndef S_IWRITE
#define	S_IWRITE	0200
#endif
#ifndef S_IEXEC
#define	S_IEXEC		0100
#endif
#ifndef S_ISTYPE
#define	S_ISTYPE(mode, mask)	(((mode) & S_IFMT) == (mask))
#endif
#ifndef S_ISDIR
#define	S_ISDIR(mode)	S_ISTYPE((mode), S_IFDIR)
#endif
#ifndef S_ISCHR
#define	S_ISCHR(mode)	S_ISTYPE((mode), S_IFCHR)
#endif
#ifndef S_ISBLK
#define	S_ISBLK(mode)	S_ISTYPE((mode), S_IFBLK)
#endif
#ifndef S_ISREG
#define	S_ISREG(mode)	S_ISTYPE((mode), S_IFREG)
#endif
#ifndef S_ISFIFO
#define	S_ISFIFO(mode)	S_ISTYPE((mode), S_IFIFO)
#endif
#ifndef S_ISLNK
#define	S_ISLNK(mode)	S_ISTYPE((mode), S_IFLNK)
#endif
#ifndef S_ISSOCK
#define	S_ISSOCK(mode)	S_ISTYPE((mode), S_IFSOCK)
#endif
#ifndef S_IRWXU
#define	S_IRWXU	(__S_IREAD|__S_IWRITE|__S_IEXEC)
#endif

#endif				/*  _MY_STRING_H  */

