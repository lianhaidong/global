# Definition for Borland C++ Compiler.
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
CC     = bcc32
AR     = tlib
LIBS   = ..\libutil\libgloutil.lib ..\libdb\libglodb.lib ..\libglibc\libgloglibc.lib
#WILDOBJ= \bc5\lib\32bit\wildargs.obj		# BCC++ 5.02
WILDOBJ= \Borland\BCC55\Lib\wildargs.obj	# BCC++ 5.5
CFLAGS = -DHAVE_CONFIG_H -I..\libutil -I..\libdb -I..\libglibc -I..\machine
BINDIR = \usr\bin
MANDIR = \usr\man\man1
