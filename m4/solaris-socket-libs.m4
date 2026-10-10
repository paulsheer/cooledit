# solaris-socket-libs.m4
# Copyright (C) 2026 Paul Sheer
#
# On SunOS (Solaris 10 and earlier) the socket API - socket(), bind(),
# connect(), listen(), accept(), recv(), send(), setsockopt(), ... - lives
# in libsocket, and the name-service functions (gethostbyname(),
# getservbyname(), ...) live in libnsl, rather than in libc as they do on
# Linux and the BSDs.  (Solaris 11 consolidated both into libc.)
#
# Detect a SunOS host via uname(1) and append -lsocket -lnsl to LIBS.

AC_DEFUN([COOLEDIT_SUNOS_SOCKET_LIBS],
[
    AC_MSG_CHECKING([whether to link SunOS socket libraries])
    if test "x`uname`" = xSunOS ; then
        AC_MSG_RESULT([yes])
        AC_CHECK_LIB([socket], [connect], [LIBS="$LIBS -lsocket"])
        AC_CHECK_LIB([nsl], [gethostbyname], [LIBS="$LIBS -lnsl"])
    else
        AC_MSG_RESULT([no])
    fi
])
