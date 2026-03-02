#!/bin/bash

# navigate to the script directory so the same commands work from
# either the build directory or the repository root.
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR" || exit 1

# remove objects and symlinks; leave real sources alone
rm -f *.o
# the build directory holds symlinks to source files, remove them if present
rm -f *.c *.h
