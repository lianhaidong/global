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
PROG   = htags
!include ..\makefile.inc

all: $(PROG)

$(PROG):
	sed -e "s/@PERL@/perl/g" $(PROG).in > $(PROG)
install:
	copy $(PROG) $(BINDIR)
	copy $(PROG).bat $(BINDIR)
	copy $(PROG).1 $(MANDIR)
clean:
	del $(PROG)
