#!/bin/bash
set -ex

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 [32|64] <package version>"
elif [ -d "/base/manylinux/" ]; then
    BUILD_DIR=/base/manylinux/linux$1/
    rm -rf $BUILD_DIR
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cp -r /base /tmp/tables

    export PATH=/opt/python/cp27-cp27m/bin/:${PATH}
    export CTEST_OUTPUT_ON_FAILURE=1
    export PATH=$PATH:/tmp/tables/ampl/
    cmake /tmp/tables/ -DARCH=$1 -DPACAKGE_VERSION=${2:-YYYYMMDD} -DGENERATE_ARITH=1
    make all -j4
    make test
    cpack
else
    echo "Must be run inside a docker container."
fi
