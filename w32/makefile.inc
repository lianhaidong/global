#
# Definition for Borland C++ Compiler.
#
CC     = bcc32
AR     = tlib
LIBS   = ..\libutil\libgloutil.lib ..\libdb\libglodb.lib ..\libregex\libgloregex.lib
#WILDOBJ= \bc5\lib\32bit\wildargs.obj		# BCC++ 5.02
WILDOBJ= \Borland\BCC55\Lib\wildargs.obj	# BCC++ 5.5
CFLAGS = -DHAVE_CONFIG_H -I..\libutil -I..\libdb -I..\libregex -I..\machine
BINDIR = \usr\bin
MANDIR = \usr\man\man1
