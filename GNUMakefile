SRCS=example.c tsv.c tsv.h
EXE=example
FILES=README COPYING COPYING.LIB LICENSE-2.0.txt LICENSE.txt NOTICE \
test1.tsv \
$(SRCS)

PACKAGE=libtsv
VERSION=1.0.0

PV=$(PACKAGE)-$(VERSION)
TARBALL=$(PV).tar.gz

LDFLAGS=-g
CPPFLAGS=-g -I. -DHAVE_STDLIB_H -DHAVE_UNISTD_H -DHAVE_ERRNO_H

$(EXE): tsv.o example.o
tsv.c: tsv.h
example.c: tsv.h

dist: $(FILES)
	rm -rf $(PV) && \
	mkdir $(PV) && \
	cp $(FILES) $(PV) && \
	tar cfz $(TARBALL) $(PV) && \
	rm -rf $(PV)

clean:
	rm -f *.o *~ *.tar.gz $(EXE)
