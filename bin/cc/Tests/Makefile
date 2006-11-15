CPPTST = ../cpptst
CCTST = ../cctst
MACINIT = ../mac.init

all: cpptests ctests cxxtests

cpptests:
	for f in *.h; do echo $$f; ${CPPTST} $$f 2>&1 | diff $$f-good - || exit 5; done

ctests:
	for f in *.c; do echo $$f; ${CCTST} $$f || exit 1; done
	for f in mac/*.c; do echo $$f; ${CCTST} -init ${MACINIT} $$f || exit 11; done

cxxtests:
	for f in *.cc; do echo $$f; ${CCTST} -classes $$f || exit 2; done
