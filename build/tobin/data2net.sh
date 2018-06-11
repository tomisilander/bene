#!/bin/bash

binpath=`dirname $0`

if [ $# -lt 4 ]; then
    echo Usage: data2net.sh [-z -s selfile] vdfile datafile score resultdir [options] 1>&2
    exit 1
fi

zeta=0
selfile=""
while getopts "zs:" opt; do
	case $opt in
		z)
			zeta=1
			shift
			;;
		s)
			selfile=$OPTARG
			selarg="-s $selfile"
			shift 2
			;;
	esac
done

vdfile=$1;    shift
datafile=$1;  shift
score=$1;     shift
rdir=$1;      shift

mkdir -p $rdir
if [ -f "$selfile" ]; then
	nof_vars=`cat $selfile | wc -l`
else
	nof_vars=`cat $vdfile | wc -l`
fi

logregfile=$binpath/logreg256x2000.bin
$binpath/get_local_scores $vdfile $datafile $score ${rdir}/res -l $logregfile $@ $selarg
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
