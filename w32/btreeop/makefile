PROG   = btreeop
EXE    = $(PROG).exe
OBJS   = btreeop.obj
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
