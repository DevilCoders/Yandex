#!/bin/bash
>&2 echo "Starting..."

#snippets_contexts/<table>
TABLE=$1
#./mr_diffctx.sh
SCRIPT=$2
#-e exp -E refexp
EXP=$3
timestamp() {
    date +"%s%N"
}
TEMP_TABLE=$TABLE."$(timestamp)"
>&2 echo "TEMP_TABLE:$TEMP_TABLE"
echo "$EXP" > exp.txt
>&2 echo "Rinning map reduce jobs..."
export MR_USER=tmp
mapreduce -cpuintensive --maxjobfails 1 -subkey -map $SCRIPT -src $TABLE -dst $TEMP_TABLE -file $SCRIPT -file exp.txt -file ./csnip
>&2 echo "Getting answer from MapReduce..."
mr_cat -sub $TEMP_TABLE
mr_rm $TEMP_TABLE


