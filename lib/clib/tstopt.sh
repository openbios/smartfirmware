#!/bin/sh
# getoptx test

./tstopt -a -bb -c +foo +FOO --bar foobar -0 --1 -debug -debuglev 3 -? \
	+x --yy +z stuff | diff - tstopt.good > tst.$$ 2>&1

if [ -s tst.$$ ]; then
    echo getoptx test failed
    cat tst.$$
else
    echo getoptx test passed
fi

rm -f tst.$$
exit 0
