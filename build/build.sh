#!/bin/bash

if [ ${1:-UNIX} == "WIN" ]; then
    CC="i586-mingw32msvc-gcc"
    EXT=".exe"
else
    CC=gcc
fi


#CFLAGS="-Wextra -g -ansi -pedantic"
#CFLAGS="-Wextra -g -ansi"
#CFLAGS="-Wall -O3 -g -pg"
CFLAGS="-Wall  -O3"

BINDIR=../bin
mkdir -p $BINDIR

ln -fs ../src/*.h ../src/*.c .

$CC $CFLAGS -c -o files.o files.c
$CC $CFLAGS -c -o varpar.o varpar.c
$CC $CFLAGS -c -o gopt.o gopt.c
$CC $CFLAGS -c -o xtab.o xtab.c

$CC $CFLAGS -o $BINDIR/get_local_scores$EXT files.c reg.c ilogi.c ls_XIC.c ls_fNML.c ls_BDe.c ls_LOO.c ls_qNML.c ls_BDq.c get_local_scores.c gopt.o xtab.o -lm
$CC $CFLAGS -o $BINDIR/split_local_scores$EXT split_local_scores.c files.o
$CC $CFLAGS -o $BINDIR/reverse_local_scores$EXT reverse_local_scores.c files.o
$CC $CFLAGS -o $BINDIR/zeta_local_scores$EXT zeta_local_scores.c files.o -lm
$CC $CFLAGS -o $BINDIR/get_best_parents$EXT get_best_parents.c files.o
$CC $CFLAGS -o $BINDIR/get_best_sinks$EXT get_best_sinks.c files.o
$CC $CFLAGS -o $BINDIR/get_best_order$EXT get_best_order.c
$CC $CFLAGS -o $BINDIR/get_best_net$EXT get_best_net.c files.o varpar.o
$CC $CFLAGS -o $BINDIR/score_net$EXT score_net.c files.o varpar.o
$CC $CFLAGS -o $BINDIR/score_nets$EXT score_nets.c files.o varpar.o
$CC $CFLAGS -o $BINDIR/score_netsets$EXT score_netsets.c files.o varpar.o -lm
$CC $CFLAGS -o $BINDIR/net2parents$EXT net2parents.c 
$CC $CFLAGS -o $BINDIR/parents2arcs$EXT parents2arcs.c
$CC $CFLAGS -o $BINDIR/arcs2dot$EXT arcs2dot.c
$CC $CFLAGS -o $BINDIR/ban_arc$EXT ban_arc.c files.o
$CC $CFLAGS -o $BINDIR/dump_scorefile$EXT dump_scorefile.c
$CC $CFLAGS -o $BINDIR/gen_prior_file$EXT gen_prior_file.c gopt.o -lm
$CC $CFLAGS -o $BINDIR/force_arc$EXT force_arc.c files.o

cp tobin/* $BINDIR
