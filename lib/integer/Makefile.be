CC = mwcc
LD = mwld
COMPFLAGS = -O -g
INCLUDES = -I- -I. -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)

OBJS = integer.o itostr.o ibits.o itol.o
HDRS = integer.h uinteger.h
FILES = Makefile LibInteger.make integer.h uinteger.h \
	ibits.cc integer.cc itol.cc itostr.cc \
	test.cc test.std
LIB = libinteger.a
LIBS = ../cgt/libcgt.a
TARGET = /cgt

all:	$(LIB) $(HDRS) test

export:	$(LIB) $(HDRS)
	cp $(HDRS) $(TARGET)/include
	cp $(LIB) $(TARGET)/lib

$(LIB):		$(OBJS)
	rm -f $(LIB)
	$(LD) -sym full -xml -o $(LIB) $(OBJS)

test:	test.o $(LIB)
	$(CC) $(CFLAGS) -o test test.o $(LIB) $(LIBS)
	./test >test.out 2>&1
	diff test.out test.std

integer.o:	integer.cc integer.h
ibits.o:	ibits.cc integer.h
itostr.o:	itostr.cc integer.h
itol.o:		itol.cc integer.h
test.o:		test.cc integer.h uinteger.h

clean:
	rm -f *.o *.a test

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<

tar:
	tar -zcf integer.tgz $(FILES)
tarall:
	tar -zcf integer.tgz $(FILES) RCS

files:
	@echo $(FILES)
