#!/bin/bash
set -e
set -x
cd `dirname $0`
[ ! -e diet.dat.db ] && (sqlite3 diet.dat.db < diet.dat.sql)
ampl -i ../../sqlite3.dll diet-sql-1.run > output-1.txt
diff output-1.txt output-1-expected.txt
