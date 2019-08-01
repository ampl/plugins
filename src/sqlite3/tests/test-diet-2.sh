#!/bin/bash
set -x
cd `dirname $0`

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 ampl dll"
    exit 1
fi
AMPL=$1
TABLE_HANDLER=$2

[ ! -e diet.dat.db ] && (sqlite3 diet.dat.db < diet.dat.sql)
$AMPL -i $TABLE_HANDLER diet-sql-2.run 2>&1 > output-2.txt
if [ $? -ne 0 ]; then
    cat output-2.txt
    exit 1
fi
set -e
diff output-2.txt output-2-expected.txt
