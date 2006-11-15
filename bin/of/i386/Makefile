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
OPTS=-DDMA_TEST -DSUN_COMPATIBILITY -DISA_HACK -DPCI_TABLE_SMALL

# cross-hosting gcc
#
#COMP=i386-unknown-elf-
CC=$(COMP)gcc
LD=$(COMP)ld
AS=$(COMP)as
SIZE=$(COMP)size
OBJCOPY=$(COMP)objcopy
STRIP=$(COMP)strip

LOADADDR=0x00500000
STACKADDR=0x0000FFFC

ZDIR=zlib

AFLAGS=-DLOADADDR=${LOADADDR} -DSTACKADDR=${STACKADDR}
CFLAGS=-Wall -pipe -O -I.. -I../pci -I../isa -I../usb \
	-I../scsi -I../fs -I../exe -I$(ZDIR) -I.  \
	$(AFLAGS) $(OPTS)
LFLAGS=-N -Ttext ${LOADADDR} -nostdlib
LIBS=$(ZDIR)/libz.a /usr/lib/libc.a	\
#LIBS=$(ZDIR)/libz.a /usr/local/$(COMP)/lib/libc.a	\
#		/usr/local/lib/gcc-lib/$(COMP)/2.97/libgcc.a

all: of


OBJS= start.o main.o fcdrv.o \
	cmdio.o control.o display.o errs.o exec.o funcs.o packages.o table.o \
	funcs64.o forth.o device.o other.o admin.o nvedit.o debug.o machdep.o \
	client.o nvram.o fb.o stdlib.o alloc.o chosen.o memory.o obptftp.o \
	deblock.o disklbl.o root.o cpu.o failsafe.o \
	isa.o isabase.o ns16550.o vga.o ne2000.o kbd.o ata.o atadisk.o \
	pci.o pciprobe.o pcidev.o ppb.o pcialloc.o pcicode.o pcibase.o \
	cardbus.o pciisa.o pcidisp.o amdlance.o decether.o ethexpro.o ncrscsi.o \
	scsi.o scsidisk.o \
	usb.o usbhub.o usbkbd.o ohci.o uhci.o \
	fs.o dospart.o dosfat.o bsdpart.o bsdufs.o ext2fs.o iso9660.o \
	exe.o loadfc.o aout.o coff.o elf.o gzip.o rawbin.o
#	moddi3.o umoddi3.o divdi3.o udivdi3.o qdivrem.o

of:	$(OBJS)
	$(LD) $(LFLAGS) -o of -M $(OBJS) $(LIBS) >of.map
	$(SIZE) of
	cp of /tftpboot/kernel

tags: of
	../mktags *.[hcS] ../*.[hc] ../isa/*.[hc] ../pci/*.[hc] ../scsi/*.[hc]

clean:
	rm -f *.o of of.* *.4th *.fc

#
# ROM-build test
#
#ROMADDR=0x000E0000		# for BOCHS
#ROMADDR=0xFFF00000		# for Intel 1Mb ROM
ROMADDR=0xFFFC0000		# for Supermicro 256Kb ROM
MEMADDR=0x1000
ZRUNADDR=0x10000

ROMOPTS= $(AFLAGS) -DROM -DROMADDR=${ROMADDR} -DMEMADDR=${MEMADDR} \
	-DZRUNADDR=${ZRUNADDR} -I$(ZDIR)

of.rom: ofmin cpureset.o of.z
	dd bs=0x37000 conv=sync count=1 < of.z > of.rom
	$(OBJCOPY) -v -S -O binary -R .data ofmin .x
	$(OBJCOPY) -v -S -O binary -R .text -R .rodata ofmin .y
	cat .x .y | dd obs=0x08F00 conv=osync >> of.rom
	cat cpureset.o >> of.rom
	ls -l of.rom

of.z: of
	$(OBJCOPY) -v -S -O binary of of.x
	../misc/zbuild -r of.x > of.z

ofmin:	ofmin.o stdlib.o startr.o end.o
	$(LD) -s -N -Ttext 0xFFFF7000 -Tdata ${MEMADDR} \
			-Map ofmin.map -o ofmin \
			startr.o ofmin.o $(LIBS) end.o

ofmin.o:	ofmin.c zarr.h
	$(CC) ${ROMOPTS} -Wall -pipe -Os -I.. -I. -c ofmin.c

startr.o:	start.S Makefile
	$(CC) ${ROMOPTS} -Wall -pipe -Os -I.. -I. -c start.S -o startr.o

zarr.h:	zrun
	$(OBJCOPY) -v -S -O binary zrun zrun.bin
	../misc/tohex zrun.bin zrun > zarr.h

zrun:	zrun.o stdlib.o
	$(LD) -s -N -Ttext ${ZRUNADDR} -Map zrun.map -o zrun \
			zrun.o stdlib.o $(LIBS)

zrun.o: zrun.c machdep.h
	$(CC) ${ROMOPTS} -Wall -pipe -Os -I.. -I. -c zrun.c

end.o:	end.S Makefile

cpureset.o:	cpureset.S Makefile
	as86 -0 -l cpureset.lst -b cpureset.oo cpureset.S
	dd if=cpureset.oo of=cpureset.o bs=0x5 skip=1

#
# client-interface demo program - needs to be built as ELF binary
#
chello:	chello.o sfclient.o
	$(LD) -s -N -Ttext 0x200000 -e main -Map chello.map -o chello \
			chello.o sfclient.o $(LIBS)
	cp chello /tftpboot

chello.o: ../misc/chello.c ../misc/sfclient.h machdep.h
sfclient.o: ../misc/sfclient.c ../misc/sfclient.h machdep.h

ctst:	ctst.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0x200000 -e main -Map chello.map -o ctst \
			ctst.o sfclient.o stdlib.o $(LIBS)
	cp ctst /tftpboot

ctst.o: ctst.c ../misc/sfclient.h machdep.h
	$(CC) ${CFLAGS} -I../misc -c ctst.c

#
# for debugging Fcode drivers
#
fcdrv.c: ../fcode/if_em.fc
	../misc/tohex ../fcode/if_em.fc fcdrv_image > fcdrv.c

../fcode/if_em.fc: ../fcode/if_em.c
	cd ../fcode; make


#
# machine-dependent interface
#
machdep.o:	machdep.c ../defs.h machdep.h ../isa/isa.h ../fs/fs.h \
			../8x16.font ../cour16x23.font ../cour32x44.font ../cour8x16.font
start.o:	start.S Makefile
	$(CC) $(CFLAGS) -c start.S
pcibase.o:	pcibase.c ../defs.h machdep.h ../pci/pci.h
isabase.o:	isabase.c ../defs.h machdep.h ../isa/isa.h
moddi3.o:	moddi3.c quad.h
umoddi3.o:	umoddi3.c quad.h
divdi3.o:	divdi3.c quad.h
udivdi3.o:	udivdi3.c quad.h
qdivrem.o:	qdivrem.c quad.h
nvram.o:	../nvram.c ../defs.h machdep.h

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
gzip.o:		../exe/gzip.c ../defs.h ../exe/exe.h machdep.h
rawbin.o:	../exe/rawbin.c ../defs.h ../exe/exe.h machdep.h
