#
# Copyright (c) 2004 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
bin_PROGRAMS= gnusort

noinst_HEADERS = closeout.h error.h hard-locale.h long-options.h \
                memcoll.h sys2.h system.h version-etc.h xalloc.h

gnusort_SOURCES = closeout.c error.c hard-locale.c long-options.c \
                memcoll.c sort.c version-etc.c xmalloc.c

INCLUDES = @INCLUDES@

LDADD = @LDADD@

gnusort_DEPENDENCIES = $(LDADD)
