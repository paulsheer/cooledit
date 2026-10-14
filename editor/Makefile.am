bin_PROGRAMS = cooledit smalledit coolman
bin_SCRIPTS = coolterm coolproject

AM_CPPFLAGS = -I$(top_srcdir)/widget -I$(top_srcdir) -I$(top_srcdir)/intl
AM_LDFLAGS = @STATIC_GCC_LDFLAGS@

# cooledit/smalledit/coolman link against static .a archives only, and the
# libX11 <-> libXcursor <-> libXrender/libXfixes cycle needs a --start-group
# that libtool reorders away.  Link them with the plain C driver instead.
LINK = $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@

cooledit_SOURCES = cooledit.c find.c editoptions.c mancmd.c options.c \
	debug.c cooleditmenus.c shell.c shell.h copyright.h mail.c complete.c \
	manpage.c percentsubs.c _coolpython.c find.h postscript.c postscript.h \
	print.c rxvt.c windowmandetector.c
smalledit_SOURCES = smalledit.c copyright.h trivoptions.c
coolman_SOURCES = coolman.c manpage.c trivoptions.c

cooledit_LDADD = ../rxvt/librxvt.a ../widget/libCw.a ../onigmo/libonigmo.a @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@
smalledit_LDADD = ../widget/libCw.a ../onigmo/libonigmo.a @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@
coolman_LDADD = ../widget/libCw.a ../onigmo/libonigmo.a @FREETYPE_LIBS@ @X_LIBS@ -lX11 @X_EXTRA_LIBS@

if BUILD_STATIC_SUBST
# libstatic-c-subst.a is linked via -lstatic-c-subst (inside the --start-group),
# so automake does not see it as a dependency.  Track it explicitly so editing
# static-c-subst/dlopen.c etc. relinks the editors.
EXTRA_cooledit_DEPENDENCIES = ../static-c-subst/libstatic-c-subst.a
EXTRA_smalledit_DEPENDENCIES = ../static-c-subst/libstatic-c-subst.a
EXTRA_coolman_DEPENDENCIES = ../static-c-subst/libstatic-c-subst.a
endif

datadir = $(prefix)/share
localedir = $(datadir)/locale

DEFS = @X_CFLAGS@ -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"${exec_prefix}/share/cooledit\" @DEFS@ 
