#!/bin/bash

mode=$1

if [ -z $mode ]; then
    echo "Usage: $0 [write|rw]"
    exit 0
fi

for d in /srv/storage/*[0-9]; do
    echo "[$d]"
    echo "filename=$d/test"
    echo "runtime=86400"
    echo "readwrite=$mode"
    if [ $mode = "write" ]; then
        echo "fill_device=1"
    fi
done
