# Cogent EDB7312 EP7312 ARM eval board
#

# build options for SmartFirmware
# OPTS=-DFAKE_PCI -DDEBUG_MALLOC -DSTANDALONE -DSF_64BIT -DLITTLE_ENDIAN
#
OPTS=-O
#OPTS=-O -DDEBUG_MALLOC

# cross-hosting gcc
#
COMP=arm-unknown-elf
CC= $(COMP)-gcc
LD= $(COMP)-ld
AS=	$(COMP)-as
SIZE=	$(COMP)-size
OBJCOPY=	$(COMP)-objcopy

ZDIR=zlib
AFLAGS=	-x assembler-with-cpp -traditional-cpp
CFLAGS= -Wall -I- -I.. -I../fs -I../exe \
	-I../misc -I. -I$(ZDIR) $(OPTS) -fsigned-char -fno-builtin
#LFLAGS = -oformat binary -s -N -Ttext 0xF7020000 -e start -defsym _etext=etext
LFLAGS = -s -N -Ttext 0xF7020000 -e start -defsym _etext=etext
LELFFLAGS = -s -N -Ttext 0xF7020000 -e start -defsym _etext=etext
#LFLAGS_WIG = -oformat srec -s -N -Ttext 0xC0100000 -e start -defsym _etext=etext
LFLAGS_WIG = -s -N -Ttext 0xC0100000 -e start -defsym _etext=etext
LFLAGS_RAM = -s -N -Ttext 0xF0100000 -e start -defsym _etext=etext
LFLAGS_RAM2 = -s -N -Ttext 0xF0200000 -e start -defsym _etext=etext
LIBS=$(ZDIR)/libz.a /usr/local/$(COMP)/lib/libc.a	\
	/usr/local/lib/gcc-lib/$(COMP)/2.96/libgcc.a

OBJS= machdep.o main.o cmdio.o control.o display.o errs.o exec.o funcs.o \
	packages.o table.o failsafe.o funcs64.o forth.o device.o other.o \
	admin.o nvedit.o debug.o fsutils.o cpu-arm.o arm.o root.o fb.o stdlib.o \
	alloc.o chosen.o memory.o nvram.o obptftp.o deblock.o disklbl.o client.o \
	fs.o dospart.o dosfat.o bsdpart.o bsdufs.o iso9660.o \
	exe.o loadfc.o coff.o elf.o gzip.o fb7312.o screen.o kbd.o kbd_test.o \
	cs8900.o uart.o console.o sram.o misc.o \
	flash.o nand.o gflash.o flash_intel.o flash_sharp.o mmu.o ide.o atadisk.o 

all:	of of.wig of.ram of.ram2 
 
of:	locore.o rev.o $(OBJS) markrom
	$(LD) $(LFLAGS) -Map of.map -o of.elf locore.o rev.o $(OBJS) $(LIBS)
	$(OBJCOPY) -O binary of.elf of
#	dd if=of.tmp of=of.tmpa bs=1k count=20
#	dd if=of.tmp of=of.tmpb bs=1k skip=896
#	cat of.tmpb of.tmpa >of
#	rm of.tmp of.tmpa of.tmpb
	./markrom of
#	cp of /var/tmp/ep7312/download/of.rom
	cp of /tftpboot
	../inno/mapsort.sh of.map >of.map.sorted

of.elf:	locore.o rev.o $(OBJS) markrom
	$(LD) $(LELFFLAGS) -o of.elf locore.o rev.o $(OBJS) $(LIBS)
 
of.wig:	locore.wig.o rev.o $(OBJS) 
	$(LD) $(LFLAGS_WIG) -Map of.wig.map -o of.wig.elf locore.wig.o rev.o $(OBJS) $(LIBS)
	$(OBJCOPY) -O binary of.wig.elf of.wig
	cp of.wig /var/tmp/of.out
	../inno/mapsort.sh of.wig.map >of.wig.map.sorted
 
x:	locore.o rev.o x.o initmap.o
	$(LD) $(LFLAGS) -Map x.map -o x locore.o rev.o x.o initmap.o
	dd if=x of=xx bs=64k count=1 conv=sync
	mv xx x
	cp x /var/tmp/ep7312/download/x.rom
	../inno/mapsort.sh x.map >x.map.sorted
 
x.wig:	locore.wig.o rev.o x.o initmap.o
	$(LD) $(LFLAGS_WIG) -Map x.wig.map -o x.wig locore.wig.o rev.o x.o initmap.o
	cp x.wig /var/tmp/x.out
	../inno/mapsort.sh x.wig.map >x.wig.map.sorted
 
of.ram:	locore.ram.o rev.o $(OBJS) 
	$(LD) $(LFLAGS_RAM) -Map of.ram.map -o of.ram locore.ram.o rev.o $(OBJS) $(LIBS)
	cp of.ram /tftpboot
 
of.ram2:	locore.ram.o rev.o $(OBJS) 
	$(LD) $(LFLAGS_RAM2) -Map of.ram2.map -o of.ram2 locore.ram.o rev.o $(OBJS) $(LIBS)
	cp of.ram2 /tftpboot
	../inno/mapsort.sh of.ram2.map >of.ram2.map.sorted

tags: of
	../mktags *.[hcS] ../*.[hc] ../isa/*.[hc] ../pci/*.[hc] \
		../scsi/*.[hc] ../exe/*.[hc]

markrom:	markrom.c
	cc -o markrom markrom.c

clean:
	rm -f *.o of of.*


#
# Basic header-file dependancies
#
admin.o:	../admin.c ../defs.h machdep.h
alloc.o:	../alloc.c ../defs.h machdep.h
client.o:	../client.c ../defs.h machdep.h
chosen.o:	../chosen.c ../defs.h machdep.h
cmdio.o:	../cmdio.c ../defs.h machdep.h
control.o:	../control.c ../defs.h machdep.h
cpu.o:		../cpu.c ../defs.h machdep.h
cpu-ppc.o:	../cpu-ppc.c ../defs.h machdep.h
root.o:		../root.c ../defs.h machdep.h
device.o:	../device.c ../defs.h machdep.h
errs.o:		../errs.c ../defs.h ../errs.h machdep.h
exec.o:		../exec.c ../defs.h machdep.h
failsafe.o:	../failsafe.c ../defs.h machdep.h
flash_intel.o:	../flash_intel.c ../defs.h machdep.h
flash_sharp.o:	../flash_sharp.c ../defs.h machdep.h
forth.o:	../forth.c ../defs.h machdep.h
fsutils.o:	../fsutils.c ../defs.h machdep.h
funcs.o:	../funcs.c ../defs.h machdep.h
funcs64.o:	../funcs64.c ../defs.h machdep.h
gflash.o:	../gflash.c ../defs.h machdep.h
main.o:		../main.c ../defs.h machdep.h
memory.o:	../memory.c ../defs.h machdep.h
nvedit.o:	../nvedit.c ../defs.h machdep.h
nvram.o:	../nvram.c ../defs.h machdep.h
packages.o:	../packages.c ../defs.h machdep.h
stdio.o:	../stdio.c ../defs.h machdep.h
table.o:	../table.c ../defs.h machdep.h
obptftp.o:	../obptftp.c ../defs.h machdep.h
other.o:	../other.c ../defs.h machdep.h
deblock.o:	../deblock.c ../defs.h machdep.h
disklbl.o:	../disklbl.c ../defs.h machdep.h
debug.o:	../debug.c ../defs.h machdep.h
display.o:	../display.c ../defs.h machdep.h
fb.o:		../fb.c ../defs.h machdep.h
stdlib.o:	../stdlib.c ../defs.h machdep.h
token.o:	../token.c ../defs.h machdep.h

#
# machine-dependent interface
#
cpu-arm.o:	cpu-arm.c ../defs.h machdep.h
flash.o:	flash.c ../defs.h machdep.h
mmu.o:		mmu.c ../defs.h machdep.h
ide.o:		ide.c ../defs.h machdep.h
nand.o:		nand.c ../defs.h machdep.h
console.o:	console.c ../defs.h machdep.h
uart.o:		uart.c ../defs.h machdep.h
cs8900.o:	cs8900.c ../defs.h machdep.h
misc.o:		misc.c ../defs.h machdep.h
kbd.o:		kbd.c ../defs.h machdep.h
screen.o:	screen.c ../defs.h machdep.h
fb7312.o:	fb7312.c ../defs.h machdep.h
arm.o:		arm.c ../defs.h machdep.h
machdep.o:	machdep.c machdep.h \
		../defs.h ../pci/pci.h ../fs/fs.h \
		../8x16.font ../cour16x23.font ../cour32x44.font ../cour8x16.font

moddi3.o:	$(MIPS)/moddi3.c
umoddi3.o:	$(MIPS)/umoddi3.c
divdi3.o:	$(MIPS)/divdi3.c
qdivrem.o:	$(MIPS)/qdivrem.c
udivdi3.o:	$(MIPS)/udivdi3.c
cmpdi2.o:	$(MIPS)/cmpdi2.c

rev.o:	rev.c $(OBJS) locore.o locore.ram.o locore.wig.o
	$(CC) $(CFLAGS) -DREVISION=\"`./increv`\" -DBUILDTIME=\"`date +%Y%m%d%H%M%S`\" -c $<
locore.o:	locore.S ../defs.h ../isa/isa.h machdep.h
	$(CC) -DROM -DMMU -Wall -I. $(OPTS) -c $<
locore.ram.o:	locore.S ../defs.h ../isa/isa.h machdep.h
	$(CC) -DMMU -Wall -I. $(OPTS) -c locore.S -o locore.ram.o
locore.wig.o:	locore.S ../defs.h ../isa/isa.h machdep.h
	$(CC) -DWIG -Wall -I. $(OPTS) -c locore.S -o locore.wig.o
pagetable.o:	pagetable.s ../defs.h ../isa/isa.h machdep.h
	$(CC) -DROM -Wall -I. $(OPTS) -c $<

#
# file-system specific objects
#
fs.o:		../fs/fs.c ../defs.h ../fs/fs.h machdep.h
dospart.o:	../fs/dospart.c ../defs.h ../fs/fs.h ../fs/dos.h machdep.h
bsdpart.o:	../fs/bsdpart.c ../defs.h ../fs/fs.h machdep.h
bsdufs.o:	../fs/bsdufs.c ../defs.h ../fs/fs.h machdep.h
dosfat.o:	../fs/dosfat.c ../defs.h ../fs/fs.h machdep.h
iso9660.o:	../fs/iso9660.c ../defs.h ../fs/fs.h machdep.h

#
# executable-image objects
#
exe.o:		../exe/exe.c ../defs.h ../exe/exe.h machdep.h
loadfc.o:	../exe/loadfc.c ../defs.h ../exe/exe.h machdep.h
coff.o:		../exe/coff.c ../defs.h ../exe/exe.h machdep.h
elf.o:		../exe/elf.c ../defs.h ../exe/exe.h machdep.h
gzip.o:		../exe/gzip.c ../defs.h ../exe/exe.h machdep.h

#
# ATA disk driver
#
atadisk.o:	../isa/atadisk.c ../defs.h machdep.h
