#!/bin/sh

for GROUP in $(cat $1); do
    echo "===> Cleaning $GROUP"
    tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
done
