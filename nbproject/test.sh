#!/bin/bash
cd `dirname $0`
cd ../build

source ~/.bashrc
ctest --output-on-failure
