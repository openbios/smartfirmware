#
# standalone build - for testing and tokenizing
#

# build options for SmartFirmware - may also be defined in machdep.h
# -DLITTLE_ENDIAN
# -DFAKE_PCI
# -DDEBUG_MALLOC
# -DLOG_MALLOC
# -DSTANDALONE
# -DSF_64BIT 
# -D__LONGLONG="long long"
# -DPTR_64BIT
# -DLIMITED=512
# -DUSE_STATIC_NAMES
# -DPROPERTY_MEM=32767
# -DSUN_COMPATIBILITY
# -DDETAILED_HELP

# For BeBox
#CC= mwcc
#CXX= mwcc
#OPTS= -DFAKE_PCI -DDEBUG_MALLOC -DSTANDALONE -DSF_64BIT -D__LONGLONG="long long" -DDETAILED_HELP
#CFLAGS= -DBeBox -O -g -I.. -I../pci -I../scsi -I../fs -I../exe -I. $(OPTS)
#CXXFLAGS= $(CFLAGS)
#LFLAGS= -map $@.xMAP
#LIBS=
#SIZE= copyres BeOS.rsrc $@; \
#	addattr BEOS:TYPE application/x-codegen-smartfirmware $@; \
#	mimeset $@; \
#	ls -l
#MAIN=beapp.o
#X11=

# For most Unix systems
CC= gcc
OPTS= -DFAKE_PCI -DDEBUG_MALLOC -DLOG_MALLOC -DSTANDALONE -DSTANDALONE_GUI -DSF_64BIT -DSUN_COMPATIBILITY -D__LONGLONG="long long" -DDETAILED_HELP -Dstart=_start
CFLAGS= -Wall -pipe -g -I.. -I../pci -I../scsi -I../fs -I../exe -I. \
	-I/usr/X11R6/include $(OPTS) $(EXTRA)
LFLAGS=
LIBS= -lz -L/usr/X11R6/lib -lX11
SIZE=size
MAIN=main.o
X11=x11.o

# if necessary for your make
VPATH=.:..:../pci:../isa:../scsi:../exe:../fs

OBJS= $(MAIN) cmdio.o control.o display.o errs.o exec.o funcs.o \
	packages.o table.o funcs64.o forth.o device.o other.o admin.o \
	nvedit.o debug.o machdep.o fb.o token.o stdlib.o alloc.o chosen.o \
	memory.o \
	client.o obptftp.o deblock.o disklbl.o cpu.o root.o failsafe.o \
	pci.o pciprobe.o pcidev.o ppb.o pcialloc.o pcicode.o fakepci.o \
	cardbus.o dc21140.o pcidiskdev.o fakedisk.o decether.o \
	fs.o dospart.o dosfat.o bsdpart.o bsdufs.o ext2fs.o iso9660.o \
	exe.o loadfc.o aout.o coff.o elf.o elf64.o gzip.o rawbin.o sun.o stlb.o \
	$(X11)


of:	$(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(OBJS) $(LIBS)
	$(SIZE) $@

tags: of
	../mktags *.[hcS] ../*.[hc] ../isa/*.[hc] ../pci/*.[hc]

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
device.o:	../device.c ../defs.h machdep.h
errs.o:		../errs.c ../defs.h ../errs.h machdep.h
exec.o:		../exec.c ../defs.h machdep.h
failsafe.o:	../failsafe.c ../defs.h machdep.h
forth.o:	../forth.c ../defs.h machdep.h
gui-dumb.o:	../gui-dumb.c ../defs.h machdep.h
funcs.o:	../funcs.c ../defs.h machdep.h
funcs64.o:	../funcs64.c ../defs.h machdep.h
main.o:		../main.c ../defs.h machdep.h
memory.o:	../memory.c ../defs.h machdep.h
nvedit.o:	../nvedit.c ../defs.h machdep.h
packages.o:	../packages.c ../defs.h machdep.h
root.o:		../root.c ../defs.h machdep.h
table.o:	../table.c ../defs.h machdep.h
obptftp.o:	../obptftp.c ../defs.h machdep.h
other.o:	../other.c ../defs.h machdep.h
deblock.o:	../deblock.c ../defs.h machdep.h
disklbl.o:	../disklbl.c ../defs.h machdep.h ../fs/fs.h
debug.o:	../debug.c ../defs.h machdep.h ../exe/exe.h
display.o:	../display.c ../defs.h machdep.h
fb.o:		../fb.c ../defs.h machdep.h ../logo.h
stdlib.o:	../stdlib.c ../defs.h machdep.h
stlb.o:		../stlb.c ../defs.h machdep.h
sun.o:		../sun.c ../defs.h machdep.h
token.o:	../token.c ../defs.h machdep.h

#
# machine-dependent interface
#
machdep.o:	machdep.c machdep.h ../defs.h \
			../pci/pci.h ../fs/fs.h ../exe/exe.h \
			../8x16.font ../cour16x23.font ../cour32x44.font ../cour8x16.font
fakedisk.o:	fakedisk.c ../defs.h machdep.h
beapp.o:	beapp.cc ../defs.h
x11.o:	x11.c ../defs.h

#
# PCI-specific object files
#
dc21140.o:	../pci/dc21140.c ../defs.h ../pci/pci.h machdep.h
pcidiskdev.o:	../pci/pcidiskdev.c ../defs.h ../pci/pci.h machdep.h
pci.o:		../pci/pci.c ../defs.h ../pci/pci.h machdep.h
pcialloc.o:	../pci/pcialloc.c ../defs.h ../pci/pci.h machdep.h
pcicode.o:	../pci/pcicode.c ../defs.h ../pci/pci.h machdep.h
pcidev.o:	../pci/pcidev.c ../defs.h ../pci/pci.h machdep.h
pciprobe.o:	../pci/pciprobe.c ../defs.h ../pci/pci.h machdep.h
ppb.o:		../pci/ppb.c ../defs.h ../pci/pci.h machdep.h
cardbus.o:	../pci/cardbus.c ../defs.h ../pci/pci.h machdep.h
fakepci.o:	../pci/fakepci.c ../defs.h ../pci/pci.h machdep.h
decether.o:	../pci/decether.c ../defs.h ../pci/pci.h machdep.h

#
# file-system specific objects
#
fs.o:		../fs/fs.c ../defs.h ../fs/fs.h machdep.h
dospart.o:	../fs/dospart.c ../defs.h ../fs/fs.h ../fs/dos.h machdep.h
bsdpart.o:	../fs/bsdpart.c ../defs.h ../fs/fs.h machdep.h
bsdufs.o:	../fs/bsdufs.c ../defs.h ../fs/fs.h machdep.h
dosfat.o:	../fs/dosfat.c ../defs.h ../fs/fs.h machdep.h
ext2fs.o:	../fs/ext2fs.c ../defs.h ../fs/fs.h machdep.h
iso9660.o:	../fs/iso9660.c ../defs.h ../fs/fs.h machdep.h

#
# executable-image objects
#
aout.o:		../exe/aout.c ../defs.h ../exe/exe.h machdep.h
exe.o:		../exe/exe.c ../defs.h ../exe/exe.h machdep.h
loadfc.o:	../exe/loadfc.c ../defs.h ../exe/exe.h machdep.h
coff.o:		../exe/coff.c ../defs.h ../exe/exe.h machdep.h
elf.o:		../exe/elf.c ../defs.h ../exe/exe.h machdep.h
elf64.o:	../exe/elf64.c ../defs.h ../exe/exe.h machdep.h
gzip.o:		../exe/gzip.c ../defs.h ../exe/exe.h machdep.h
rawbin.o:	../exe/rawbin.c ../defs.h ../exe/exe.h machdep.h
