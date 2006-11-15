CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)

OBJS = gcmalloc.o gcsnap.o gcio.o gcdebug.o
HDRS = gcmalloc.h
FILES = Makefile $(HDRS) internals.h \
	gcmalloc.cc gcdebug.cc gcsnap.cc gcio.cc \
	memtest.cc test.c
LIB = libgcmalloc.a
LIBS = ../cgt/libcgt.a
TARGET = /cgt

all:	$(LIB) $(HDRS) memtest tst

export:	$(LIB) $(HDRS) 
	cp $(HDRS) $(TARGET)/include
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	ranlib $(LIB)

tst:	test.o $(LIB)
	$(CC) $(CFLAGS) -o tst test.o $(LIB) $(LIBS)

memtest:	memtest.o $(LIB)
	$(CC) $(CFLAGS) -o memtest memtest.o $(LIB) $(LIBS)

gcmalloc.o:	gcmalloc.cc gcmalloc.h internals.h
gcsnap.o:	gcsnap.cc gcmalloc.h internals.h
gcio.o:	gcdebug.cc gcmalloc.h internals.h
gcdebug.o:	gcdebug.cc gcmalloc.h internals.h
test.o:		test.c gcmalloc.h
memtest.o:	memtest.cc gcmalloc.h

clean:
	rm -f *.o *.a test

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<

tar:
	tar -cf - $(FILES) | compress -b14 >gcmalloc.tar.Z
tarall:
	tar -cf - $(FILES) RCS | compress -b14 >gcmalloc.trz

files:
	@echo $(FILES)
