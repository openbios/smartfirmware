CFLAGS=-g -pipe

ALL=elf2bin elf2tekhex fcode2rom forth2c showsyms tohex xpm2bin pad

all: $(ALL)

elf2bin: elf2bin.c
elf2tekhex: elf2tekhex.c
fcode2rom: fcode2rom.c
forth2c: forth2c.c
showsyms: showsyms.c
tohex: tohex.c
xpm2bin: xpm2bin.c
pad: pad.c

clean:
	rm -f $(ALL)
