#!/bin/sh

for GROUP in $(cat recluster2/126-vla/groups.txt); do
    echo "===> Cleaning $GROUP"
    ./tools/recluster/main.py -a recluster -g ${GROUP} -c cleanup
done
