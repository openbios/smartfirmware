
CC = gcc
CCC = gcc
# optional COMPFLAGS: -D_DJGPP_SOURCE -DGETTIMEOFDAY
#COMPFLAGS = -O -DMTSAFE
COMPFLAGS = -O -fpic -DBLOCKKERN
#COMPFLAGS = -g -DBLOCKKERN -DDEBUG
INCLUDES = -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)
CCFLAGS = $(CFLAGS)
CXXFLAGS = $(CCFLAGS)
#RUNLOGOBJS = runlog.o stopwch.o stopwcha.o
RUNLOGOBJS = runlogc.o 
RUNLOGMTOBJS = runlogmt.o 

OBJS = aoa.o
HDRS =
FILES = Makefile $(HDRS) aoa.c memtest.cc memt.c memt2.c memt3.c memt4.c
LIB = libaoa.a
LIBS =
#LIBS = -lthread
LIBS = ../cgt/libcgt.a
TARGET = /cgt

#all:	$(LIB) $(HDRS) memtest memt memt2 memt3 memt4 \
#	munge hist runloga runlogb runlogf runlogi runlogw

all:	$(LIB) $(HDRS) memtest memt memt2 \
	munge hist runloga runlogb runlogi libs sharedlibs

libs:	libsmartalloc.a libsmartalloc_mt.a libsmartalloc_mti.a libsmartalloc_i.a

libsmartalloc.a:	smartalloc.o
	rm -f libsmartalloc.a
	ar r libsmartalloc.a smartalloc.o

libsmartalloc_i.a:	smartalloc_i.o
	rm -f libsmartalloc_i.a
	ar r libsmartalloc_i.a smartalloc_i.o

libsmartalloc_mt.a:	smartalloc_mt.o
	rm -f libsmartalloc_mt.a
	ar r libsmartalloc_mt.a smartalloc_mt.o

libsmartalloc_mti.a:	smartalloc_mti.o
	rm -f libsmartalloc_mti.a
	ar r libsmartalloc_mti.a smartalloc_mti.o

sharedlibs:	libsmartalloc.so.1 libsmartalloc_mt.so.1 \
	libsmartalloc_mti.so.1 libsmartalloc_i.so.1

libsmartalloc.so.1:	smartalloc.o
	$(CC) -G -o libsmartalloc.so.1	smartalloc.o

libsmartalloc_i.so.1:	smartalloc_i.o
	$(CC) -G -o libsmartalloc_i.so.1	smartalloc_i.o

libsmartalloc_mt.so.1:	smartalloc_mt.o
	$(CC) -G -o libsmartalloc_mt.so.1	smartalloc_mt.o

libsmartalloc_mti.so.1:	smartalloc_mti.o
	$(CC) -G -o libsmartalloc_mti.so.1	smartalloc_mti.o

smartalloc.o:	smartalloc.c
	$(CC) $(CFLAGS) -c smartalloc.c

smartalloc_i.o:	smartalloc_i.c
	$(CC) $(CFLAGS) -DINSTRUMENT -c smartalloc_i.c

smartalloc_mt.o:	smartalloc_mt.c
	$(CC) $(CFLAGS) -DMTSAFE -c smartalloc_mt.c

smartalloc_mti.o:	smartalloc_mti.c
	$(CC) $(CFLAGS) -DMTSAFE -DINSTRUMENT -c smartalloc_mti.c

smartalloc.c:	aoa.c
	rm -f smartalloc.c
	cp aoa.c smartalloc.c

smartalloc_i.c:	aoa.c
	rm -f smartalloc_i.c
	cp aoa.c smartalloc_i.c

smartalloc_mt.c:	aoa.c
	rm -f smartalloc_mt.c
	cp aoa.c smartalloc_mt.c

smartalloc_mti.c:	aoa.c
	rm -f smartalloc_mti.c
	cp aoa.c smartalloc_mti.c

export:	$(LIB) $(HDRS) 
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	ar r $(LIB) $(OBJS)
	if [ -x /usr/bin/ranlib ]; then ranlib $(LIB); else ar rs $(LIB); fi

memtest:	memtest.o $(LIB)
	$(CC) $(CFLAGS) -o memtest memtest.o $(LIB) $(LIBS)

memt:	memt.o $(LIB)
	$(CC) $(CFLAGS) -o memt memt.o $(LIB) $(LIBS)

memt2:	memt2.o $(LIB)
	$(CC) $(CFLAGS) -o memt2 memt2.o $(LIB) $(LIBS)

memt3:	memt3.o $(LIB)
	$(CC) $(CFLAGS) -o memt3 memt3.o $(LIB) $(LIBS)

memt4:	memt4.o $(LIB)
	$(CC) $(CFLAGS) -o memt4 memt4.o $(LIB) $(LIBS)

munge:	munge.o
	$(CC) $(CFLAGS) -o munge munge.o $(LIBS)

hist:	hist.o
	$(CC) $(CFLAGS) -o hist hist.o $(LIBS)

runloga:	$(RUNLOGOBJS) $(LIB)
	$(CC) $(CFLAGS) -o runloga $(RUNLOGOBJS) $(LIB) $(LIBS)
#	$(CC) $(CFLAGS) -o runloga $(RUNLOGOBJS) libsa.so $(LIBS)

runlogb:	$(RUNLOGOBJS)
	$(CC) $(CFLAGS) -o runlogb $(RUNLOGOBJS) -lbsdmalloc

runlogf:	$(RUNLOGOBJS)
	$(CC) $(CFLAGS) -o runlog

runlogi:	$(RUNLOGOBJS) smartalloc_i.o
	$(CC) $(CFLAGS) -o runlogi $(RUNLOGOBJS) smartalloc_i.o $(LIBS)
