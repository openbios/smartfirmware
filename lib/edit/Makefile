CXX = g++
.SUFFIXES: .cc
.cc.o:
	$(CXX) $(CFLAGS) -c $<

CFLAGS = -Wall -g -Icgt
# additional flags:
#	-DSGTTY		use BSD <sgtty.h> instead of Posix <termios.h>
#	-DTERMIO	use System V <termio.h> instead of Posix <termios.h>
#	-DDLD		use custom dynamic linker - need libs below

CGT = cgt/libcgt.a
LIBS = $(CGT) -ltermlib

SRCS = Makefile README allchars.c \
	buffer.cc buffer.h bufvar.h defs.h display.cc display.h \
	keymap.cc keymap.h mark.cc rex.cc rex.h search.cc show.cc \
	terminal.cc terminal.h undo.cc undo.h utils.cc window.cc window.h \
	tst.cc tst1.cc tst2.cc tst3.cc \
	cgt/Makefile cgt/dynarray.cc cgt/dynarray.h cgt/expand.cc cgt/expand.h \
	cgt/hashtable.cc cgt/hashtable.h cgt/stdlibx.cc cgt/stdlibx.h \
	cgt/strbld.cc cgt/strhash.cc cgt/stringx.h cgt/xprintf.cc

WOBJS = terminal.o display.o window.o keymap.o
OBJS = utils.o buffer.o undo.o mark.o rex.o

all: tst tgrep

tst: tst2.o $(WOBJS) $(OBJS) $(CGT)
	$(CXX) $(CFLAGS) $(WOBJS) $(OBJS) tst2.o -o tst $(LIBS)

tgrep: tst3.o $(OBJS) $(CGT)
	$(CXX) $(CFLAGS) $(OBJS) tst3.o -o tgrep $(LIBS)

$(CGT):
	cd cgt; make

tst3.o: defs.h buffer.h rex.h
tst2.o: defs.h buffer.h terminal.h display.h window.h keymap.h
buffer.o: defs.h buffer.h undo.h
undo.o: defs.h buffer.h undo.h
mark.o: defs.h buffer.h
terminal.o: defs.h terminal.h
display.o: defs.h buffer.h terminal.h display.h
window.o: defs.h buffer.h terminal.h display.h window.h
keymap.o: defs.h keymap.h
rex.o: defs.h buffer.h rex.h

clean:
	rm -f tst tgrep debug *.o tags cgt/*.[oa]

tags:
	cxxtags *.h *.cc

tar: $(SRCS)
	tar -zcf edit.tgz $(SRCS)

files:
	@echo $(SRCS)
