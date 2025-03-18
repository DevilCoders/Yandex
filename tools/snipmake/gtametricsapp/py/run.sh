#!/bin/bash

cd `dirname $0`

function run
{
    local log="log_$1.txt"
    python gtametrics.py -t $1 > $log 2>&1
    if [ ! $? -eq 0 ]; then
        echo "Build for $1 failed"
    fi
}

for i in "ru" "ua" "by" "kz" "tr" ; do
    run ${i} &
done

wait
