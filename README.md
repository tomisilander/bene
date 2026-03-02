# bene

This package contains software for constructing  the globally optimal
Bayesian network structure using decomposable scores AIC, BIC, BDe,
fNML, qNML and LOO. 

## Requirements

bash, gcc.
If you also have graphviz package (thus dot) you can get images of network
structures.

## Compile

Say `build/build.sh`. This will build necessary executables to bin directory.

## This is how the package is used

Run and study example/run.sh and then probably bin/data2net.sh,
You may also take a look at util/data2net.py

### Regression tests

A simple regression harness is provided under `tests/regression/`.
It verifies that the various binaries execute successfully and that
results remain unchanged over time.

To generate or update the baseline data run:

```sh
cd tests/regression
./run_regression.sh -s
```

This will invoke `data2net.sh` for every dataset in `tests/data` and
for each scoring criterion (BIC, AIC, HQC, fNML, qNML, LOO, BDq, BDe).
The script saves the `stdout` output and an ASCII representation of the
learned network in `tests/regression/expected/<dataset>/<score>`.

Once a baseline exists you can run the suite without `-s` to perform
comparisons:

```sh
cd tests/regression
./run_regression.sh
```

Any discrepancies between current output and the saved baseline are
reported as failures.  This is useful for catching regressions when
modifying the codebase.

The harness only requires the same set of binaries listed earlier in
this document; missing tools cause datasets to be skipped but do not
stop the test.


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
