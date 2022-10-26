bin_PROGRAMS = remotefs

AM_CPPFLAGS = -I$(top_srcdir)/widget -I$(top_srcdir) 

remotefs_SOURCES = remotefs.c regex.c regtools.c pathdup.c ipv6scan.c aes.c sha256.c symauth.c cterminal.c
FREETYPE_LIBS = 
LIBS = 
LDFLAGS = -static 
# CFLAGS = -O0 -Wall -g3 -s
CFLAGS = -O2 -s

remotefs.c:
	ln -sf ../widget/remotefs.c .

regex.c:
	ln -sf ../widget/regex.c .

regtools.c:
	ln -sf ../widget/regtools.c .

pathdup.c:
	ln -sf ../widget/pathdup.c .

ipv6scan.c:
	ln -sf ../widget/ipv6scan.c .

aes.c:
	ln -sf ../widget/aes.c .

sha256.c:
	ln -sf ../widget/sha256.c .

symauth.c:
	ln -sf ../widget/symauth.c .

cterminal.c:
	ln -sf ../widget/cterminal.c .


remotefs_LDADD = 

DEFS = -DSTANDALONE -DNO_INSPECT




