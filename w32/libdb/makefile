#
# Copyright (c) 2000 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
LIB    = libglodb.lib
OBJS=	bt_close.obj bt_conv.obj bt_debug.obj bt_delete.obj bt_get.obj \
	bt_open.obj bt_overflow.obj bt_page.obj bt_put.obj bt_search.obj \
	bt_seq.obj bt_split.obj bt_utils.obj mpool.obj db.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
