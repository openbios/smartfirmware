CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)

OBJS = wfmalloc.o
HDRS = 
FILES = Makefile $(HDRS) wfmalloc.cc memtest.cc
LIB = libwfmalloc.a
LIBS = ../cgt/libcgt.a
TARGET = /cgt

all:	$(LIB) $(HDRS) memtest

export:	$(LIB) $(HDRS) 
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)

memtest:	memtest.o $(LIB)
	$(CC) $(CFLAGS) -o memtest memtest.o $(LIB) $(LIBS)

wfmalloc.o:	wfmalloc.cc
gcdebug.o:	gcdebug.cc
test.o:		test.c
memtest.o:	memtest.cc

clean:
	rm -f *.o *.a test

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<

tar:
	tar -cf - $(FILES) | compress -b14 >wfmalloc.tar.Z
tarall:
	tar -cf - $(FILES) RCS | compress -b14 >wfmalloc.trz

files:
	@echo $(FILES)
