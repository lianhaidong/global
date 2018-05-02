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
noinst_LIBRARIES = libglodb.a

INCS = btree.h db.h extern.h mpool.h queue.h compat.h
SRCS = bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c bt_overflow.c \
        bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c bt_utils.c db.c mpool.c

if USE_SQLITE3
INCS += sqlite3.h
SRCS += sqlite3.c
AM_CPPFLAGS += -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION
endif

noinst_HEADERS = $(INCS)
libglodb_a_SOURCES = $(SRCS)
libglodb_a_DEPENDENCIES = $(libglodb_a_LIBADD)
