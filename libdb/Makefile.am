## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libglodb.a

noinst_HEADERS = btree.h db.h extern.h mpool.h queue.h

libglodb_a_SOURCES = \
bt_close.c bt_conv.c bt_debug.c bt_delete.c bt_get.c bt_open.c bt_overflow.c \
bt_page.c bt_put.c bt_search.c bt_seq.c bt_split.c bt_utils.c db.c mpool.c

libglodb_a_DEPENDENCIES = $(glodb_LIBADD)
