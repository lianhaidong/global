## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001, 2002 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
SUBDIRS= libdb libglibc libutil gnusort gtags-parser global gozilla gtags htags doc

EXTRA_DIST = FAQ LICENSE \
	gtags.conf gtags.el nvi-1.79-gtags.diff reconf.sh globash.rc \
	convert.pl gtags.pl gtags.vim

gtagsdir = ${datadir}/gtags
gtags_DATA = AUTHORS COPYING ChangeLog FAQ INSTALL LICENSE NEWS README THANKS \
        globash.rc gtags.conf gtags.pl gtags.vim nvi-1.79-gtags.diff \
	htags/bless.sh.tmpl htags/global.cgi.tmpl

dist_lisp_LISP = gtags.el
