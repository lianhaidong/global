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
LIB    = libgloutil.lib
OBJS   = conf.obj dbop.obj defined.obj die.obj find.obj getdbpath.obj \
	 gtagsop.obj locatestring.obj makepath.obj path.obj gpathop.obj \
	 strbuf.obj strmake.obj tab.obj test.obj token.obj usable.obj \
	 version.obj is_unixy.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
