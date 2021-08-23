lib_LTLIBRARIES = libCw.la

AM_CPPFLAGS = -I. -I$(top_srcdir) -I$(top_srcdir)/intl

libCw_la_SOURCES = initapp.c coolwidget.c dirtools.c menu.c mathfuncs.c \
	filebrowser.c coolnext.c bitmapbutton.c pixmap.c \
	dialog.c disable.c focus.c xdnd.c ClientWin.c textwidget.c \
	hintpos.c inputwidget.c mousemark.c mousemark.h \
	cmdlineopt.c cmdlineopt.h scrollbar.c fieldedtextbox.c \
	edit.c editwidget.c editdraw.c editcmd.c editmenu.c \
	propfont.c international.c edit_key_translator.c \
	loadfile.c stringtools.c pool.c pool.h compatable.c \
	mad.c regex.c coollocal.h coolwidget.h drawings.h \
	imagewidget.h filelist.c compatable.h \
	loadfile.h loadtiff.h mad.h my_string.h regex.h \
	stringtools.h pipe.c edit.h editcmddef.h \
	triangle.h vgagl.h config.h wordproc.c syntax.c \
	vertchar.c cursor.c bookmark.c font.c font.h xim.c \
	pathdup.c widechar.c unicodedialog.c aafont.c aafont.h \
	look-cool.c look-gtk.c look-next.c redblack.c redblack.h \
	regtools.c remotefs.c ipv6scan.c

libCw_la_LDFLAGS = -version-info 1:0:0

DEFS = @X_CFLAGS@ -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"${exec_prefix}/share/cooledit\" @DEFS@ 

