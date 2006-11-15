# For BeOS:
#CC = mwcc
#ARCMD = mwld -sym full -xml -o
#RANLIB = echo
#LFLAGS = -map $@.xMAP
#COMPFLAGS = -O -g -I-

# For Unix:
CC = gcc
ARCMD = ar r
RANLIB = ranlib
LFLAGS = -Wl,-export-dynamic
COMPFLAGS = -O -pipe -Wall

INCLUDES = -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)
LIB = libib.a
GCLIB = libibgc.a
INSTALL = /usr/local

OBJS =	expand.o except.o hashbump.o hash.o stringx.o strhash.o strpool.o \
	stdiox.o getoptx.o stkx86.o crash.o bits.o darray.o
GCOBJS = gcmalloc.o gcdebug.o gcsnap.o
GENS =	darray.g dynarray.g hash.g
HDRS =	stdlibx.h expand.h stringx.h stdiox.h getoptx.h stkback.h \
	bits.h gcmalloc.h darray.h dynarray.h hash.h except.h
GETOPTXFILES = getoptx.c getoptx.3 tstopt.c tstopt.good tstopt.sh getoptx.mk
FILES =	Makefile $(HDRS) $(GENS) gcdefs.h \
	expand.c except.c hashbump.c hash.c \
	stringx.c strhash.c strpool.c stdiox.c bsearch.c \
	$(GETOPTXFILES) stkx86.c stk386.c stk68k.c stkback.c \
	crash.c bits.c darray.c \
	gcmalloc.c gcdebug.c gcsnap.c ccgen.c tst.c tstgc.c \
	fmalloc.c wfmalloc.c tstmem.c tstxcpt.c libclib.PPC.proj.hqx

#all:	ccgen $(LIB) $(GCLIB) $(HDRS) tst tstopt tstgc tstmem tstxcpt
all:	ccgen $(LIB) $(HDRS) tst tstopt tstxcpt tststk

export: all purge
	cp $(LIB) $(INSTALL)/lib
	cp $(HDRS) $(GENS) $(INSTALL)/include

purge:
	rm -f $(INSTALL)/lib/$(LIB)
	cd $(INSTALL)/include && rm -f $(HDRS) $(GENS)

$(LIB):	$(OBJS)
	rm -f $(LIB)
	$(ARCMD) $(LIB) $(OBJS)
	$(RANLIB) $(LIB)

$(GCLIB):	$(GCOBJS)
	rm -f $(GCLIB)
	$(ARCMD) $(GCLIB) $(GCOBJS)
	$(RANLIB) $(LIB)

tstmem:	tstmem.o $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tstmem.o -o tstmem $(LIB)

fmalloc.o:	fmalloc.c stdlibx.h
wfmalloc.o:	wfmalloc.c stdlibx.h

tst:	tst.o $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tst.o -o tst $(LIB)

tst.o:	tst.c darray.h expand.h stdlibx.h

tststk: tststk.o $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tststk.o -o tststk $(LIB)

tststk.o: stkx86.c
	$(CC) $(CFLAGS) -DEBUG -c -o tststk.o stkx86.c

tstxcpt: tstxcpt.o $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tstxcpt.o -o tstxcpt $(LIB)

tstxcpt.o: tstxcpt.c except.h stdlibx.h stkback.h

tstopt:	tstopt.o $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tstopt.o -o tstopt $(LIB)
	./tstopt.sh

tstopt.o:	tstopt.c getoptx.h

tstgc:	tstgc.o $(GCLIB) $(LIB)
	$(CC) $(LFLAGS) $(CFLAGS) tstgc.o -o tstgc $(GCLIB) $(LIB)

tst.o:	darray.h expand.h stdlibx.h

ccgen:	ccgen.o
	$(CC) $(LFLAGS) $(CFLAGS) ccgen.o -o ccgen

expand.o:	expand.c expand.h stdlibx.h
except.o:	except.c except.h stdlibx.h
hashbump.o:	hashbump.c hash.h
hash.o:		hash.c hash.h
darray.o:	darray.c darray.h
bsearch.o:	bsearch.c
stringx.o:	stringx.c stringx.h expand.h stdlibx.h stdiox.h
strhash.o:	strhash.c stringx.h
strpool.o:	strpool.c stringx.h stdlibx.h
stdiox.o:	stdiox.c stdiox.h stringx.h expand.h stdlibx.h
getoptx.o:	getoptx.c getoptx.h stringx.h stdlibx.h hash.h
stk68k.o:	stk68k.c stkback.h stringx.h stdlibx.h
crash.o:	crash.c stkback.h stdlibx.h
bits.o:		bits.c bits.h stdlibx.h
gcmalloc.o:	gcmalloc.c gcmalloc.h gcdefs.h stdlibx.h bits.h stkback.h
gcdebug.o:	gcdebug.c gcmalloc.h gcdefs.h stdlibx.h
gcsnap.o:	gcsnap.c gcmalloc.h gcdefs.h stkback.h hash.h

darray.h:	darray.g ccgen
	./ccgen <darray.g >darray.h

dynarray.h:	dynarray.g ccgen
	./ccgen <dynarray.g >dynarray.h

hash.h:	hash.g ccgen
	./ccgen <hash.g >hash.h

tags: $(FILES)
	cxxtags $(FILES)

clean:
	rm -f ccgen darray.h dynarray.h hash.h *.o *.a tst tstgc tstopt tstmem tstxcpt

tar:
	tar -zcf clib.tgz $(FILES)

files:
	@echo $(FILES)

getoptxfiles:
	@echo getoptx.h $(GETOPTXFILES)
