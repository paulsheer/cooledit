noinst_LIBRARIES = librxvt.a

librxvt_a_SOURCES = _rxvtlib.h grkelot.c grkelot.h rxvtlib.c rxvtlib.h	\
	rxvtlib_command.c rxvtlib_graphics.c rxvtlib_logging.c	\
	rxvtlib_main.c rxvtlib_menubar.c rxvtlib_misc.c rxvtlib_netdisp.c	\
	rxvtlib_rmemset.c rxvtlib_screen.c rxvtlib_scrollbar.c	\
	rxvtlib_xdefaults.c rxvtlib_xpm.c rxvtlibproto.h rxvtlibtypedef.h 

AM_CPPFLAGS = -I$(top_srcdir)/widget -I$(top_srcdir) -I$(top_srcdir)/intl

DEFS = -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"${exec_prefix}/lib/cooledit\" @DEFS@ 

