CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)

LIB = libcgt.a

OBJS =	stdlibx.o dynarray.o hashtable.o \
	xprintf.o expand.o strhash.o strbld.o
HDRS =	stdlibx.h stringx.h expand.h dynarray.h hashtable.h
FILES =	Makefile Makefile.dos Makefile.be LibCGT.make \
	$(HDRS) stdlibx.cc expand.cc \
	dynarray.cc hashtable.cc bsearch.cc \
	xprintf.cc \
	strsplit.cc strhash.cc strbld.cc

all:	$(LIB) $(HDRS)

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	-[ -x /usr/bin/ranlib ] && ranlib $(LIB)

expand.o:	expand.cc expand.h
stdlibx.o:	stdlibx.cc stdlibx.h
xprintf.o:	xprintf.cc stdlibx.h
strsplit.o:	strsplit.cc stringx.h stdlibx.h expand.h
strbld.o:	strbld.cc stringx.h stdlibx.h expand.h
dynarray.o:	dynarray.cc dynarray.h stdlibx.h
hashtable.o:	hashtable.cc hashtable.h stdlibx.h
strhash.o:	strhash.cc stringx.h stdlibx.h

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a
