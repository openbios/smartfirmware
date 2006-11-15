
tests="memtest bitset buftest hashtest pooltest qtest cachetst stdtest"
#tests="memtest bitset hashtest pooltest qtest cachetst stdtest"
#tests="buftest"

rm -f zero
touch zero
errs=0

for t in $tests; do
    if [ -x /gnu/bin/stub.exe ]; then
	if [ $t -nt $t.exe -o ! -x $t.exe ]; then
	    cp /gnu/bin/stub.exe .
	    mv $t $t.tmp
	    cp $t.tmp $t
	    aout2exe $t
	    mv $t.tmp $t
	fi
    fi

    if [ -r $t.in ]; then
    	./$t <$t.in >$t.out 2>&1
    else
    	./$t <zero >$t.out 2>&1
    fi

    if [ ! -f $t.std ]; then
	cp $t.out $t.std
    fi

    diff $t.std $t.out >$t.dif

    if [ -s $t.dif ]; then
    	(( errs = $errs + 1 ))
	echo "$t: FAILED"
    else
	echo "$t: passed"
	rm $t.dif
    fi
done

exit $errs
