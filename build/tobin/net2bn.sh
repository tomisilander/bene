#!/bin/bash

if [ $# -ne 1 ]; then
    echo Usage: $0 netfile
    exit 1
fi

bindir=`dirname $0`
wc -l < $1
$bindir/net2parents $1 - | $bindir/parents2arcs - -
