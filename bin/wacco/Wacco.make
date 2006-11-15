CPlusOptions = -y MacOS:Temp: -model far -i "/:cgt:include"
LIBS = "/:cgt:lib:LibCGT.o"

MPWLIBS = ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		"{CLibraries}"CplusLib.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdCLib.o ¶
		#"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o

.cp.o   Ä    .cc
        {Cplus} {CPlusOptions} {DepDir}{Default}.cc -o {TargDir}{Default}.cp.o

OBJECTS =	sym.cp.o parse.cp.o scan.cp.o build.cp.o check.cp.o read.cp.o ¶
			gen.cp.o main.cp.o libw.cp.o

Wacco ÄÄ {OBJECTS}
	Link -model far -d -c 'MPS ' -t MPST {OBJECTS} ¶
		{LIBS} {MPWLIBS} -o Wacco

Tst ÄÄ parser.c.o scanner.c.o libw.cp.o
	Link -model far -d -c 'MPS ' -t MPST  ¶
		parser.c.o scanner.c.o libw.cp.o ¶
		{MPWLIBS} {LIBS} -o Tst

tparser.c.o Ä tparser.c
tparser.c Ä tgram.w Wacco
	Wacco -h ttokens.h -p tparser.c -s tscanner.l tgram.w
tscanner.c.o Ä tscanner.c
tscanner.c Ä tscanner.l
	Flex -t tscanner.l >tscanner.c

main.cp.o Ä main.cc toks.h defs.h
sym.cp.o Ä sym.cc defs.h
parse.cp.o Ä parse.cc toks.h defs.h
scan.cp.o Ä scan.cc toks.h defs.h
build.cp.o Ä build.cc defs.h
check.cp.o Ä check.cc defs.h
read.cp.o Ä read.cc toks.h defs.h
gen.cp.o Ä gen.cc toks.h defs.h
libw.cp.o Ä libw.cc toks.h
