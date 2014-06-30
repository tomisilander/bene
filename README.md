# bene

This package contains software for constructing  the globally optimal
Bayesian network structure using decomposable scores AIC, BIC, BDe,
fNML, and LOO. 

## Requirements

bash, gcc.
If you also have graphviz package (thus dot) you can get images of network
structures.

## Compile

Go to build directory and say ./build.sh


## This is how the package is used

Run and study example/run.sh and then probably bin/data2net.sh,
You may also take a look at util/data2net.py


## Input data format

(see example directory for an example)

vdfile
    value description file in which each line containis the variable name 
    and the names of the values all separated by tabulators.

datfile
    datafile for discretized data, data vectors on rows,  variables on columns
    values numerated from zero on (0, 1, 2, ..)


## DISCLAIMER

This work is anything but ready, tested etc. Use the source.

## Contact

tomi.silander@iki.fi
