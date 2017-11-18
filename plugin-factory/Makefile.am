## Process this file with automake to create Makefile.in
#
# Copyright (c) 2010 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
plugindir = ${libdir}/gtags
plugin_LTLIBRARIES = \
		exuberant-ctags.la \
		universal-ctags.la \
		pygments-parser.la \
		user-custom.la

# Exuberant Ctags parser
exuberant_ctags_la_SOURCES = exuberant-ctags.c
exuberant_ctags_la_LDFLAGS = -module -avoid-version -no-undefined
# Univercal Ctags parser
universal_ctags_la_SOURCES = exuberant-ctags.c
universal_ctags_la_CFLAGS  = -DUSE_EXTRA_FIELDS
universal_ctags_la_LDFLAGS = -module -avoid-version -no-undefined

# Pygments parser
pygments_parser_la_SOURCES = pygments-parser.c
pygments_parser_la_LDFLAGS = -module -avoid-version -no-undefined

# skeleton
user_custom_la_SOURCES = user-custom.c
user_custom_la_LDFLAGS = -module -avoid-version -no-undefined

gtagsdir = ${datadir}/gtags
gtags_DATA = PLUGIN_HOWTO PLUGIN_HOWTO.pygments

scriptdir = ${datadir}/gtags/script
script_DATA = pygments_parser.py maps2conf.pl

EXTRA_DIST = $(gtags_DATA) ${script_DATA}

AM_CPPFLAGS = @AM_CPPFLAGS@ -DDATADIR='"$(datadir)"'

install-data-hook:
	if cd $(DESTDIR)$(datadir)/gtags/script; then \
		chmod 755 $(script_DATA); \
	fi
