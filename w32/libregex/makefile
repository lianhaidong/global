LIB    = libgloregex.lib
OBJS   = regex.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
