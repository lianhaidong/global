## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000, 2001
#        Tama Communications Corporation. All rights reserved.
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

noinst_HEADERS = C.h Cpp.h gctags.h java.h

gctags_SOURCES = C.c Cpp.c assembler.c gctags.c java.c

INCLUDES = @INCLUDES@

LDADD = @LDADD@

gctags_DEPENDENCIES = $(LDADD)

man_MANS = gctags.1

EXTRA_DIST = $(man_MANS) const.h manual.in
