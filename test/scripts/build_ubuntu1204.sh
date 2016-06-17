#!/bin/bash -x

set -e
set -o pipefail

: ${LLVM_VERSION:?"LLVM_VERSION must be specified"}
: ${BUILD_SYSTEM:?"BUILD_SYSTEM must be specified"}

# Set variables that the build script needs
export LLVM_INCLUDE="/usr/local/llvm/include"
export LLVM_LIB="/usr/local/llvm/lib"
export LLVM_BIN="/usr/local/llvm/bin"
# Travis has 2 CPUs but only 3GiB of RAM so we need
# to avoid doing stuff in parallel to avoid the linker getting killed
export NUM_JOBS=1
export RUN_TESTS=1

# Have the generic script to the real work
test/scripts/build_generic.sh
