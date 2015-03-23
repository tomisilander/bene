#!/bin/bash

binpath=`dirname $0`

if [ $# -lt 4 ]; then
    echo Usage: data2net.sh [-z] vdfile datafile score resultdir [options] 1>&2
    exit 1
fi

zeta=0
while getopts z opt; do
	if [ $opt = z ]; then
		zeta=1
		shift
	fi
done

vdfile=$1;    shift 
datafile=$1;  shift
score=$1;     shift
rdir=$1;      shift

mkdir -p $rdir
nof_vars=`cat $vdfile | wc -l`

logregfile=$binpath/logreg256x2000.bin
$binpath/get_local_scores $vdfile $datafile $score ${rdir}/res -l $logregfile $@
$binpath/split_local_scores   $nof_vars ${rdir}
$binpath/reverse_local_scores $nof_vars ${rdir}
if [ $zeta -eq 1 ]; then
    echo ZETAING
    $binpath/zeta_local_scores $nof_vars ${rdir}
fi
$binpath/get_best_parents     $nof_vars ${rdir}
$binpath/get_best_sinks       $nof_vars ${rdir} ${rdir}/sinks
$binpath/get_best_order       $nof_vars ${rdir}/sinks ${rdir}/ord
$binpath/get_best_net         $nof_vars ${rdir} ${rdir}/ord ${rdir}/net
$binpath/score_net            ${rdir}/net ${rdir}
