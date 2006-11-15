# Cogent CSB238 EP7312 ARM eval board Utilities
#
OPTS=-O

# cross-hosting gcc
#
COMP=arm-unknown-elf
CC= $(COMP)-gcc
LD= $(COMP)-ld
AS=	$(COMP)-as
SIZE=	$(COMP)-size

CFLAGS= -Wall -I- -I. $(OPTS) -fsigned-char -fno-builtin -fpack-struct

PROGS=	chello showkeys showdevs showboard showticks dofault takemmu linuxload \
	smedia take2

all:	$(PROGS)
 
#
# client-interface demo program
#
chello:	chello.o sfclient.o 
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map chello.map -o chello \
			chello.o sfclient.o
	cp chello /tftpboot

chello.o: chello.c sfclient.h
sfclient.o: sfclient.c sfclient.h

chello.c:	../../misc/chello.c
	cp ../../misc/chello.c chello.c

#
#	Keypad input test program
#
showkeys:	showkeys.o sfclient.o 
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map showkeys.map -o showkeys \
			showkeys.o sfclient.o
	cp showkeys /tftpboot

#
#	Show devices demo program
#
showdevs:	showdevs.o sfclient.o 
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map showdevs.map -o showdevs \
			showdevs.o sfclient.o
	cp showdevs /tftpboot

showdevs.o: showdevs.c sfclient.h

#
#	Board information demo program
#
showboard:	showboard.o sfclient.o 
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map showboard.map -o showboard \
			showboard.o sfclient.o
	cp showboard /tftpboot

showboard.o: showboard.c sfclient.h

#
#	Clock tick demo program
#
showticks:	showticks.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map showticks.map -o showticks \
			showticks.o sfclient.o stdlib.o
	cp showticks /tftpboot

showticks.o: showticks.c sfclient.h

#
#	Fault generation program
#
dofault:	dofault.o sfclient.o 
	$(LD) -s -N -Ttext 0xF0010000 -e sf_start -Map dofault.map -o dofault \
			dofault.o sfclient.o
	cp dofault /tftpboot

dofault.o: dofault.c sfclient.h

#
#	Take over MMU demo program
#
takemmu:	takemmu.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0xF0100000 -e sf_start -Map takemmu.map -o takemmu \
			takemmu.o sfclient.o stdlib.o
	cp takemmu /tftpboot

takemmu.o: takemmu.c sfclient.h

#
#	Take over MMU demo program, sleazy EDB7312 hack
#
take2:	take2.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0xF0100000 -e sf_start -Map take2.map -o take2 \
			take2.o sfclient.o stdlib.o
	cp take2 /tftpboot

take2.o: take2.c sfclient.h

#
#	Load a linux kernel image into memory and branch to it
#
linuxload:	linuxload.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0xC0F00000 -e sf_start -Map linuxload.map -o linuxload \
			linuxload.o sfclient.o stdlib.o
	cp linuxload /tftpboot
	../../inno/mapsort.sh linuxload.map >linuxload.map.sorted

linuxload.o: linuxload.c sfclient.h

#
#	Test SmartMedia card
#
smedia:	smedia.o nandflsh.o sfclient.o stdlib.o
	$(LD) -s -N -Ttext 0xF0100000 -e sf_start -Map smedia.map -o smedia \
			smedia.o nandflsh.o sfclient.o stdlib.o
	cp smedia /tftpboot

smedia.o: smedia.c sfclient.h
nandflsh.o: nandflsh.c sfclient.h

stdlib.o: stdlib.c

clean:
	rm -f *.o $(PROGS) *.map


