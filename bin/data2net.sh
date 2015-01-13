#!/bin/bash

binpath=`dirname $0`

if [ $# -lt 4 ]; then
    echo Usage: data2net.sh vdfile datafile score resultdir [options] 1>&2
    exit 1
fi

vdfile=$1;    shift 
datafile=$1;  shift
score=$1;     shift
rdir=$1;      shift

mkdir -p $rdir
nof_vars=`cat $vdfile | wc -l`

logregfile=$bin/logreg256x2000.bin
$binpath/get_local_scores $vdfile $datafile $score ${rdir}/res -l $logregfile $@
$binpath/split_local_scores   $nof_vars ${rdir}
$binpath/reverse_local_scores $nof_vars ${rdir}
$binpath/get_best_parents     $nof_vars ${rdir}
$binpath/get_best_sinks       $nof_vars ${rdir} ${rdir}/sinks
$binpath/get_best_order       $nof_vars ${rdir}/sinks ${rdir}/ord
$binpath/get_best_net         $nof_vars ${rdir} ${rdir}/ord ${rdir}/net
$binpath/score_net            ${rdir}/net ${rdir}
