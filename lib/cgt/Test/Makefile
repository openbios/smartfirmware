CC = gcc
COMPFLAGS = -O -g -pipe -Wall
CGT = ..
INCLUDES = -I$(CGT) -I.
CFLAGS = $(COMPFLAGS) $(INCLUDES)
LIBS = ../../fmalloc/libfmalloc.a $(CGT)/libcgt.a
 
FILES = Makefile Makefile.be test.sh \
	backtest.cc bitset.cc buftest.cc cachetst.cc \
	except.cc hashtest.cc memtest.cc pooltest.cc qtest.cc stdtest.cc \
	bitset.std buftest.std cachetst.std except.std hashtest.std \
	memtest.std pooltest.in pooltest.std qtest.in qtest.std stdtest.std
PROGS = hashtest bitset pooltest buftest qtest cachetst memtest \
	stdtest backtest

test:	$(PROGS)
	ksh test.sh
	@touch test

export:	test

hashtest:	hashtest.o $(LIBS)
	$(CC) -o hashtest $(CFLAGS) hashtest.o $(LIBS)

hashtest.o:	hashtest.cc $(CGT)/hashtable.h $(CGT)/stringx.h \
	$(CGT)/stdlibx.h

bitset:	bitset.o $(LIBS)
	$(CC) -o bitset $(CFLAGS) bitset.o $(LIBS)

bitset.o:	bitset.cc $(CGT)/bitset.h

pooltest:	pooltest.o $(LIBS)
	$(CC) -o pooltest $(CFLAGS) pooltest.o $(LIBS)

pooltest.o: pooltest.cc $(CGT)/pool.h $(CGT)/stringx.h \
	$(CGT)/stdlibx.h $(CGT)/stdiox.h

buftest:	buftest.o $(LIBS)
	$(CC) -o buftest $(CFLAGS) buftest.o $(LIBS)

buftest.o:	buftest.cc $(CGT)/dynarray.h

qtest:	qtest.o $(LIBS)
	$(CC) -o qtest $(CFLAGS) qtest.o $(LIBS)

qtest.o:	qtest.cc $(CGT)/expand.h $(CGT)/stdiox.h $(CGT)/stringx.h

cachetst:	cachetst.o $(LIBS)
	$(CC) -o cachetst $(CFLAGS) cachetst.o $(LIBS)

cachetst.o:	cachetst.cc $(CGT)/cachetable.g \
	$(CGT)/stdlibx.h $(CGT)/stringx.h

except:		except.o $(LIBS)
	$(CC) -o except $(CFLAGS) except.o $(LIBS)

except.o:	except.cc $(CGT)/exception.h $(CGT)/stdlibx.h

memtest:	memtest.o $(LIBS)
	$(CC) -o memtest $(CFLAGS) memtest.o $(LIBS)

memtest.o:	memtest.cc $(CGT)/stdlibx.h

backtest:	backtest.o $(LIBS)
	$(CC) -o backtest $(CFLAGS) backtest.o $(LIBS)

backtest.o:	backtest.cc $(CGT)/backtrace.h

stdtest:	stdtest.o $(LIBS)
	$(CC) -o stdtest $(CFLAGS) stdtest.o $(LIBS)

stdtest.o:	stdtest.cc $(CGT)/stdlibx.h

clean:
	rm -f *.o $(PROGS) *.out test zero

tar:
	tar -zcf cgttest.tgz $(FILES)
tarall:
	tar -zcf cgttest.tgz $(FILES) RCS


.SUFFIXES:	.cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<
