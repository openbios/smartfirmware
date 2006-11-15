COMPFLAGS = -y MacOS:Temp: -model far
INCLUDES = -i : -i ::cgt
COptions = {COMPFLAGS} {INCLUDES}

.cp.o	Ä	.cc
	{Cplus} {COptions} {DepDir}{Default}.cc -o {TargDir}{Default}.cp.o

OBJS = integer.cp.o itostr.cp.o ibits.cp.o itol.cp.o
HDRS = integer.h
LIB = LibInteger.o
LIBS = ::cgt:LibCgt.o

LibInteger	ÄÄ	{LIB} TestInt

{LIB}	ÄÄ	makefile {OBJS}
		Delete -i {LIB}
		Lib -o {LIB} {OBJS}

TestInt	ÄÄ	makefile test.cp.o {LIB}
	Link -model far -d -c 'MPS ' -t MPST ¶
		test.cp.o {LIB} {LIBS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		"{CLibraries}"CplusLib.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdCLib.o ¶
		#"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o ¶
		-o TestInt
	TestInt >test.out
	Compare test.out test.std

integer.cp.o	Ä	integer.cc integer.h
ibits.cp.o		Ä	ibits.cc integer.h
itostr.cp.o		Ä	itostr.cc integer.h
itol.cp.o		Ä	itol.cc integer.h
test.cp.o		Ä	test.cc integer.h uinteger.h
