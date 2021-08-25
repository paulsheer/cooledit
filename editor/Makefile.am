bin_PROGRAMS = cooledit smalledit coolman
bin_SCRIPTS = cooledit-gdb coolproject

AM_CPPFLAGS = -I$(top_srcdir)/widget -I$(top_srcdir) -I$(top_srcdir)/intl

cooledit_SOURCES = cooledit.c find.c editoptions.c mancmd.c options.c \
	debug.c cooleditmenus.c shell.c shell.h copyright.h mail.c complete.c \
	manpage.c percentsubs.c _coolpython.c find.h postscript.c postscript.h \
	print.c rxvt.c
smalledit_SOURCES = smalledit.c copyright.h trivoptions.c
coolman_SOURCES = coolman.c manpage.c trivoptions.c

cooledit_LDADD = ../rxvt/librxvt.a ../widget/libCw.la @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@
smalledit_LDADD = ../widget/libCw.la @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@
coolman_LDADD = ../widget/libCw.la @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@

datadir = $(prefix)/share
localedir = $(datadir)/locale

DEFS = @X_CFLAGS@ -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"${exec_prefix}/share/cooledit\" @DEFS@ 
