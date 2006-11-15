#!/bin/sh

set -e
set -x

CMD="export"
[ -n "$1" ] && CMD="$*"

CGT=`pwd`
export CGT

PROC="`uname -p`"
export PROC

if [ ! -d /cgt ]; then
    mkdir /cgt
fi

if [ ! -d /cgt/bin ]; then
    mkdir /cgt/bin
fi

if [ ! -d /cgt/lib ]; then
    mkdir /cgt/lib
fi

if [ ! -d /cgt/include ]; then
    mkdir /cgt/include
fi

(cd bin/ccgen; make $CMD)

if [ "$PROC" = "sparc" ]; then
    (cd lib/cgt; make -f Makefile.sun $CMD)
elif [ "$PROC" = "powerpc" ]; then
    (cd lib/cgt; make -f Makefile.osx $CMD)
else
    (cd lib/cgt; make $CMD)
fi

(cd lib/integer; make $CMD)

if [ "$PROC" = "powerpc" ]; then
	rm -f lib/aoa/Makefile; ln -s makefile.osx lib/aoa/Makefile
else
	rm -f lib/aoa/Makefile; ln -s makefile.fbsd lib/aoa/Makefile
fi

(cd lib/floatc; make $CMD)
(cd lib/fmalloc; make $CMD)
(cd lib/aoa; make $CMD)
(cd lib/wfmalloc; make $CMD)
(cd bin/wacco; make $CMD)
(cd bin/cc; make $CMD)
(cd lib/gcmalloc; make $CMD)
(cd lib/wgmalloc; make $CMD)

# (cd lib/sym; make $CMD)
# (cd lib/clib; make $CMD)
# (cd lib/edit; make $CMD)
