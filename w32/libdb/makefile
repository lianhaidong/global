LIB    = libglodb.lib
OBJS=	bt_close.obj bt_conv.obj bt_debug.obj bt_delete.obj bt_get.obj \
	bt_open.obj bt_overflow.obj bt_page.obj bt_put.obj bt_search.obj \
	bt_seq.obj bt_split.obj bt_utils.obj mpool.obj db.obj
!include ..\makefile.inc

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(LIB) @objs
install:
	@echo -n
clean:
	del *.lib
	del *.obj
