#!/bin/bash
set -e
set -x
cd `dirname $0`

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 ampl dll"
    exit 1
fi
AMPL=$1
TABLE_HANDLER=$2

[ ! -e diet.dat.db ] && (sqlite3 diet.dat.db < diet.dat.sql)
$AMPL -i $TABLE_HANDLER diet-sql-2.run > output-2.txt
diff output-2.txt output-2-expected.txt
