noinst_LIBRARIES = libonigmo.a

AM_CPPFLAGS = -I$(srcdir) -I$(srcdir)/enc -I$(srcdir)/enc/unicode -I$(top_srcdir)

libonigmo_a_SOURCES = regcomp.c regenc.c regerror.c regexec.c regext.c reggnu.c \
	regparse.c regposerr.c regposix.c regsyntax.c regtrav.c regversion.c st.c \
	onigmo.h onigmognu.h onigmoposix.h regenc.h regint.h regparse.h st.h \
	enc/ascii.c enc/unicode.c enc/us_ascii.c enc/utf_8.c \
	enc/utf_16be.c enc/utf_16le.c enc/utf_32be.c enc/utf_32le.c
