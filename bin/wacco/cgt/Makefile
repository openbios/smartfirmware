CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)

LIB = libcgt.a

OBJS =	stdlibx.o dynarray.o hashtable.o pool.o \
	bitset.o xprintf.o getoptx.o expand.o \
	strsplit.o strhash.o strcmpi.o
HDRS =	stdlibx.h stringx.h expand.h dynarray.h hashtable.h pool.h \
	bitset.h getoptx.h
FILES =	Makefile Makefile.dos Makefile.be LibCGT.make \
	$(HDRS) stdlibx.cc expand.cc \
	dynarray.cc hashtable.cc pool.cc bitset.cc bsearch.cc \
	xprintf.cc getoptx.cc \
	strsplit.cc strhash.cc strcmpi.cc

all:	$(LIB) $(HDRS)

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	-[ -x /usr/bin/ranlib ] && ranlib $(LIB)

expand.o:	expand.cc expand.h
stdlibx.o:	stdlibx.cc stdlibx.h
xprintf.o:	xprintf.cc stdlibx.h
strsplit.o:	strsplit.cc stringx.h stdlibx.h expand.h
dynarray.o:	dynarray.cc dynarray.h stdlibx.h
hashtable.o:	hashtable.cc hashtable.h stdlibx.h
pool.o:		pool.cc pool.h stdlibx.h stringx.h
bitset.o:	bitset.cc bitset.h stdlibx.h
strhash.o:	strhash.cc stringx.h stdlibx.h
strcmpi.o:	strcmpi.cc stringx.h stdlibx.h
getoptx.o:	getoptx.h getoptx.cc stdlibx.h stringx.h

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a
