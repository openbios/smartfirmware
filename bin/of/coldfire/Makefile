# Motorola SBC5206 Coldfire Evaluation Board
#

# build options for SmartFirmware
# OPTS=-DFAKE_PCI -DDEBUG_MALLOC -DSTANDALONE -DSF_64BIT -DLITTLE_ENDIAN
#OPTS=-DSUN_COMPATIBILITY -O
OPTS=-DNE2000_BROKEN

# For cross-hosting gcc
COMP=m68k-rom68k-coff
CC= $(COMP)-gcc
LD= $(COMP)-ld
AS=	$(COMP)-as
SIZE=	$(COMP)-size
OBJCOPY=	$(COMP)-objcopy

ZLIB=zlib

AFLAGS=	
CFLAGS= -Wall -pipe -O -m5200 -I.. -I../isa -I../fs -I../exe -I$(ZLIB) -I. $(OPTS)
LFLAGS =
LIBS=/usr/local/lib/gcc-lib/$(COMP)/2.8.1/m5200/libgcc.a $(ZLIB)/libz.a


#OBJS= machdep.o main.o failsafe.o duart.o \
#	cmdio.o control.o display.o errs.o exec.o funcs.o packages.o table.o \
#	funcs64.o forth.o device.o other.o admin.o nvedit.o debug.o \
#	cpu.o root.o fb.o stdlib.o alloc.o chosen.o memory.o obptftp.o deblock.o \
#	disklbl.o isabase.o isa.o ns16550.o ne2000.o client.o \
#	fs.o dospart.o dosfat.o bsdpart.o bsdufs.o \
#	exe.o loadfc.o coff.o elf.o

OBJS= machdep.o main.o failsafe.o duart.o \
	cmdio.o control.o display.o errs.o exec.o funcs.o packages.o table.o \
	forth.o device.o other.o admin.o nvedit.o debug.o \
	cpu.o root.o stdlib.o alloc.o chosen.o memory.o obptftp.o \
	isabase.o isa.o ns16550.o ne2000.o client.o \
	exe.o loadfc.o coff.o elf.o gzip.o rawbin.o sun.o

all: of of.rom

of:	$(OBJS)
	-rm -f of
	$(LD) -e start $(OBJS) $(LIBS) -Map of.map -Ttext 0x10000 -o of -n -s
	$(SIZE) of
	cp of /tftpboot

of.srec: of
	$(OBJCOPY) -O srec of of.srec

tags: of
	../mktags *.[hcS] ../*.[hc] ../isa/*.[hc] ../pci/*.[hc] ../scsi/*.[hc]

clean:
	rm -f *.o of of.*

ROM=0xFFE20000
BSS=0xF0000

of.rom: zrun
	$(OBJCOPY) -O binary zrun of.rom
	cp of.rom /tftpboot

zrun: zrun.o stdlib.o
	-rm -f zrun
	$(LD) $> $(LIBS) -Map zrun.map -Ttext $(ROM) -Tbss $(BSS) -o zrun
	$(SIZE) zrun

zrun.o:	zrun.c zarr.h
	$(CC) -DROM=$(ROM) -DBSS=$(BSS) $(CFLAGS) -c zrun.c

zarr.h: of
	../misc/zbuild of >zarr.h

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
root.o:		../root.c ../defs.h machdep.h
device.o:	../device.c ../defs.h machdep.h
errs.o:		../errs.c ../defs.h ../errs.h machdep.h
exec.o:		../exec.c ../defs.h machdep.h
failsafe.o:	../failsafe.c ../defs.h machdep.h
forth.o:	../forth.c ../defs.h machdep.h
funcs.o:	../funcs.c ../defs.h machdep.h
funcs64.o:	../funcs64.c ../defs.h machdep.h
main.o:		../main.c ../defs.h machdep.h
memory.o:	../memory.c ../defs.h machdep.h
nvedit.o:	../nvedit.c ../defs.h machdep.h
packages.o:	../packages.c ../defs.h machdep.h
stdio.o:	../stdio.c ../defs.h machdep.h
sun.o:		../sun.c ../defs.h machdep.h
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
machdep.o:	machdep.c ../defs.h machdep.h ../isa/isa.h ../fs/fs.h \
			../8x16.font ../cour16x23.font ../cour32x44.font ../cour8x16.font
duart.o:	duart.c ../defs.h ../isa/isa.h machdep.h
isabase.o:	isabase.c ../defs.h ../isa/isa.h machdep.h
pcibase.o:	pcibase.c ../defs.h ../pci/pci.h machdep.h
locore.o:	locore.S ../defs.h ../isa/isa.h machdep.h

#
# PCI-specific object files
#
pci.o:		../pci/pci.c ../defs.h ../pci/pci.h machdep.h
pcialloc.o:	../pci/pcialloc.c ../defs.h ../pci/pci.h machdep.h
dc21140.o:	../pci/dc21140.c ../defs.h ../pci/pci.h machdep.h
fakepci.o:	../pci/fakepci.c ../defs.h ../pci/pci.h machdep.h
pcidisp.o:	../pci/pcidisp.c ../defs.h ../pci/pci.h machdep.h
pciisa.o:	../pci/pciisa.c ../defs.h ../pci/pci.h machdep.h
decether.o:	../pci/decether.c ../defs.h ../pci/pci.h machdep.h
ncrscsi.o:	../pci/ncrscsi.c ../defs.h ../pci/pci.h machdep.h
ncrscsi.o:	../pci/ncrscsi.c ../defs.h ../pci/pci.h ../scsi/scsi.h machdep.h

#
# SCSI-specific object files
#
scsi.o:	../scsi/scsi.c ../defs.h machdep.h ../scsi/scsi.h
scsidisk.o:	../scsi/scsidisk.c ../defs.h machdep.h ../scsi/scsi.h

#
# ISA-specific object file
#
isa.o:		../isa/isa.c ../defs.h ../isa/isa.h machdep.h
kbd.o:		../isa/kbd.c ../defs.h ../isa/isa.h machdep.h
vga.o:		../isa/vga.c ../defs.h ../isa/isa.h machdep.h
ns16550.o:	../isa/ns16550.c ../defs.h ../isa/isa.h machdep.h
ne2000.o:	../isa/ne2000.c ../defs.h ../isa/isa.h machdep.h

#
# file-system specific objects
#
fs.o:		../fs/fs.c ../defs.h ../fs/fs.h machdep.h
dospart.o:	../fs/dospart.c ../defs.h ../fs/fs.h ../fs/dos.h machdep.h
bsdpart.o:	../fs/bsdpart.c ../defs.h ../fs/fs.h machdep.h
bsdufs.o:	../fs/bsdufs.c ../defs.h ../fs/fs.h machdep.h
dosfat.o:	../fs/dosfat.c ../defs.h ../fs/fs.h machdep.h

#
# executable-image objects
#
exe.o:		../exe/exe.c ../defs.h ../exe/exe.h machdep.h
loadfc.o:	../exe/loadfc.c ../defs.h ../exe/exe.h machdep.h
coff.o:		../exe/coff.c ../defs.h ../exe/exe.h machdep.h
elf.o:		../exe/elf.c ../defs.h ../exe/exe.h machdep.h
gzip.o:		../exe/gzip.c ../defs.h ../exe/exe.h machdep.h
rawbin.o:	../exe/rawbin.c ../defs.h ../exe/exe.h machdep.h
