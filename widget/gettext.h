/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* gettext.h
   Copyright (C) 1996-2022 Paul Sheer
 */


#ifndef _LIBGETTEXT_H
#define _LIBGETTEXT_H 1

/* NLS can be disabled through the configure --disable-nls option.  */
#if ENABLE_NLS

/* Get declarations of GNU message catalog functions.  */
# include <libintl.h>

#else

/* Solaris /usr/include/locale.h includes /usr/include/libintl.h, which
   chokes if dcgettext is defined as a macro.  So include it now, to make
   later inclusions of <locale.h> a NOP.  We don't include <libintl.h>
   as well because people using "gettext.h" will not include <libintl.h>,
   and also including <libintl.h> would fail on SunOS 4, whereas <locale.h>
   is OK.  */
#if defined(__sun)
# include <locale.h>
#endif

/* Disabled NLS.
   The casts to 'const char *' serve the purpose of producing warnings
   for invalid uses of the value returned from these functions.
   On pre-ANSI systems without 'const', the config.h file is supposed to
   contain "#define const".  */
# define gettext(Msgid) ((const char *) (Msgid))
# define dgettext(Domainname, Msgid) ((const char *) (Msgid))
# define dcgettext(Domainname, Msgid, Category) ((const char *) (Msgid))
# define ngettext(Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define dngettext(Domainname, Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define dcngettext(Domainname, Msgid1, Msgid2, N, Category) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define textdomain(Domainname) do { (void) (Domainname); } while (0)
# define bindtextdomain(Domainname, Dirname) do { (void) (Dirname); } while (0)
# define bind_textdomain_codeset(Domainname, Codeset) do { (void) (Codeset); } while (0)

#endif

/* A pseudo function call that serves as a marker for the automated
   extraction of messages, but does not call gettext().  The run-time
   translation is done at a different place in the code.
   The argument, String, should be a literal string.  Concatenated strings
   and other string expressions won't work.
   The macro's expansion is not parenthesized, so that it is suitable as
   initializer for static 'char[]' or 'const char[]' variables.  */
#define gettext_noop(String) String

#endif /* _LIBGETTEXT_H */
