CC = gcc
COMPFLAGS = -O -pipe -Wall
INCLUDES = -I. -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)
LIB = libfmalloc.a
TARGET = /cgt

OBJS = fmalloc.o
HDRS =
FILES = Makefile fmalloc.cc

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	ranlib $(LIB)

export:	$(LIB)
	cp $(LIB) $(TARGET)/lib

install:	export

fmalloc.o:	fmalloc.cc

clean:
	rm -f *.o

tar:
	tar -zcf fmalloc.tgz $(FILES)

tarall:
	tar -zcf fmalloc.tgz $(FILES) RCS

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<
