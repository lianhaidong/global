## Process this file with automake to create Makefile.in
#
# Copyright (c) 2010 Tama Communications Corporation
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
noinst_LIBRARIES = libgloparser.a

noinst_HEADERS = parser.h internal.h asm_parse.h

libgloparser_a_SOURCES = parser.c C.c Cpp.c asm_parse.c asm_scan.c java.c php.c

INCLUDES = @INCLUDES@

libgloparser_a_DEPENDENCIES = $(libgloparser_a_LIBADD)

EXTRA_DIST = php.l asm_parse.y asm_scan.l
