#!/bin/bash
#*.hamsterwheel
QUERY_FILE=$1
#snippets_contexts
MR_TABLE=$2

export MR_USER=tmp
BATCH_SIZE=5000
for TO in {0..1000000..5000}
do
    echo $TO
    time ionice -c 3 head -n $TO $QUERY_FILE | tail -n $BATCH_SIZE | hamsterwheel -j 8 -e pron=snipfactors -f -q | csnip -r mr -j 8 | mapreduce -subkey -append -write $MR_TABLE
done
