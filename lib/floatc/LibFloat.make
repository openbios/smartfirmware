COMPFLAGS = -y MacOS:Temp: -model far
INCLUDES = -i : -i ::integer -i ::cgt
COptions = {COMPFLAGS} {INCLUDES}
LIBS = ::cgt:LibCGT.o
MPWLIBS = "{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		"{CLibraries}"CplusLib.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdCLib.o ¶
		#"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o


.cp.o   Ä    .cc
        {Cplus} {COptions} {DepDir}{Default}.cc -o {TargDir}{Default}.cp.o

OBJS = float.cp.o fltutil.cp.o fltmisc.cp.o fltmath.cp.o fltconst.cp.o ¶
	fltlog.cp.o fltsqrt.cp.o flttrig.cp.o fltstr.cp.o fltdbl.cp.o fltdump.cp.o
HDRS = floatc.h
LIB = LibFloat.o
SRCS = Makefile floatc.h float.cc fltutil.cc fltmisc.cc fltmath.cc fltconst.cc ¶
	fltlog.cc fltsqrt.cc flttrig.cc fltstr.cc fltdbl.cc fltdump.cc

TEST1 = test1.cc test1.std
TEST2 = test2.cc test2.in test2.std
TEST3 = test3.cc test3.std
TEST4 = test4.cc test4.std
TEST5 = test5.cc test5.std
TESTLOG = tstcvt.cc tstlog.cc
TESTS = {TEST1} {TEST2} {TEST3} {TEST4} {TEST5}

LibFloat	ÄÄ	{LIB} {HDRS} Test1 Test2 Test3 Test4 Test5 tests

{LIB}	ÄÄ	{OBJS}
	Delete -i {LIB}
	Lib -o {LIB} {OBJS}

tests	ÄÄ Test1 Test2 Test3 Test4 Test5
	Test1 >t1.out
	Test2 <test2.in >t2.out
	Test3 >t3.out
	Test4 >t4.out
	Test5 >t5.out
	Compare t1.out test1.std >t1.diff
	Compare t2.out test2.std >t2.diff
	Compare t3.out test3.std >t3.diff
	Compare t4.out test4.std >t4.diff
	Compare t5.out test5.std >t5.diff

Test1	Ä	test1.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST test1.cp.o {MPWLIBS} {LIB} {LIBS} -o Test1

Test2	Ä	test2.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST {LIBS} test2.cp.o {MPWLIBS} {LIB} -o Test2

Test3	Ä	test3.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST test3.cp.o {MPWLIBS} {LIB} {LIBS} -o Test3

Test4Ä	test4.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST test4.cp.o {MPWLIBS} {LIB} {LIBS} -o Test4

Test5	Ä	test5.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST test5.cp.o {MPWLIBS} {LIB} {LIBS} -o Test5

Tstcvt	Ä	tstcvt.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST tstcvt.cp.o {MPWLIBS} {LIB} {LIBS} -o Tstcvt

Tstlog	Ä	tstlog.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST tstlog.cp.o {LIB} {LIBS} -o Tstlog


float.oÄ	float.cc floatc.h
fltutil.oÄ	fltutil.cc floatc.h
fltmisc.oÄ	fltmisc.cc floatc.h
fltmath.oÄ	fltmath.cc floatc.h
fltconst.oÄ	fltconst.cc floatc.h
fltlog.oÄ	fltlog.cc floatc.h
fltsqrt.oÄ	fltsqrt.cc floatc.h
flttrig.oÄ	flttrig.cc floatc.h
fltstr.oÄ	fltstr.cc floatc.h
fltdbl.oÄ	fltdbl.cc floatc.h
fltdump.oÄ	fltdump.cc floatc.h
test1.oÄ	test1.cc floatc.h
test2.oÄ	test2.cc floatc.h
test3.oÄ	test3.cc floatc.h
test4.oÄ	test4.cc floatc.h
test5.oÄ	test5.cc floatc.h
tstcvt.oÄ	tstcvt.cc floatc.h
tstlog.oÄ	tstlog.cc floatc.h
