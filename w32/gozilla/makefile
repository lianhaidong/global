#
# Copyright (c) 2002 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
PROG   = gozilla
EXE    = $(PROG).exe
OBJS   = gozilla.obj
!include ..\makefile.inc

all: $(EXE)

$(EXE): $(OBJS) $(LIBS)
	$(CC) -e$(EXE) $(OBJS) $(WILDOBJ) @..\libs
install:
	copy $(EXE) $(BINDIR)
	copy $(PROG).1 $(MANDIR)
clean:
	del $(EXE)
	del $(OBJS)
