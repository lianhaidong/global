PROG   = gctags
EXE    = $(PROG).exe
OBJS   = java.obj gctags.obj assembler.obj C.obj Cpp.obj
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
