## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001, 2003, 2004, 2010 Tama Communications Corporation
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

htags_SOURCES = htags.c defineindex.c dupindex.c fileindex.c cflowindex.c src2html.c \
		anchor.c cache.c common.c incop.c path2url.c \
		c.c cpp.c java.c php.c asm.c

noinst_HEADERS = htags.h anchor.h cache.h common.h incop.h path2url.h lexcommon.h

AM_CPPFLAGS = @AM_CPPFLAGS@ -I$(srcdir)

LDADD = @LDADD@

htags_DEPENDENCIES = $(LDADD)

man_MANS = htags.1

gtagsdir = ${datadir}/gtags
gtags_DATA = global.cgi completion.cgi dot_htaccess \
	jscode_suggest jscode_treeview style.css
EXTRA_DIST = $(man_MANS) manual.in global.cgi.in completion.cgi.in dot_htaccess \
		jscode_suggest jscode_treeview style.css \
		const.h c.l cpp.l java.l php.l asm.l
CLEANFILES = global.cgi completion.cgi
# CGI scripts
convert=sed -e 's!@PERLPATH@!$(PERL)!g' -e 's!@GLOBALPATH@!$(bindir)/global!g' -e 's!@GENERATOR@!$(PACKAGE_STRING)!g'
global.cgi: $(srcdir)/global.cgi.in Makefile
	$(convert) $(srcdir)/global.cgi.in > global.cgi
completion.cgi: $(srcdir)/completion.cgi.in Makefile
	$(convert) $(srcdir)/completion.cgi.in > completion.cgi
