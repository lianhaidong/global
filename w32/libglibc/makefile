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
LIB    = libgloglibc.lib
OBJS   = regex.obj getopt.obj getopt1.obj snprintf.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
