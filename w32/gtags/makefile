PROG   = gtags
EXE    = $(PROG).exe
OBJS   = gtags.obj
!include ..\makefile.inc

all: $(EXE)

$(EXE): $(OBJS) $(LIBS)
	$(CC) -e$(EXE) $(OBJS) $(WILDOJB) @..\libs
install:
	copy $(EXE) $(BINDIR)
	copy $(PROG).1 $(MANDIR)
clean:
	del $(EXE)
	del *.obj
