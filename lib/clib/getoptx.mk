CC = cc
ARCMD = ar r
RANLIB = ranlib
LFLAGS =
COMPFLAGS = -O -g

INCLUDES = -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)
LIB = libib.a
GCLIB = libibgc.a

all:	tstopt

tstopt:	tstopt.o getoptx.o
	$(CC) $(LFLAGS) $(CFLAGS) tstopt.o -o tstopt $(LIB)
	./tstopt.sh

tstopt.o:	tstopt.c getoptx.h

getoptx.o:	getoptx.c getoptx.h
