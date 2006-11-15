CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)

OBJS = wgmalloc.o
HDRS =
FILES = Makefile $(HDRS) wgmalloc.c testmem.c
LIB = libwgmalloc.a
LIBS = ../cgt/libcgt.a
TARGET = /cgt

all:	$(LIB) testmem
#all:	$(LIB) $(HDRS) testmem testmem2

export:	$(LIB) $(HDRS) 
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	ranlib $(LIB)

testmem:	testmem.o $(OBJS)
	$(CC) $(CFLAGS) -o testmem testmem.o $(OBJS) $(LIBS)

testmem2:	testmem2.o $(OBJS)
	$(CC) $(CFLAGS) -o testmem2 testmem2.o $(OBJS) $(LIBS)

wgmalloc.o:	wgmalloc.c ../cgt/stdlibx.h
testmem.o:	testmem.c
testmem2.o:	testmem2.c

clean:
	rm -f *.o *.a testmem

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<

tar:
	tar -zcf wgmalloc.tgz $(FILES)
tarall:
	tar -zcf wgmalloc.tgz $(FILES) RCS

files:
	@echo $(FILES)
