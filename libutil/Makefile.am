## Process this file with automake to create Makefile.in
#
# Copyright (c) 2000
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
noinst_LIBRARIES = libgloutil.a

noinst_HEADERS = \
conf.h dbop.h defined.h die.h find.h getdbpath.h global.h gparam.h \
gtagsop.h locatestring.h makepath.h path.h gpathop.h strbuf.h \
strmake.h tab.h test.h token.h usable.h version.h

libgloutil_a_SOURCES = \
conf.c dbop.c defined.c die.c find.c getdbpath.c gtagsop.c locatestring.c \
makepath.c path.c gpathop.c strbuf.c strmake.c tab.c test.c \
token.c usable.c version.c

INCLUDES = @INCLUDES@

libgloutil_a_DEPENDENCIES = $(libgloutil_a_LIBADD)
