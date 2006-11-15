# Copyright (c) 1992 by Parag Patel.  All Rights Reserved.
# $Header: /u/cgt/cvs/bin/cc/Makefile,v 1.75 1999/09/26 18:51:18 parag Exp $

CC = gcc
YACC = yacc
CFLAGS = -O -g -pipe -Wall -I/cgt/include
yFLAGS = -dv

FILES = Makefile Makefile.be CCtst.make FEATURES \
	cppdefs.h cpp.h globals.h error.h \
	cc.h ccsym.h cctype.h ccnode.h cctree.h ccsetup.h fileuse.h \
	globals.cc cppmain.cc cppscan.cc cppimpl.cc cppif.cc \
	cppmac.cc cppdir.cc error.cc cctst.cc ccgram.w \
	testcpp.h testcpp.good dummy.cc fileuse.cc \
	ccmain.cc cckey.cc ccsym.cc cctype.cc cctype2.cc \
	ccrules.cc ccstor.cc ccimpl.cc ccdebug.cc \
	ccsetup.cc ccnode.cc ccnode2.cc ccnode3.cc \
	cceval.cc ccinit.cc cctree.cc cctree2.cc \
	ccfcode.cc fcode.cc README-Fcode \
	mac.init macxx.init \
	bsdi.init bsdixx.init \
	linux.init linuxxx.init \
	freebsd.init freebsdxx.init \
	be.init bexx.init \
	MWProjects.hqx
CPPOBJS = cppscan.o cppimpl.o cppdir.o cppif.o cppmac.o \
	fileuse.o error.o globals.o
#COBJS = ytab.o
COBJS = parser.o cckey.o ccsym.o cctype.o cctype2.o ccrules.o ccstor.o \
	ccimpl.o ccnode.o ccnode2.o ccnode3.o ccsetup.o \
	cceval.o ccinit.o ccdebug.o dummy.o
FCODE = fcode.o ccfcode.o
CTREE = cctree.o cctree2.o ccmain.o
M68K = /cgt/lib/gen68k.o
HC11 = /cgt/lib/genhc11.o
LIBS = -L/cgt/lib -lfloat -linteger -laoa -lcgt -lm -lc
TLIBS = -L/cgt/lib -lcg -lir -ltree -lutil \
	-lfloat -linteger -laoa -lcgt -lm -lc

#all: cctst cc-fcode cc-m68k cc-hc11 cpptst test tags
all: cctst cc-fcode cpptst

export: cc-fcode
	cp -f cc-fcode /cgt/bin

cctst: $(COBJS) $(CPPOBJS) cctst.o 
	$(CC) $(CFLAGS) -o cctst $(CPPOBJS) $(COBJS) cctst.o $(LIBS)
cctst.o: cctst.cc cc.h ccsym.h cctype.h cpp.h ccsetup.h

cc-fcode: $(COBJS) $(FCODE) $(CPPOBJS)
	$(CC) $(CFLAGS) -o cc-fcode $(CPPOBJS) $(COBJS) $(FCODE) $(LIBS)
ccfcode.o: ccfcode.cc cc.h ccsym.h cctype.h cpp.h
fcode.o: fcode.cc cc.h ccsym.h cctype.h cpp.h

ccmain.o: ccmain.cc cc.h ccsym.h cctype.h cpp.h

cc-m68k: $(COBJS) $(CTREE) $(CPPOBJS) $(M68K)
	$(CC) $(CFLAGS) -o cc-m68k $(CPPOBJS) $(COBJS) $(CTREE) $(M68K) $(TLIBS)

cc-hc11: $(COBJS) $(CTREE) $(CPPOBJS) $(HC11)
	$(CC) $(CFLAGS) -o cc-hc11 $(CPPOBJS) $(COBJS) $(CTREE) $(HC11) $(TLIBS)

cctst.exe: $(COBJS) $(CTREE) $(CPPOBJS)
	echo $(CFLAGS) -o cctst.exe $(CPPOBJS) $(COBJS) $(CTREE) $(LIBS) >linkit
	$(CC) @linkit

cpptst: cppmain.o $(CPPOBJS)
	$(CC) $(CFLAGS) -o cpptst cppmain.o $(CPPOBJS) $(LIBS)
cppmain.o: cppmain.cc cpp.h cppdefs.h

cpptst.exe: cppmain.o $(CPPOBJS)
	echo $(CFLAGS) -o cpptst.exe cppmain.o $(CPPOBJS) $(LIBS) >linkit
	$(CC) @linkit

test: testcpp tests

testcpp: cpptst
	./cpptst testcpp.h 2>&1 | diff testcpp.good -

tests: cctst
	(cd Tests; make)

tags: $(FILES) tokens.h parser.cc
	cxxtags $(FILES) tokens.h parser.cc

fileuse.o: fileuse.cc fileuse.h
globals.o: globals.cc globals.h cppdefs.h cpp.h
cppscan.o: cppscan.cc cpp.h cppdefs.h tokens.h
cppimpl.o: cppimpl.cc cpp.h cppdefs.h tokens.h
cppmac.o: cppmac.cc cpp.h cppdefs.h tokens.h
cppdir.o: cppdir.cc cpp.h cppdefs.h tokens.h
cppif.o: cppif.cc cpp.h cppdefs.h tokens.h
error.o: error.cc error.h cpp.h cppdefs.h
ytab.o: ytab.cc cpp.h cppdefs.h cc.h ccsym.h ccnode.h cctype.h
ytab.cc: ccgram.y
	$(YACC) $(YFLAGS) ccgram.y
	@[ -f y.tab.h ] && mv y.tab.h ytab.h || exit 0
	@[ -f y.tab.c ] && mv y.tab.c ytab.cc || exit 0
	@[ -f y.output ] && mv y.output ytab.out || exit 0
#ytab.h: ccgram.y
ccsetup.o: ccsetup.cc ccsetup.h globals.h cpp.h
cckey.o: cckey.cc cc.h tokens.h
ccsym.o: ccsym.cc ccsym.h ccnode.h cctype.h
cctype.o: cctype.cc cctype.h ccsym.h cc.h cpp.h cppdefs.h tokens.h
cctype2.o: cctype2.cc cctype.h ccsym.h cc.h cpp.h cppdefs.h
ccrules.o: ccrules.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccstor.o: ccstor.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccimpl.o: ccimpl.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h
ccnode.o: ccnode.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccnode2.o: ccnode2.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccnode3.o: ccnode3.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
cceval.o: cceval.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccinit.o: ccinit.cc cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
cctree.o: cctree.cc cctree.h cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
cctree2.o: cctree2.cc cctree.h cctype.h ccsym.h ccnode.h cc.h cpp.h cppdefs.h tokens.h
ccdebug.o: ccdebug.cc cctype.h ccsym.h ccnode.h cc.h
parser.o: parser.cc cpp.h cppdefs.h cc.h ccsym.h ccnode.h cctype.h
parser.cc: ccgram.w
	wacco ccgram.w

.SUFFIXES:	.cc .o
.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o cc-* cctst cpptst lexyy.* ytab.* mon.out


tar:
	tar -zcf cc.tgz $(FILES)
tarall:
	tar -zcf cc.tgz $(FILES) RCS Tests

files:
	@echo $(FILES)

difflist:
	@for f in $(FILES); do rcsdiff -q $$f >/dev/null || echo $$f; done
