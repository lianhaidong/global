## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001, 2003, 2004 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
bin_PROGRAMS= htags

if DJGPP
bin_SCRIPTS = htags.pl htags.bat
else
bin_SCRIPTS = htags.pl
endif

htags_SOURCES = htags.c defineindex.c dupindex.c fileindex.c src2html.c \
		anchor.c cache.c common.c incop.c assoc.c path2url.c \
		c.c cpp.c java.c php.c

noinst_HEADERS = htags.h anchor.h cache.h common.h incop.h assoc.h path2url.h \
		lexcommon.h

INCLUDES = @INCLUDES@ -I$(srcdir)

LDADD = @LDADD@

htags_DEPENDENCIES = $(LDADD)

PERL = @PERL@

body = $(srcdir)/const.pl $(top_srcdir)/gctags/htags_res.pl $(srcdir)/htags.in

htags.pl: $(body)
	echo "#! @PERL@" | cat - $(body) | \
	sed -e 's,@''PERL''@,$(PERL),g' -e 's,@''BINDIR''@,$(bindir),g' >htags.tmp && \
	chmod +x htags.tmp && mv htags.tmp htags.pl

man_MANS = htags.1

EXTRA_DIST = $(man_MANS) htags.in manual.in const.pl htags.bat \
		const.h global.cgi.tmpl.in bless.sh.tmpl \
		c.l cpp.l java.l php.l 
const.h: manual.in
	perl $(top_srcdir)/convert.pl --c $(srcdir)/manual.in >const.h

SUFFIXES = .c .l
.l.c:
	flex -o$@ $<
