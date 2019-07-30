#!/bin/bash
# Usage example: ./try-local.sh -b tables-trusty64 -b tables-win64
buildbot try --connect=pb --master=127.0.0.1:5555 --username=tables --passwd=tables --vc=git $@

