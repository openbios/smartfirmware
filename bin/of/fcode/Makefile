all: if_em.fc if_em_674tx.rom if_em_674.rom if_em_676tx.rom if_em_676.rom if_wx.rom fcdisp.fc


fcdisp.fc: fcdisp.4th
	of -t fcdisp.4th fcdisp.fc
	of -f in | cat
	of -d fcdisp.fc > fcdisp.detok

fcdisp.4th: fcdisp.c fcode.h fcpci.h /cgt/bin/cc-fcode
	cc-fcode -g -DSTANDALONE fcdisp.c > fcdisp.4th


# customize these for the WX driver

WX_VENDOR=8086
WX_DEVICE=1004
WX_CLASS=20000
WX_SUBSYS=0
WX_SUBSYSVEND=0
WX_REVISION=1

if_wx.4th: if_wx.c fcode.h fcpci.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -g -DVENDOR_ID=\"$(WX_VENDOR)\" \
		-DDEVICE_ID=\"$(WX_DEVICE)\"	\
		-DSUBSYSTEM_ID=\"$(WX_SUBSYS)\"	\
		-DSUBSYSTEM_VENDOR_ID=\"$(WX_SUBSYSVEND)\"	\
		-I../fcode if_wx.c > if_wx.4th || ( rm -f if_wx.4th && return 1 )

if_wx.fc: if_wx.4th
	of -t if_wx.4th if_wx.fc
	of -d if_wx.fc > if_wx.detok

if_wx.bin: if_wx.fc ../misc/fcode2rom Makefile
	../misc/fcode2rom -v 0x$(WX_VENDOR) -d 0x$(WX_DEVICE) \
		-c 0x$(WX_CLASS) -r 0x$(WX_REVISION) \
		-y "Copyright (c) 2001 by CodeGen, Inc.  All Rights Reserved." \
		-R if_wx.fc >if_wx.bin
	hd if_wx.bin >if_wx.dump

if_wx.rom:	if_wx.bin
	../misc/pad -i 0x10000 -b 0xFF 0x40000 if_wx.bin >if_wx.rom


# generic all-device test version of EM driver
#
if_em.4th: if_em.c fcode.h fcpci.h eeprom.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -g -I../fcode if_em.c > if_em.4th || \
			( rm -f if_em_674tx.4th && return 1 )

if_em.fc: if_em.4th
	of -t if_em.4th if_em.fc
	of -d if_em.fc > if_em.detok

# customize these for each version of the EM driver

# all RAMIX -82544 and -82546 boards
EM_VENDOR=8086
EM_544_DEVICE=1008
EM_546_DEVICE=1010
EM_CLASS=20000
EM_544_REVISION=2
EM_546_REVISION=1
EM_MANUFACTURER="RAMIX"

# RAMIX copper quad-82544 board
if_em_674tx.4th: if_em.c fcode.h fcpci.h eeprom.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -DDEVICE_ID=$(EM_544_DEVICE)	\
		-DSUBSYSTEM_VENDOR_ID=8086	\
		-DSUBSYSTEM_ID=1008	\
		-DREVISION=$(EM_544_REVISION)	\
		-DMANUFACTURER=\"$(EM_MANUFACTURER)\"	\
		-DBOARD_TYPE=\"RM674TX\"	\
		-I../fcode if_em.c > if_em_674tx.4th || \
			( rm -f if_em_674tx.4th && return 1 )

if_em_674tx.fc: if_em_674tx.4th
	of -t if_em_674tx.4th if_em_674tx.fc
	of -d if_em_674tx.fc > if_em_674tx.detok

if_em_674tx.bin: if_em_674tx.fc ../misc/fcode2rom Makefile
	../misc/fcode2rom -v 0x$(EM_VENDOR) -d 0x$(EM_544_DEVICE) \
		-c 0x$(EM_CLASS) -r 0x$(EM_544_REVISION) \
	-y "Copyright (c) 2002-2003 by CodeGen, Inc.  All Rights Reserved." \
		-R if_em_674tx.fc >if_em_674tx.bin
	hd if_em_674tx.bin >if_em_674tx.dump

if_em_674tx.rom:	if_em_674tx.bin
	../misc/pad -i 0x10000 -b 0xFF 0x80000 if_em_674tx.bin >if_em_674tx.rom

# RAMIX fibre quad-82544 board
if_em_674.4th: if_em.c fcode.h fcpci.h eeprom.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -DDEVICE_ID=$(EM_544_DEVICE)	\
		-DSUBSYSTEM_VENDOR_ID=140B	\
		-DSUBSYSTEM_ID=0670	\
		-DREVISION=$(EM_544_REVISION)	\
		-DMANUFACTURER=\"$(EM_MANUFACTURER)\"	\
		-DBOARD_TYPE=\"RM674\"	\
		-I../fcode if_em.c > if_em_674.4th || \
			( rm -f if_em_674.4th && return 1 )

if_em_674.fc: if_em_674.4th
	of -t if_em_674.4th if_em_674.fc
	of -d if_em_674.fc > if_em_674.detok

if_em_674.bin: if_em_674.fc ../misc/fcode2rom Makefile
	../misc/fcode2rom -v 0x$(EM_VENDOR) -d 0x$(EM_544_DEVICE) \
		-c 0x$(EM_CLASS) -r 0x$(EM_544_REVISION) \
	-y "Copyright (c) 2002-2003 by CodeGen, Inc.  All Rights Reserved." \
		-R if_em_674.fc >if_em_674.bin
	hd if_em_674.bin >if_em_674.dump

if_em_674.rom:	if_em_674.bin
	../misc/pad -i 0x10000 -b 0xFF 0x80000 if_em_674.bin >if_em_674.rom

# RAMIX copper 82546 board
if_em_676tx.4th: if_em.c fcode.h fcpci.h eeprom.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -DDEVICE_ID=$(EM_546_DEVICE)	\
		-DSUBSYSTEM_VENDOR_ID=140B	\
		-DSUBSYSTEM_ID=0676	\
		-DREVISION=$(EM_546_REVISION)	\
		-DMANUFACTURER=\"$(EM_MANUFACTURER)\"	\
		-DBOARD_TYPE=\"RM676TX\"	\
		-I../fcode if_em.c > if_em_676tx.4th || \
			( rm -f if_em_676tx.4th && return 1 )

if_em_676tx.fc: if_em_676tx.4th
	of -t if_em_676tx.4th if_em_676tx.fc
	of -d if_em_676tx.fc > if_em_676tx.detok

if_em_676tx.bin: if_em_676tx.fc ../misc/fcode2rom Makefile
	../misc/fcode2rom -v 0x$(EM_VENDOR) -d 0x$(EM_546_DEVICE) \
		-c 0x$(EM_CLASS) -r 0x$(EM_546_REVISION) \
	-y "Copyright (c) 2002-2003 by CodeGen, Inc.  All Rights Reserved." \
		-R if_em_676tx.fc >if_em_676tx.bin
	hd if_em_676tx.bin >if_em_676tx.dump

if_em_676tx.rom:	if_em_676tx.bin
	../misc/pad -i 0x10000 -b 0xFF 0x80000 if_em_676tx.bin >if_em_676tx.rom

# RAMIX fiber 82546 board
if_em_676.4th: if_em.c fcode.h fcpci.h eeprom.h \
		/cgt/bin/cc-fcode Makefile
	cc-fcode -DDEVICE_ID=$(EM_546_DEVICE)	\
		-DSUBSYSTEM_VENDOR_ID=140B	\
		-DSUBSYSTEM_ID=0676	\
		-DREVISION=$(EM_546_REVISION)	\
		-DMANUFACTURER=\"$(EM_MANUFACTURER)\"	\
		-DBOARD_TYPE=\"RM676TX\"	\
		-I../fcode if_em.c > if_em_676.4th || \
			( rm -f if_em_676.4th && return 1 )

if_em_676.fc: if_em_676.4th
	of -t if_em_676.4th if_em_676.fc
	of -d if_em_676.fc > if_em_676.detok

if_em_676.bin: if_em_676.fc ../misc/fcode2rom Makefile
	../misc/fcode2rom -v 0x$(EM_VENDOR) -d 0x$(EM_546_DEVICE) \
		-c 0x$(EM_CLASS) -r 0x$(EM_546_REVISION) \
	-y "Copyright (c) 2002-2003 by CodeGen, Inc.  All Rights Reserved." \
		-R if_em_676.fc >if_em_676.bin
	hd if_em_676.bin >if_em_676.dump

if_em_676.rom:	if_em_676.bin
	../misc/pad -i 0x10000 -b 0xFF 0x80000 if_em_676.bin >if_em_676.rom

eeprom.h:	mkeeprom eeprom
	./mkeeprom <eeprom >eeprom.h

mkeeprom:	mkeeprom.c
	cc -o mkeeprom mkeeprom.c

clean:
	rm -f *.4th *.fc *.detok *.bin *.rom *.dump mkeeprom
