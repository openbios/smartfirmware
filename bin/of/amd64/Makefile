#
# i386 build using FreeBSD bootloader
#

# build options for SmartFirmware
# -DFAKE_PCI -DDEBUG_MALLOC -DSTANDALONE -DSF_64BIT -DLIMITED=512

#-DPCI_NO_EXPANSION_ROM
#-DDETAILED_HELP
#-DISA_HACK
#-DDMA_TEST
#-DSUN_COMPATIBILITY
OPTS=-DDMA_TEST -DSUN_COMPATIBILITY

ROM_LOAD= 0x00B00000
ROM_STACK=0x00A00000

RAM_LOAD= 0x00E00000
RAM_STACK=0x00A00000

COMP=x86_64-unknown-netbsd
CC=$(COMP)-gcc
LD=$(COMP)-ld
AS=$(COMP)-as
NM=$(COMP)-nm
SIZE=$(COMP)-size
STRIP=$(COMP)-strip
OBJCOPY=$(COMP)-objcopy

ZDIR=zlib
ZLIB=$(ZDIR)/libz.a

ROM_FLAGS=-DLOADADDR=${ROM_LOAD} -DSTACKADDR=${ROM_STACK} -DROM_BUILD
RAM_FLAGS=-DLOADADDR=${RAM_LOAD} -DSTACKADDR=${RAM_STACK}

CFLAGS=-Wall -pipe -I.. -I../pci -I../isa -I../usb \
	-I../scsi -I../fs -I../exe -I$(ZDIR) -I. $(OPTS)
LFLAGS=-N -nostdlib

LIBS=$(ZLIB)
#LIBS=

all: of of.rom chello of.ram of32


OBJS= main.o fcdrv.o \
	cmdio.o control.o display.o errs.o exec.o funcs.o packages.o table.o \
	funcs64.o forth.o device.o other.o admin.o nvedit.o debug.o machdep.o \
	client.o nvram.o fb.o stdlib.o alloc.o chosen.o memory.o obptftp.o \
	deblock.o disklbl.o root.o cpu-amd64.o failsafe.o \
	isa.o isabase.o ns16550.o vga.o ne2000.o kbd.o ata.o atadisk.o \
	pci.o pciprobe.o pcidev.o ppb.o pcialloc.o pcicode.o pcibase.o \
	cardbus.o pciisa.o pcidisp.o amdlance.o decether.o ethexpro.o \
	ncrscsi.o scsi.o scsidisk.o \
	usb.o usbhub.o usbkbd.o ohci.o \
	fs.o dospart.o dosfat.o bsdpart.o bsdufs.o ext2fs.o iso9660.o \
	exe.o loadfc.o elf64.o gzip.o rawbin.o amd8111.o hammer.o \
	gflash.o flash_sharp.o flash_intel.o flash_sst.o flash.o mmu.o \
	msr.o


#
# 64-bit ROM build
#
of:	start.o rev.o $(OBJS) Makefile $(ZLIB)
	$(LD) $(LFLAGS) -e start64 -Ttext ${ROM_LOAD} -Map of.map \
		-o of start.o rev.o $(OBJS) $(LIBS)
	$(NM) -n of > of.syms
	$(SIZE) of

start.o:	start.S Makefile
	$(CC) $(CFLAGS) $(ROM_FLAGS) -c start.S

rev.o:	rev.c $(OBJS) start.o start1.o start2.o start2r.o startr.o startram.o
	cp revision lastrevision
	date +%Y%m%d%H%M%S >buildtime
	$(CC) $(CFLAGS) -DREVISION=\"`./increv`\" -DBUILDTIME=\"`cat buildtime`\" -c $<

of.rom: of start1.o start2.bin
	$(OBJCOPY) -v -S -O binary of of.t
	ls -l of.t
	../misc/pad -b 0xFF -l 960k 960k of.t > of.rom && rm -f of.t
#	../misc/pad -S -l 960k 960k of.t > of.rom && rm -f of.t
	../misc/pad -l 56k 56k start2.bin >> of.rom
	../misc/pad -s 5 8k start1.o >> of.rom

of.upd:	of start1.o start2.bin markrom
	$(OBJCOPY) -v -S -O binary of of.t
	ls -l of.t
	../misc/pad -b 0xFF -s 5 8k start1.o >start1.bin
	./markrom -v `cat lastrevision` -b `cat buildtime` -o of.upd \
		0xFFF00000 of.t 0xFFFF0000 start2.bin 0xFFFFE000 start1.bin
	gzip of.upd
	mv of.upd.gz of.upd

start1.o : start1.as86 Makefile
	m4 $(ROM_FLAGS) start1.as86 > start1.s
	as86 -l start1.lst -b start1.o start1.s || rm -f start1.o

start2.o: start2.c Makefile
	$(CC) -m32 -Wall -pipe -I.. -I. $(ROM_FLAGS) -c start2.c

start2.bin: start2.o
	$(LD) -nostdlib -Ttext 0xFFFF0000 -Tdata 0x1000 -Map start2.map \
		-o start2.b start2.o
	$(OBJCOPY) -v -S -O binary -j .text -j .rodata start2.b start2.bin
	rm -f start2.b

../i386/stdlib.o: ../stdlib.c ../defs.h ../i386/machdep.h
	cd ../i386; make stdlib.o



#
# RAM 64-bit build that loads and runs from SF 64-bit ROM
#
of.ram:	$(OBJS) rev.o Makefile startram.o startr.o $(ZLIB)
	$(LD) -N -e start -Ttext $(RAM_LOAD) -o of.ram -Map of.ram.map \
		startram.o startr.o rev.o $(OBJS) $(LIBS)
	$(NM) -n of.ram > of.ram.syms
	$(SIZE) of.ram

startram.o: 	startram.c Makefile
	$(CC) $(CFLAGS) $(RAM_FLAGS) -c startram.c

#
# RAM 32->64 build that loads and runs under standard Hammer BIOS
#
of32.ram:	of32 Makefile
	$(OBJCOPY) -O elf32-i386 of32 of32.ram

of32:	start2r.o startr.o rev.o $(OBJS) Makefile $(ZLIB)
	$(LD) -N -Ttext $(RAM_LOAD) -o of32 -Map of32.map \
		start2r.o startr.o rev.o $(OBJS) $(LIBS)
	$(NM) -n of32 > of32.syms
	$(SIZE) of32

start2r.o: start2.c Makefile
	$(CC) -m32 -Wall -pipe -I.. -I. $(RAM_FLAGS) \
		-c start2.c -o start2r.o

startr.o:	start.S Makefile
	$(CC) $(CFLAGS) $(RAM_FLAGS) -c start.S -o startr.o

markrom:	markrom.c updateimage.h
	cc -g -o markrom markrom.c

tags: of
	../mktags *.[hcS] ../*.[hc] ../isa/*.[hc] ../pci/*.[hc] ../scsi/*.[hc]

clean:
	rm -f *.o of of.* of32.* chello* *.4th *.fc *.sym *.map *.bin *.lst *.s

tst:	start1.o start2.bin ../i386/stdlib.o
	../misc/pad -b 0xFF 960k > tst
	../misc/pad -l 56k 56k start2.bin >> tst
	../misc/pad -s 5 8k start1.o >> tst

#
# for debugging Fcode drivers
#
fcdrv.c: ../fcode/if_em.fc
	../misc/tohex ../fcode/if_em.fc fcdrv_image > fcdrv.c

../fcode/if_em.fc: ../fcode/if_em.c
	cd ../fcode; make if_em.fc


#
# client-interface demo program - needs to be built as ELF binary
#
chello:	chello.o sfclient.o Makefile
	$(LD) -s -N -Ttext 0x900000 -e main -Map chello.map -o chello \
			chello.o sfclient.o

chello.o: ../misc/chello.c ../misc/sfclient.h machdep.h
sfclient.o: ../misc/sfclient.c ../misc/sfclient.h machdep.h
	$(CC) -DINT64=long -Wall -pipe -I../misc -I. -c ../misc/sfclient.c

#
# make zlib
#
$(ZLIB): zlib-Makefile
	cd $(ZDIR); make -f ../zlib-Makefile

#
# machine-dependent interface
#
machdep.o:	machdep.c ../defs.h machdep.h ../isa/isa.h ../fs/fs.h \
	../8x16.font ../cour16x23.font ../cour32x44.font ../cour8x16.font
#			fcvfx.c fcvcq.c
pcibase.o:	pcibase.c ../defs.h machdep.h ../pci/pci.h
isabase.o:	isabase.c ../defs.h machdep.h ../isa/isa.h
nvram.o:	../nvram.c ../defs.h machdep.h
hammer.o:	../i386/hammer.c ../defs.h machdep.h
flash.o:	flash.c ../flash.h ../defs.h machdep.h updateimage.h

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
cpu-amd64.o:	cpu-amd64.c ../defs.h machdep.h
deblock.o:	../deblock.c ../defs.h machdep.h
debug.o:	../debug.c ../defs.h machdep.h
device.o:	../device.c ../defs.h machdep.h
disklbl.o:	../disklbl.c ../defs.h machdep.h ../fs/fs.h
display.o:	../display.c ../defs.h machdep.h
errs.o:		../errs.c ../defs.h ../errs.h machdep.h
exec.o:		../exec.c ../defs.h machdep.h
failsafe.o:	../failsafe.c ../defs.h machdep.h
fb.o:		../fb.c ../defs.h machdep.h
forth.o:	../forth.c ../defs.h machdep.h
funcs.o:	../funcs.c ../defs.h machdep.h
funcs64.o:	../funcs64.c ../defs.h machdep.h
main.o:		../main.c ../defs.h machdep.h
memory.o:	../memory.c ../defs.h machdep.h
nvedit.o:	../nvedit.c ../defs.h machdep.h
obptftp.o:	../obptftp.c ../defs.h machdep.h
other.o:	../other.c ../defs.h machdep.h
packages.o:	../packages.c ../defs.h machdep.h
root.o:		../root.c ../defs.h machdep.h
stdlib.o:	../stdlib.c ../defs.h machdep.h
table.o:	../table.c ../defs.h machdep.h
token.o:	../token.c ../defs.h machdep.h

#
# PCI-specific object files
#
pci.o:		../pci/pci.c ../defs.h ../pci/pci.h machdep.h
pciprobe.o:	../pci/pciprobe.c ../defs.h ../pci/pci.h machdep.h
pcidev.o:	../pci/pcidev.c ../defs.h ../pci/pci.h machdep.h
ppb.o:		../pci/ppb.c ../defs.h ../pci/pci.h machdep.h
pcialloc.o:	../pci/pcialloc.c ../defs.h ../pci/pci.h machdep.h
pcicode.o:	../pci/pcicode.c ../defs.h ../pci/pci.h machdep.h
cardbus.o:	../pci/cardbus.c ../defs.h ../pci/pci.h machdep.h
dc21140.o:	../pci/dc21140.c ../defs.h ../pci/pci.h machdep.h
ethexpro.o:	../pci/ethexpro.c ../defs.h ../pci/pci.h machdep.h
fakepci.o:	../pci/fakepci.c ../defs.h ../pci/pci.h machdep.h
pcidisp.o:	../pci/pcidisp.c ../defs.h ../pci/pci.h machdep.h
pciisa.o:	../pci/pciisa.c ../defs.h ../pci/pci.h machdep.h
amdlance.o:	../pci/amdlance.c ../defs.h ../pci/pci.h machdep.h
decether.o:	../pci/decether.c ../defs.h ../pci/pci.h machdep.h
ncrscsi.o:	../pci/ncrscsi.c ../defs.h ../pci/pci.h machdep.h ../scsi/scsi.h

#
# SCSI-specific object files
#
scsi.o:	../scsi/scsi.c ../defs.h machdep.h ../scsi/scsi.h
scsidisk.o:	../scsi/scsidisk.c ../defs.h machdep.h ../scsi/scsi.h

#
# ISA-specific object files
#
isa.o:		../isa/isa.c ../defs.h machdep.h ../isa/isa.h
ata.o:		../isa/ata.c ../defs.h machdep.h ../isa/isa.h
kbd.o:		../isa/kbd.c ../defs.h machdep.h ../isa/isa.h
ns16550.o:	../isa/ns16550.c ../defs.h machdep.h ../isa/isa.h
vga.o:		../isa/vga.c ../defs.h machdep.h ../isa/isa.h
ne2000.o:	../isa/ne2000.c ../defs.h machdep.h ../isa/isa.h

#
# ATA-specific object files
#
atadisk.o:	../isa/atadisk.c ../defs.h machdep.h

#
# USB-specific object files
#
usb.o:	../usb/usb.c ../defs.h machdep.h ../usb/usb.h ../pci/pci.h
usbhub.o:	../usb/usbhub.c ../defs.h machdep.h ../usb/usb.h
usbkbd.o:	../usb/usbkbd.c ../defs.h machdep.h ../usb/usb.h
ohci.o:	../usb/ohci.c ../defs.h machdep.h ../usb/usb.h ../usb/ohci.h \
		../pci/pci.h
uhci.o:	../usb/uhci.c ../defs.h machdep.h ../usb/usb.h ../pci/pci.h

#
# file-system specific objects
#
fs.o:		../fs/fs.c ../defs.h ../fs/fs.h machdep.h
dospart.o:	../fs/dospart.c ../defs.h ../fs/fs.h ../fs/dos.h machdep.h
dosfat.o:	../fs/dosfat.c ../defs.h ../fs/fs.h ../fs/dos.h machdep.h
bsdpart.o:	../fs/bsdpart.c ../defs.h ../fs/fs.h machdep.h
bsdufs.o:	../fs/bsdufs.c ../defs.h ../fs/fs.h machdep.h
ext2fs.o:	../fs/ext2fs.c ../defs.h ../fs/fs.h machdep.h
iso9660.o:	../fs/iso9660.c ../defs.h ../fs/fs.h machdep.h

#
# executable-image objects
#
exe.o:		../exe/exe.c ../defs.h ../exe/exe.h machdep.h
loadfc.o:	../exe/loadfc.c ../defs.h ../exe/exe.h machdep.h
aout.o:		../exe/aout.c ../defs.h ../exe/exe.h machdep.h
coff.o:		../exe/coff.c ../defs.h ../exe/exe.h machdep.h
elf.o:		../exe/elf.c ../defs.h ../exe/exe.h machdep.h
elf64.o:	../exe/elf64.c ../defs.h ../exe/exe.h machdep.h
gzip.o:		../exe/gzip.c ../defs.h ../exe/exe.h machdep.h
rawbin.o:	../exe/rawbin.c ../defs.h ../exe/exe.h machdep.h

#
# generic flash support
#
gflash.o:	../gflash.c ../flash.h ../defs.h machdep.h
flash_sharp.o:	../flash_sharp.c ../flash.h ../defs.h machdep.h
flash_intel.o:	../flash_intel.c ../flash.h ../defs.h machdep.h
flash_sst.o:	../flash_sst.c ../flash.h ../defs.h machdep.h
