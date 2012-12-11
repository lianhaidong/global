## Process this file with automake to create Makefile.in
#
# Copyright (c) 2011 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
bin_PROGRAMS = gtags-cscope

gtags_cscope_SOURCES = gtags-cscope.c alloc.c alloc.h basename.c build.c \
	build.h command.c constants.h display.c edit.c   \
	exec.c find.c global-cscope.h help.c history.c input.c library.h \
	logdir.c mouse.c mygetenv.c mypopen.c  version-cscope.h

AM_CPPFLAGS = $(CURSES_INCLUDEDIR) @AM_CPPFLAGS@

LDADD = @LDADD@

gtags_cscope_LDADD = $(LDADD) $(LIBLTDL) $(CURSES_LIBS)

gtags_cscope_DEPENDENCIES = $(LDADD) $(LTDLDEPS)

man_MANS = gtags-cscope.1

EXTRA_DIST = $(man_MANS) const.h manual.in
