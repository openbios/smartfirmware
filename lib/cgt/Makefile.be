CC = mwcc
LD = mwld
COMPFLAGS = -O -g
INCLUDES = -I- -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)

LIB = libcgt.a
TARGET = /cgt

OBJS =	stdlibx.o stdiox.o dynarray.o hashtable.o pool.o \
	bitset.o quickset.o xprintf.o path.o getoptx.o merge.o expand.o \
	strsplit.o strapp.o strbld.o strcmpi.o strhash.o strvec.o \
	backtrace.o meminfo.o
HDRS =	stdlibx.h stdiox.h stringx.h expand.h dynarray.h hashtable.h pool.h \
	bitset.h quickset.h pointer.h srchsort.h cachetable.h getoptx.h \
	matrix.h backtrace.h 
GENS =	expand.g dynarray.g  hashtable.g pool.g pointer.g srchsort.g \
	cachetable.g matrix.g
FILES =	Makefile Makefile.dos LibCGT.make \
	$(HDRS) $(GENS) stdlibx.cc stdiox.cc merge.cc expand.cc \
	dynarray.cc hashtable.cc pool.cc bitset.cc quickset.cc bsearch.cc \
	xprintf.cc path.cc getoptx.cc meminfo.cc \
	strsplit.cc strapp.cc strbld.cc strcmpi.cc strhash.cc strvec.cc \
	backtrace.cc back68k.cc back386.cc

all:	$(LIB) $(HDRS)

export:	$(LIB) $(HDRS)
	cp $(HDRS) $(TARGET)/include
	cp $(GENS) $(TARGET)/include
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	$(LD) -sym full -xml -o $(LIB) $(OBJS)

expand.o:	expand.cc expand.h
stdlibx.o:	stdlibx.cc stdlibx.h backtrace.h
stdiox.o:	stdiox.cc stdiox.h stdlibx.h expand.h
xprintf.o:	xprintf.cc stdlibx.h stdiox.h
path.o:		path.cc stdiox.h stdlibx.h expand.h
strsplit.o:	strsplit.cc stringx.h stdlibx.h stdiox.h expand.h
dynarray.o:	dynarray.cc dynarray.h stdlibx.h
hashtable.o:	hashtable.cc hashtable.h stdlibx.h
pool.o:		pool.cc pool.h pool.g stdlibx.h stringx.h
bitset.o:	bitset.cc bitset.h stdlibx.h
quickset.o:	quickset.cc quickset.h stdlibx.h
strapp.o:	strapp.cc stringx.h stdlibx.h
strbld.o:	strbld.cc stringx.h stdlibx.h stdiox.h expand.h
strcmpi.o:	strcmpi.cc stringx.h stdlibx.h
strhash.o:	strhash.cc stringx.h stdlibx.h
strvec.o:	strvec.cc stringx.h stdlibx.h
merge.o:	merge.cc srchsort.h
getoptx.o:	getoptx.h getoptx.cc stdlibx.h stringx.h
backtrace.o:	backtrace.cc backtrace.h stdlibx.h
back68k.o:	back68k.cc backtrace.h stdlibx.h
back386.o:	back386.cc backtrace.h stdlibx.h
meminfo.o:	meminfo.cc stdlibx.h
bsearch.o:	bsearch.cc

expand.h:	expand.g
	ccgen <expand.g >expand.h

dynarray.h:	dynarray.g
	ccgen <dynarray.g >dynarray.h

hashtable.h:	hashtable.g
	ccgen <hashtable.g >hashtable.h

pool.h:		pool.g
	ccgen <pool.g >pool.h

pointer.h:	pointer.g
	ccgen <pointer.g >pointer.h

srchsort.h:	srchsort.g
	ccgen <srchsort.g >srchsort.h

cachetable.h:	cachetable.g
	ccgen <cachetable.g >cachetable.h

matrix.h:	matrix.g
	ccgen <matrix.g >matrix.h

.SUFFIXES:	.cc .o

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a expand.h dynarray.h hashtable.h pool.h \
		pointer.h srchsort.h cachetable.h

tar:
	tar -zcf cgt.tgz $(FILES)
tarall:
	tar -zcf cgt.tgz $(FILES) RCS

files:
	@echo $(FILES)

difflist:
	@for f in $(FILES); do rcsdiff -q $$f >/dev/null || echo $$f; done
