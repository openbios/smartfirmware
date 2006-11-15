CC = mwcc
LD = mwld

# Add -DLITTLE_ENDIAN for those sorts of machines.
#COMPFLAGS = -O -g -DLITTLE_ENDIAN
#LITTLE_ENDIAN is already defined in FreeBSD
COMPFLAGS = -O -g
INCLUDES = -I- -I. -I../integer -I../cgt
CFLAGS = $(COMPFLAGS) $(INCLUDES)
LIBS = ../cgt/libcgt.a

OBJS =	float.o fltutil.o fltmisc.o fltmath.o fltconst.o fltlog.o \
	fltsqrt.o flttrig.o fltstr.o fltdbl.o fltdump.o
HDRS = floatc.h
LIB = libfloat.a
FILES =	Makefile Makefile-dos LibFloat.make testplan \
	floatc.h float.cc fltutil.cc fltmisc.cc \
	fltmath.cc fltconst.cc fltlog.cc fltsqrt.cc flttrig.cc \
	fltstr.cc fltdbl.cc fltdump.cc \
	test1.cc test1.std test2.cc test2.in test2.std \
	test3.cc test3.std test4.cc test4.std test5.cc test5.std
TARGET = /cgt

TEST1 = test1.cc test1.std
TEST2 = test2.cc test2.in test2.std
TEST3 = test3.cc test3.std
TEST4 = test4.cc test4.std
TEST5 = test5.cc test5.std
TESTLOG = tstcvt.cc tstlog.cc
TESTS = $(TEST1) $(TEST2) $(TEST3) $(TEST4) $(TEST5)

all:	$(LIB) $(HDRS) tests

export:	$(LIB) $(HDRS) 
	cp $(HDRS) $(TARGET)/include
	cp $(LIB) $(TARGET)/lib

$(LIB):	$(OBJS)
	rm -f $(LIB)
	$(LD) -sym full -xml -o $(LIB) $(OBJS)

tests: test1 test2 test3 test4 test5
	-./test1 >test1.out 2>&1
	-diff test1.out test1.std >test1.diff
	-./test2 <test2.in >test2.out 2>&1
	-diff test2.out test2.std >test2.diff
	-./test3 >test3.out 2>&1
	-diff test3.out test3.std >test3.diff
	-./test4 >test4.out 2>&1
	-diff test4.out test4.std >test4.diff
	-./test5 >test5.out 2>&1
	-diff test5.out test5.std >test5.diff

test1:	test1.o $(LIB)
	$(CC) $(CFLAGS) -o test1 test1.o $(LIB) $(LIBS)

test2:	test2.o $(LIB)
	$(CC) $(CFLAGS) -o test2 test2.o $(LIB) $(LIBS)

test3:	test3.o $(LIB)
	$(CC) $(CFLAGS) -o test3 test3.o $(LIB) $(LIBS)

test4:	test4.o $(LIB)
	$(CC) $(CFLAGS) -o test4 test4.o $(LIB) $(LIBS)

test5:	test5.o $(LIB)
	$(CC) $(CFLAGS) -o test5 test5.o $(LIB) $(LIBS)

tstcvt:	tstcvt.o $(LIB)
	$(CC) $(CFLAGS) -o tstcvt tstcvt.o $(LIB) $(LIBS)

tstlog:	tstlog.o $(LIB)
	$(CC) $(CFLAGS) -o tstlog tstlog.o $(LIB) $(LIBS)

float.o:	float.cc floatc.h
fltutil.o:	fltutil.cc floatc.h
fltmisc.o:	fltmisc.cc floatc.h
fltmath.o:	fltmath.cc floatc.h
fltconst.o:	fltconst.cc floatc.h
fltlog.o:	fltlog.cc floatc.h
fltsqrt.o:	fltsqrt.cc floatc.h
flttrig.o:	flttrig.cc floatc.h
fltstr.o:	fltstr.cc floatc.h
fltdbl.o:	fltdbl.cc floatc.h
fltdump.o:	fltdump.cc floatc.h
test1.o:	test1.cc floatc.h
test2.o:	test2.cc floatc.h
test3.o:	test3.cc floatc.h
test4.o:	test4.cc floatc.h
test5.o:	test5.cc floatc.h
tstcvt.o:	tstcvt.cc floatc.h
tstlog.o:	tstlog.cc floatc.h

clean:
	rm -f *.o *.a core

.SUFFIXES: .cc .o

.cc.o:
	$(CC) -c $(CFLAGS) $<

tar:
	tar -zcf float.tgz $(FILES)
tarall:
	tar -zcf float.tgz $(FILES) RCS

files:
	@echo $(FILES)
