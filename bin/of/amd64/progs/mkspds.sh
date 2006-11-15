#!/bin/sh

SPDS="`cd spd-src; ls`"

for spd in $SPDS; do
    echo $spd:
    ./spdcomp spd-src/$spd >spds/$spd
done
