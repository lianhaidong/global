## Process this file with automake to create Makefile.in

noinst_LIBRARIES = libgloutil.a

noinst_HEADERS = \
conf.h dbop.h defined.h die.h find.h getdbpath.h global.h gparam.h \
gtagsop.h locatestring.h makepath.h path.h pathop.h strbuf.h \
strmake.h tab.h test.h token.h usable.h getopt.h version.h

libgloutil_a_SOURCES = \
conf.c dbop.c defined.c die.c find.c getdbpath.c gtagsop.c locatestring.c \
makepath.c path.c pathop.c strbuf.c strmake.c tab.c test.c \
token.c usable.c getopt.c getopt1.c version.c

INCLUDES = @INCLUDES@

libgloutil_a_DEPENDENCIES = $(libgloutil_a_LIBADD)
