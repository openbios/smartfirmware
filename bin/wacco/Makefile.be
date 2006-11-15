# Copyright (c) 1993,1995 by Parag Patel.  All Rights Reserved.

LEX = flex
CXX = mwcc
CC = mwcc
LD = mwld

.SUFFIXES: .cc
.cc.o:
	$(CXX) $(CFLAGS) -c $<

# system-dependent options - use any appropriate -D<sys> macros
# -DBSD		 	for a BSD-style wait(2) which takes "union wait" (Sun)
# -Dpid_t=long		if your headers don't define a pid_t type
# -DFREE_TAKES_CHAR	if you have a free(char*) instead of free(void*) (Sun)
# -DNEED_MALLOC_H	if you have a <malloc.h> header file
#CFLAGS = -O -g -I/cgt/include
CFLAGS = -O -g -I- -I. -I../cgt -I../integer -I../float
#LIBS = -L/cgt/lib -laoa -lcgt -lc
LIBS = ../aoa/libaoa.a ../cgt/libcgt.a

SRCS =	README Makefile wacco.1 wacco.doc wacco.w \
	defs.h toks.h tgram.w tgram.good tgram.bad \
	main.cc sym.cc parse.cc scan.cc build.cc check.cc \
	read.cc gen.cc libw.cc yywrap.c Wacco.make calc.w \
	MWProjects.sit.hqx

OBJS =	main.o sym.o parse.o scan.o build.o check.o read.o gen.o

wacco : $(OBJS) libwacco.a
	$(CXX) $(CFLAGS) -o wacco $(OBJS) libwacco.a $(LIBS)

self: wacco.w
	-[ ! -f wparse.cc ] && cp parse.cc wparse.cc
	-[ ! -f wtoks.h ] && cp toks.h wtoks.h
	wacco-tst -h toks.h -p parse.cc wacco.w

libwacco.a : libw.o yywrap.o
	rm -f libwacco.o
	$(LD) -sym full -xml -o libwacco.a libw.o yywrap.o

test : tst
	-./tst <tgram.bad
	./tst <tgram.good

tst: tparser.o tscanner.o libwacco.a
	$(CC) $(CFLAGS) -o tst tparser.o tscanner.o libwacco.a
tparser.o: tparser.c ttokens.h

tparser.c tscanner.l: tgram.w wacco
	./wacco -h ttokens.h -p tparser.c -s tscanner.l tgram.w

calc :	parser.o scanner.o libwacco.a
	$(CXX) $(CFLAGS) -o calc parser.o scanner.o libwacco.a

parser.cc scanner.l tokens.h: calc.w
	./wacco calc.w

parser.o: parser.cc tokens.h
scanner.o: scanner.l tokens.h

tags: $(SRCS)
	cxxtags *.h *.cc *.c

tar: $(SRCS)
	tar -zcf wacco.tgz $(SRCS)

tarall: $(SRCS)
	tar -zcf wacco.tgz $(SRCS) RCS

shar: $(SRCS)
	shar -ac -nwacco -l50 -owacco-shar $(SRCS)

clean:
	rm -f wacco *.o libwacco.a wacco.tar* wacco.shar* \
		tst ttokens.h tparser.c tscanner.l

files:
	@echo $(SRCS)

difflist:
	@for f in $(SRCS); do rcsdiff -q $$f >/dev/null || echo $$f; done

main.o : main.cc toks.h defs.h
sym.o : sym.cc defs.h
parse.o : parse.cc toks.h defs.h
scan.o : scan.cc toks.h defs.h
build.o : build.cc defs.h
check.o : check.cc defs.h
read.o : read.cc toks.h defs.h
gen.o : gen.cc toks.h defs.h
libw.o : libw.cc toks.h
