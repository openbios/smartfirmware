# AMD Hammer Utilities
#
OPTS=-O

# cross-hosting gcc
#
COMP=x86_64-unknown-netbsd
CC= $(COMP)-gcc
LD= $(COMP)-ld
AS=	$(COMP)-as
SIZE=	$(COMP)-size

CFLAGS= -Wall -I- -I. $(OPTS) -fsigned-char -fno-builtin -fpack-struct -DINT64=long

PROGS=	sysinfo

all:	$(PROGS)
 
#
# Device and Memory map dummper
#
sysinfo:	sysinfo.o sfclient.o 
	$(LD) -s -N -Ttext 0x100000 -e sf_start -Map sysinfo.map -o sysinfo \
			sysinfo.o sfclient.o
	scp sysinfo flute:/tftpboot

sysinfot:	sysinfo.c
	cc -DINT64='long long' -DSTANDALONE -g -o sysinfot sysinfo.c sfclient.c

sysinfo.o: sysinfo.c sfclient.h
sfclient.o: sfclient.c sfclient.h

stdlib.o: stdlib.c

clean:
	rm -f *.o $(PROGS) *.map


