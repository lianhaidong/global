PROG   = htags
!include ..\makefile.inc

all: $(PROG)

$(PROG):
	sed -e "s/@PERL@/perl/g" $(PROG).pl > $(PROG)
install:
	copy $(PROG) $(BINDIR)
	copy $(PROG).bat $(BINDIR)
	copy $(PROG).1 $(MANDIR)
clean:
	del $(PROG)
