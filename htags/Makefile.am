## Process this file with automake to create Makefile.in

bin_SCRIPTS = htags

PERL = @PERL@
VERSION = @VERSION@

SUFFIXES = .pl

.pl:
	sed -e 's,@''PERL''@,$(PERL),g' $< >$@.tmp && \
	chmod +x $@.tmp && mv $@.tmp $@

man_MANS = htags.1

EXTRA_DIST = ${man_MANS} ${bin_SCRIPTS}${SUFFIXES}

CLEANFILES = ${bin_SCRIPTS}
