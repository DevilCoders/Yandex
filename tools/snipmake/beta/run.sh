#!/usr/local/bin/bash

set -e
set -o pipefail

SH=`hostname -s`
BP=$2

cat shards | grep "^$SH" | wc -l > num_processes
cat shards | grep "^$SH" | awk '{print $2}' | while read db
do
    mkdir $BP; cd $BP;
    daemon -f -p pid ../$1 -d -p $BP -V IndexDir=$db -V LoadLog=/dev/null -V PassageLog=/dev/null ../$3
    sleep 20
    set +e
    fetch -o test.html http://localhost:$BP/yandsearch?text=test >/dev/null 2>/dev/null </dev/null
    set -e
    if [ ! -f test.html ]
    then
        echo "basesearch failed to start on port $BP" >&2
    fi
    cd ..;
    BP=$(($BP + 1));
done
