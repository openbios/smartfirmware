INCLUDES = -i :
COMPFLAGS = -y MacOS:Temp: -model far
COptions = {COMPFLAGS} {INCLUDES}

.cp.o	Ä	.cc
	{Cplus} {COptions} {DepDir}{Default}.cc -o {TargDir}{Default}.cp.o

OBJS =	stdlibx.cp.o stdiox.cp.o dynarray.cp.o hashtable.cp.o pool.cp.o ¶
	bitset.cp.o quickset.cp.o  xprintf.cp.o path.cp.o getoptx.cp.o merge.cp.o ¶
	expand.cp.o strsplit.cp.o strapp.cp.o strbld.cp.o strcmpi.cp.o strhash.cp.o ¶
	strvec.cp.o backtrace.cp.o meminfo.cp.o
HDRS =	stdlibx.h stdiox.h stringx.h expand.h dynarray.h hashtable.h pool.h ¶
	bitset.h quickset.h pointer.h srchsort.h cachetable.h exception.h getoptx.h ¶
	matrix.h backtrace.h
GENS =	expand.g dynarray.g  hashtable.g pool.g pointer.g srchsort.g ¶
	cachetable.g matrix.g
FILES =	Makefile {HDRS} {GENS} stdlibx.cc stdiox.cc merge.cc expand.cc ¶
	dynarray.cc hashtable.cc pool.cc bitset.cc quickset.cc ¶
	xprintf.cc path.cc getoptx.cc meminfo.cc bsearch.cc ¶
	strsplit.cc strapp.cc strbld.cc strcmpi.cc strhash.cc strvec.cc ¶
	backtrace.cc back68k.cc back386.cc

LibCGT	ÄÄ	LibCGT.o

LibCGT.o	ÄÄ	{OBJS}
		Delete -i LibCGT.o
		Lib -o LibCGT.o {OBJS}

expand.cp.o		Ä	expand.cc expand.h
stdlibx.cp.o	Ä	stdlibx.cc stdlibx.h backtrace.h
stdiox.cp.o		Ä	stdiox.cc stdiox.h stdlibx.h expand.h
xprintf.cp.o	Ä	xprintf.cc stdlibx.h stdiox.h
path.cp.o		Ä	path.cc stdiox.h stdlibx.h expand.h
strsplit.cp.o	Ä	strsplit.cc stringx.h stdlibx.h stdiox.h expand.h
dynarray.cp.o	Ä	dynarray.cc dynarray.h stdlibx.h
hashtable.cp.o	Ä	hashtable.cc hashtable.h stdlibx.h
pool.cp.o		Ä	pool.cc pool.h pool.g stdlibx.h stringx.h
bitset.cp.o		Ä	bitset.cc bitset.h stdlibx.h
quickset.cp.o	Ä	quickset.cc quickset.h stdlibx.h
strapp.cp.o		Ä	strapp.cc stringx.h stdlibx.h
strbld.cp.o		Ä	strbld.cc stringx.h stdlibx.h stdiox.h expand.h
strcmpi.cp.o	Ä	strcmpi.cc stringx.h stdlibx.h
strhash.cp.o	Ä	strhash.cc stringx.h stdlibx.h
strvec.cp.o		Ä	strvec.cc stringx.h stdlibx.h
merge.cp.o		Ä	merge.cc srchsort.h
exception.cp.o	Ä	exception.h exception.cc stdlibx.h backtrace.h
getoptx.cp.o	Ä	getoptx.h getoptx.cc stdlibx.h stringx.h
backtrace.cp.o	Ä	backtrace.cc backtrace.h stdlibx.h
back68k.cp.o	Ä	back68k.cc backtrace.h stdlibx.h
back386.cp.o	Ä	back386.cc backtrace.h stdlibx.h
meminfo.cp.o	Ä	meminfo.cc
bsearch.cp.o	Ä	bsearch.cc

expand.h	Ä	expand.g
		CCGen <expand.g >expand.h

dynarray.h	Ä	dynarray.g
		CCGen <dynarray.g >dynarray.h

hashtable.h	Ä	hashtable.g
		CCGen <hashtable.g >hashtable.h

pool.h		Ä		pool.g
		CCGen <pool.g >pool.h

pointer.h	Ä	pointer.g
		CCGen <pointer.g >pointer.h

srchsort.h	Ä	srchsort.g
		CCGen <srchsort.g >srchsort.h

cachetable.h	Ä	cachetable.g
		CCGen <cachetable.g >cachetable.h

matrix.h	Ä	matrix.g
		CCGen <matrix.g >matrix.h
