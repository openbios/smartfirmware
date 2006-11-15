# Copyright (c) 1993,1995,1997,2000 by Parag Patel.  All Rights Reserved.

LEX = flex
CXX = gcc
CC = gcc
.SUFFIXES: .cc
.cc.o:
	$(CXX) $(CFLAGS) -c $<

# system-dependent options - use any appropriate -D<sys> macros
# -DBSD		 	for a BSD-style wait(2) which takes "union wait" (Sun)
# -Dpid_t=long		if your headers don't define a pid_t type
# -DFREE_TAKES_CHAR	if you have a free(char*) instead of free(void*) (Sun)
# -DNEED_MALLOC_H	if you have a <malloc.h> header file
COMPFLAGS = -O -g -pipe -Wall
CFLAGS = $(COMPFLAGS) -Icgt
LIBS = cgt/libcgt.a

SRCS =	README Makefile Makefile.be wacco.1 wacco.doc wacco.w \
	defs.h toks.h tgram.w tgramj.w tgramm.w tgram.good tgram.bad \
	main.cc sym.cc parse.cc scan.cc build.cc check.cc read.cc \
	gen.h gen.cc genc.h genc.cc genj.h genj.cc genm.h genm.cc \
	libw.cc yywrap.c Wacco.make calc.w cgram.w \
	MWProjects.sit.hqx \
	cgt/Makefile cgt/bitset.cc cgt/bitset.h cgt/dynarray.cc \
	cgt/dynarray.h cgt/expand.cc cgt/expand.h cgt/getoptx.cc \
	cgt/getoptx.h cgt/hashtable.cc cgt/hashtable.h cgt/pool.cc \
	cgt/pool.h cgt/stdlibx.cc cgt/stdlibx.h cgt/strhash.cc \
	cgt/strcmpi.cc cgt/stringx.h cgt/strsplit.cc cgt/xprintf.cc

OBJS =	main.o sym.o parse.o scan.o build.o check.o read.o \
	gen.o genc.o genj.o genm.o

wacco : $(OBJS) libwacco.a cgt/libcgt.a
	$(CXX) $(CFLAGS) -o wacco $(OBJS) libwacco.a $(LIBS)

export: wacco
	cp -f wacco /cgt/bin

cgt/libcgt.a:
	cd cgt; $(MAKE) $(MFLAGS) COMPFLAGS="$(COMPFLAGS)"

self: wacco.w
	-[ ! -f wparse.cc ] && cp parse.cc wparse.cc
	-[ ! -f wtoks.h ] && cp toks.h wtoks.h
	wacco-tst -h toks.h -p parse.cc wacco.w

libwacco.a : libw.o yywrap.o
	 rm -f libwacco.o
	 ar ru libwacco.a libw.o yywrap.o
	 -[ -x /usr/bin/ranlib ] && ranlib libwacco.a

test : tst
	-./tst <tgram.bad
	./tst <tgram.good

tst: tparser.o tscanner.o libwacco.a
	$(CC) $(CFLAGS) -o tst tparser.o tscanner.o libwacco.a
tparser.o: tparser.c ttokens.h

tparser.c tscanner.l: tgram.w wacco
	./wacco -h ttokens.h -p tparser.c -s tscanner.l tgram.w

ctst: cparser.o cscanner.o libwacco.a
	$(CC) $(CFLAGS) -o ctst cparser.o cscanner.o libwacco.a
tparser.o: cparser.c ctokens.h

cparser.c cscanner.l: cgram.w wacco
	./wacco -h ctokens.h -p cparser.c -s cscanner.l cgram.w

calc :	parser.o scanner.o libwacco.a
	$(CXX) $(CFLAGS) -o calc parser.o scanner.o libwacco.a

parser.cc scanner.l tokens.h: calc.w
	./wacco calc.w

parser.o: parser.cc tokens.h
scanner.o: scanner.l tokens.h

tags: $(SRCS)
	cxxtags *.h *.c* cgt/*.[hc]*

tar: $(SRCS)
	tar -zcf wacco.tgz $(SRCS)

tarall: $(SRCS)
	tar -zcf wacco.tgz $(SRCS) RCS

shar: $(SRCS)
	shar -ac -nwacco -l50 -owacco-shar $(SRCS)

clean:
	rm -f wacco *.o libwacco.a wacco.tar* wacco.shar* \
		tst ttokens.h tparser.c tscanner.l cgt/*.o cgt/libcgt.a

files:
	@echo $(SRCS)

difflist:
	@for f in $(SRCS); do rcsdiff -q $$f >/dev/null || echo $$f; done

main.o : main.cc toks.h defs.h gen.h genc.h genj.h
sym.o : sym.cc defs.h
parse.o : parse.cc toks.h defs.h
scan.o : scan.cc toks.h defs.h
build.o : build.cc defs.h
check.o : check.cc defs.h
read.o : read.cc toks.h defs.h
gen.o : gen.cc gen.h toks.h defs.h
genc.o : genc.cc genc.h gen.h toks.h defs.h
genj.o : genj.cc genj.h gen.h toks.h defs.h
genm.o : genm.cc genm.h gen.h toks.h defs.h
libw.o : libw.cc toks.h
