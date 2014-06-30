#!/bin/bash

d=`dirname $0`

if [ $# -eq 1 ]; then
	ess=$1
else
    ess=1
fi

score=`$d/../bin/data2net.sh $d/iris.vd $d/iris.idt $ess $d/resdir`
echo Score : $score
echo Acs : 
$d/../bin/net2parents $d/resdir/net - | $d/../bin/parents2arcs - -

if which dot > /dev/null; then
    $d/../bin/net2parents $d/resdir/net - \
	| $d/../bin/parents2arcs - - \
	| $d/../bin/arcs2dot $d/iris.vd - - \
	| dot -Tpng -o $d/resdir/iris.png
    echo See $d/resdir/iris.png for a png picture of the net.
fi
