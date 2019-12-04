#!/bin/bash
cd `dirname $0`
cd ..
mkdir -p build
cd build

source ~/.bashrc
cmake ..
