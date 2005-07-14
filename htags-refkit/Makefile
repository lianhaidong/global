#
# Htags Hyper-text Reference Kit.
#
# This file is placed into the public domain by the author,
# 2005 Tama Communications Corporation.
#
CFLAGS = -g -DTEST
CC = gcc
PROGRAM = htags_path2url

all: $(PROGRAM)

$(PROGRAM): $(PROGRAM).o
	$(CC) -o $(PROGRAM) $(PROGRAM).o
clean:
	rm -f $(PROGRAM) $(PROGRAM).o
