## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001, 2003 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
bin_PROGRAMS= gctags

noinst_HEADERS = gctags.h c_res.h cpp_res.h java_res.h php_res.h

gctags_SOURCES = gctags.c C.c Cpp.c assembler.c java.c php.c

INCLUDES = @INCLUDES@

LDADD = @LDADD@

gctags_DEPENDENCIES = $(LDADD)

man_MANS = gctags.1

EXTRA_DIST = $(man_MANS) const.h manual.in reserved.pl HACKING \
		sharp_res.pl \
		c_res.in c_res.gpf c_res.pl \
		cpp_res.in cpp_res.gpf cpp_res.pl \
		java_res.in java_res.gpf java_res.pl \
		php_res.in php_res.gpf php_res.pl php.l

SUFFIXES = .c .h .l .gpf .in .pl

RESERVED = $(srcdir)/reserved.pl

if ENABLE_GPERF
.l.c:
	flex -o$@ $<
.in.gpf:
	prefix=`echo $* | sed 's/_res//'` && \
	perl $(RESERVED) --prefix=$$prefix $< > $@
.gpf.h:
	prefix=`echo $* | sed 's/_res//'` && \
	option=`perl $(RESERVED) --prefix=$$prefix --option` && \
	gperf $$option < $< > $@
.in.pl:
	prefix=`echo $* | sed 's/_res//'` && \
	perl $(RESERVED) --prefix=$$prefix --perl $< > $@
endif
