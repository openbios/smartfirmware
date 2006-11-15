#
# Meta-build makefile - real builds are in sub-dirs
#
all: tag

None:
	@echo "Nothing to build here. Build something in one of the subdirectories."


#
# Handy operations to build archives, cleanup, etc
#
clean:
	cd unix; $(MAKE) $(MFLAGS) clean
	cd amd64; $(MAKE) $(MFLAGS) clean
	cd i386; $(MAKE) $(MFLAGS) clean
	cd fcode; $(MAKE) $(MFLAGS) clean
	cd misc; $(MAKE) $(MFLAGS) clean

tar:
	./mktarball -z ALL

files:
	@./mktarball FILES

tag:
	./mktags unix/*.[hcSs] *.[hc] pci/*.[hc] isa/*.[hc] scsi/*.[hc] \
			usb/*.[hc] fs/*.[hc] exe/*.[hc]
