LIB    = libgloutil.lib
OBJS   = conf.obj dbop.obj defined.obj die.obj find.obj getdbpath.obj \
	 gtagsop.obj locatestring.obj makepath.obj path.obj pathop.obj \
	 strbuf.obj strmake.obj tab.obj test.obj token.obj usable.obj \
	 getopt.obj getopt1.obj version.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
