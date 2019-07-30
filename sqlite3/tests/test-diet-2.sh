#!/bin/bash
set -e
set -x
cd `dirname $0`
[ ! -e diet.dat.db ] && (sqlite3 diet.dat.db < diet.dat.sql)
ampl -i ../../sqlite3.dll diet-sql-2.run > output-2.txt
diff output-2.txt output-2-expected.txt
