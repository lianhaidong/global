## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
if DJGPP
bin_SCRIPTS = htags htags.bat
else
bin_SCRIPTS = htags
endif

PERL = @PERL@

resdir = $(srcdir)/../gctags

RESFILES = $(resdir)/c_res.pl $(resdir)/cpp_res.pl $(resdir)/java_res.pl \
	$(resdir)/php_res.pl

htags: $(srcdir)/htags.in $(srcdir)/const.pl $(RESFILES)
	echo "#! @PERL@" | \
	cat - $(srcdir)/const.pl $(RESFILES) $(srcdir)/htags.in | \
	sed -e 's,@''PERL''@,$(PERL),g' >htags.tmp && \
	chmod +x htags.tmp && mv htags.tmp htags

man_MANS = htags.1

EXTRA_DIST = $(man_MANS) htags.in manual.in const.pl htags.bat

CLEANFILES = htags
