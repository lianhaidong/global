## Process this file with automake to create Makefile.in

bin_PROGRAMS= gctags

noinst_HEADERS = C.h Cpp.h gctags.h java.h

gctags_SOURCES = C.c Cpp.c assembler.c gctags.c java.c

INCLUDES = @INCLUDES@

LDADD = @LDADD@

gctags_DEPENDENCIES = $(LDADD)

man_MANS = gctags.1

EXTRA_DIST = ${man_MANS}
