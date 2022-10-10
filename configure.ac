
dnl Process this file with autoconf to produce a configure script.
AC_INIT([cooledit], [4.1.0])

dnl Identify target.
AC_CANONICAL_SYSTEM
AC_DEFINE_UNQUOTED(TARGET_MACHINE, "${target_cpu}-${target_vendor}-${target_os}", "unknown")

AC_CONFIG_SRCDIR([editor/cooledit.c])
AM_INIT_AUTOMAKE


AM_CONFIG_HEADER(config.h)

echo
echo "Now configuring $PACKAGE-$VERSION"
echo

AC_AIX

LT_INIT
AC_CONFIG_MACRO_DIRS([m4])

dnl For shared libraries
dnl AM_PROG_LIBTOOL

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_ISC_POSIX

echo $ac_n "checking for grotty... "
if which grotty 1>/dev/null 2>&1 ; then
    echo "yes" ;
else
    echo "" ;
    echo "Command 'which grotty' returned error. Aborting." ;
    echo "" ;
    echo "You need to install grotty for the man page feature to work" ;
    echo "If you don't care about this then delete the exit command below" ;
    echo "in the configure script" ;
    echo "" ;
    exit 1 ; 
fi
if test "x`uname`" = xFreeBSD ; then
  TST_MAN_CMD='for i in `man -a -w ls` ; do man $i ; echo ; echo --- ; echo ; done' ;
  THE_MAN_CMD='for i in `man -a -w %m` ; do man $i ; echo ; echo --- ; echo ; done' ;
else
  TST_MAN_CMD='for i in `man -a -w ls` ; do man -Tutf8 -Z $i | grotty ; echo ; echo --- ; echo ; done' ;
  THE_MAN_CMD='for i in `man -a -w %m` ; do man -Tutf8 -Z $i | grotty ; echo ; echo --- ; echo ; done' ;
fi ;
touch TESTTHIS ;
echo $ac_n "checking whether '$TST_MAN_CMD' works... " ;
sh -c "$TST_MAN_CMD" 1>conftestman 2>&1 ;
if egrep -q '(FreeBSD General Commands Manual|Copyright.*Free.Software.Foundation)' conftestman ; then
        MAN_CMD="$THE_MAN_CMD" ;
        echo "yes" ;
else
        MAN_CMD='man %m' ;
        echo "no, defaulting to just 'man %m'" ;
fi
rm -f conftestman
AC_DEFINE_UNQUOTED(MAN_CMD, "${MAN_CMD}","man")

dnl Do you have rxvt?
echo $ac_n "checking for rxvt... "
if rxvt -help 2>&1 | grep geom >/dev/null ; then
	XTERM_CMD="rxvt -fn %F -bg Navy -fg White " ;
	echo "yes" ;
else
	XTERM_CMD="xterm" ;
	echo "no" ;
fi
rm -f conftestrxvt
AC_DEFINE_UNQUOTED(XTERM_CMD, "${XTERM_CMD}", "xterm")

PKG_CHECK_MODULES(FREETYPE, [freetype2])

dnl Checks for C compiler flags that X needs and the X linker flags
AC_PATH_XTRA
CFLAGS="$CFLAGS $X_CFLAGS $FREETYPE_CFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS"
AC_SUBST(LDFLAGS)
AC_SUBST(FREETYPE_LIBS)



dnl python stuff

have_python_config=no
have_python_include=no

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_PROG_INSTALL
AC_CHECK_HEADERS(assert.h fcntl.h sys/fcntl.h sys/ioctl.h sys/time.h unistd.h)
AC_CHECK_HEADERS(grp.h libc.h lastlog.h  sys/byteorder.h sys/sockio.h)
AC_CHECK_HEADERS(sys/select.h sys/errno.h alloca.h grp.h pwd.h sys/wait.h)
AC_CHECK_HEADERS(string.h strings.h stdarg.h stdlib.h stdio.h termios.h select.h sys/stat.h)
AC_CHECK_HEADERS(sys/ioctl.h ioctl.h termios.h sys/termios.h sys/timeb.h types.h sys/types.h)
AC_CHECK_HEADERS(wchar.h langinfo.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_CACHE_CHECK(for mbstate_t, mbstate_t_cv_works, [
    AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif], [if ((mbstate_t *) 0)
  return 0;
if (sizeof (mbstate_t))
  return 0;], mbstate_t_cv_works=yes, mbstate_t_cv_works=no)])
if test x${mbstate_t_cv_works} = xno; then
  AC_DEFINE_UNQUOTED(mbstate_t, unsigned long, Define to 'unsigned long' if not defined in <wchar.h> or <sys/types/h>)
fi

dnl all from rxvt-2.6.1
AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(int *, 4)
dnl# ugly
if test x$cross_compiling = xyes; then
    AC_MSG_WARN(Define typedefs in src/rsizes.h manually if necessary)
fi

dnl# see usage below
AC_DEFUN([RXVT_CHECK_SIZE],
  [AC_CACHE_CHECK([for $2], $1,
    [AC_TRY_COMPILE([#include <sys/types.h>], $2 dummy;,
      [$1=yes],
      [if test $ac_cv_sizeof_char -ge $3; then
        $1="$4 char"
      else
        if test $ac_cv_sizeof_short -ge $3; then
          $1="$4 short"
        else
          if test $ac_cv_sizeof_int -ge $3; then
            $1="$4 int"
          else
            if test $ac_cv_sizeof_long -ge $3; then
              $1="$4 long"
            else
              if test $ac_cv_sizeof_long_long -ge $3; then
                $1="$4 long long"
              else
                $1="$4 long" # we _must_ have a (possibly wrong) default
              fi
            fi
          fi
        fi
      fi
      ])
    ])
  ]
  if test x"$$1" != xyes; then
    $5="typedef $$1 $2;"
  fi
  )
dnl#
dnl# Look for types the system may know about anyway.
dnl#
RXVT_CHECK_SIZE(rxvt_cv_int16_t, int16_t, 2, , rxvt_int16_typedef)
AC_SUBST(rxvt_int16_typedef)
RXVT_CHECK_SIZE(rxvt_cv_u_int16_t, u_int16_t, 2, unsigned, rxvt_u_int16_typedef)
AC_SUBST(rxvt_u_int16_typedef)
RXVT_CHECK_SIZE(rxvt_cv_int32_t, int32_t, 4, , rxvt_int32_typedef)
AC_SUBST(rxvt_int32_typedef)
RXVT_CHECK_SIZE(rxvt_cv_u_int32_t, u_int32_t, 4, unsigned, rxvt_u_int32_typedef)
AC_SUBST(rxvt_u_int32_typedef)
RXVT_CHECK_SIZE(rxvt_cv_int64_t, int64_t, 8, , rxvt_int64_typedef)
AC_SUBST(rxvt_int64_typedef)
RXVT_CHECK_SIZE(rxvt_cv_u_int64_t, u_int64_t, 8, unsigned, rxvt_u_int64_typedef)
AC_SUBST(rxvt_u_int64_typedef)
dnl#
dnl# Now look for another we use
dnl#
if test $ac_cv_sizeof_int_p -eq 8; then
  rxvt_intp_define="#define intp_t int64_t"
  rxvt_u_intp_define="#define u_intp_t u_int64_t"
else
  if test $ac_cv_sizeof_int_p -eq 4; then
    rxvt_intp_define="#define intp_t int32_t"
    rxvt_u_intp_define="#define u_intp_t u_int32_t"
  else
    if test $ac_cv_sizeof_int_p -eq 2; then
      rxvt_intp_define="#define intp_t int16_t"
      rxvt_u_intp_define="#define u_intp_t u_int16_t"
    else
      rxvt_intp_define="#error set intp_t"
      rxvt_u_intp_define="#error set u_intp_t"
    fi
  fi
fi
AC_SUBST(rxvt_intp_define)
AC_SUBST(rxvt_u_intp_define)

AC_CHECK_LIB(intl, tolower)

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
dnl
dnl autoscan wants to check for a select function. How can the piping work without select?
dnl
AC_CHECK_FUNCS(atexit unsetenv putenv setenv setutent seteuid _getpty grantpt unlockpt)
AC_CHECK_FUNCS(strerror getcwd re_comp regcomp strdup strspn strstr strcspn)
AC_CHECK_FUNCS(strchr strrchr memmove memcpy memset memchr memcmp)
AC_CHECK_FUNCS(strcasecmp strncasecmp strftime alloca tcgetattr setsid)
AC_CHECK_FUNCS(setpgrp tcsetpgrp nl_langinfo)

dnl for solaris - i commented this out because it seems autoconf now has its own routines
dnl AC_CHECK_LIB(socket, gethostbyname)
dnl AC_CHECK_LIB(nsl, gethostbyname)
dnl AC_CHECK_LIB(socket, connect)
dnl AC_CHECK_LIB(nsl, connect)

dnl mmmmmmmmmh
AC_CACHE_CHECK([for XPointer], rxvt_cv_xpointer,
[AC_TRY_COMPILE([#include <X11/Xlib.h>], [XPointer dummy;],
rxvt_cv_xpointer=yes, rxvt_cv_xpointer=no)])
if test x$rxvt_cv_xpointer = xyes; then
  AC_DEFINE(HAVE_XPOINTER,1,0)
fi


dnl all the pty stuff

AC_CACHE_CHECK(for getpt, rxvt_cv_func_getpt,
[AC_TRY_LINK([#define _GNU_SOURCE
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif],
  [(void)getpt();],
  rxvt_cv_func_getpt=yes, rxvt_cv_func_getpt=no)])
if test x${rxvt_cv_func_getpt} = xyes; then
  AC_DEFINE(HAVE_GETPT,1,0)
fi

# if we don't guess right then it's up to the user
AC_CACHE_CHECK(for tty/pty type, rxvt_cv_ptys,
[AC_TRY_RUN([#include "ptytest.c"], rxvt_cv_ptys=`cat conftestval`, rxvt_cv_ptys=UNKNOWN
AC_MSG_WARN([manually set PTYS_ARE_* by editing config.h]))])

if test x${rxvt_cv_ptys} = xSCO; then
  AC_DEFINE(PTYS_ARE_NUMERIC,1,0)
  AC_SUBST(PTYS_ARE_NUMERIC)
fi
if test x${rxvt_cv_ptys} = xUSG; then
  AC_DEFINE(PTYS_ARE_PTMX,1,0)
  AC_SUBST(PTYS_ARE_PTMX)
  AC_DEFINE(PTYS_ARE_SEARCHED,1,0)
  AC_SUBST(PTYS_ARE_SEARCHED)
fi
if test x${rxvt_cv_ptys} = xSGI3; then
  AC_DEFINE(PTYS_ARE_PTC,1,0)
  AC_SUBST(PTYS_ARE_PTC)
fi
if test x${rxvt_cv_ptys} = xSGI4; then
  AC_DEFINE(PTYS_ARE__GETPTY,1,0)
  AC_SUBST(PTYS_ARE__GETPTY)
fi
if test x${rxvt_cv_ptys} = xCONVEX; then
  AC_DEFINE(PTYS_ARE_GETPTY,1,0)
  AC_SUBST(PTYS_ARE_GETPTY)
fi
if test x${rxvt_cv_ptys} = xGLIBC; then
  AC_DEFINE(PTYS_ARE_GETPT,1,0)
  AC_SUBST(PTYS_ARE_GETPT)
  AC_DEFINE(PTYS_ARE_SEARCHED,1,0)
  AC_SUBST(PTYS_ARE_SEARCHED)
fi
if test x${rxvt_cv_ptys} = xHPUX; then
  AC_DEFINE(PTYS_ARE_CLONE,1,0)
  AC_SUBST(PTYS_ARE_CLONE)
  AC_DEFINE(PTYS_ARE_SEARCHED,1,0)
  AC_SUBST(PTYS_ARE_SEARCHED)
fi
if test x${rxvt_cv_ptys} = xBSD; then
  AC_DEFINE(PTYS_ARE_SEARCHED,1,0)
  AC_SUBST(PTYS_ARE_SEARCHED)
fi

dnl# FreeBSD needs to link libxpg4
AC_CHECK_LIB(xpg4, setlocale, [LIBS="$LIBS -lxpg4"])
dnl AC_CHECK_LIB(freetype, FT_Init_FreeType, [LIBS="$LIBS -lfreetype"])



AC_SUBST(DEBUG)
AC_SUBST(DLIB)
AC_SUBST(DINCLUDE)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(LIBS)
AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)
AC_SUBST(X_EXTRA_LIBS)





have_forkpty=no
AC_CHECK_FUNCS(forkpty, have_forkpty=yes)
for lib in bsd util; do
    got_lib=no
    if test $have_forkpty = no ; then
	AC_CHECK_LIB($lib, forkpty, [
	    if test x$got_lib = xno ; then
		LIBS="$LIBS -l$lib"
	    fi
	    have_forkpty=yes
	    got_lib=yes
	    AC_DEFINE(HAVE_FORKPTY)
	    ])
    fi
done

dnl check for `struct winsize'
AC_MSG_CHECKING(for struct winsize)
AC_TRY_COMPILE([#include "widget/pipe-headers.h"], [struct winsize w; ioctl(0, 1234, &w); ], [
ioctl_get_winsize_works=yes], [
ioctl_get_winsize_works=no
])
if test x$ioctl_get_winsize_works = xno; then
AC_DEFINE(NEED_WINSIZE,1,0)
AC_MSG_RESULT(no)
else
AC_MSG_RESULT(yes)
fi

AC_MSG_CHECKING(for --with-libs)
AC_ARG_WITH(libs, [  --with-libs='lib1 ...'  link against additional libs], [
AC_MSG_RESULT($withval)
LIBS="$withval $LIBS"
], AC_MSG_RESULT(no))

dnl internationalization macros
AC_OUTPUT([Makefile rxvt/Makefile widget/Makefile remotefs/Makefile editor/Makefile \
	pixmap/Makefile lib/Makefile syntax/Makefile man/Makefile notosans/Makefile])

echo ""
echo "Configuration done."
echo "Type 'make' to compile $PACKAGE-$VERSION"
echo ""

