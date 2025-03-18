#!/usr/bin/env sh

set -e

for GROUP in $(python groups.py --dump non_optimized); do
    echo Allocating $GROUP
    ./tools/recluster/main.py -a recluster -g ${GROUP} -c alloc_hosts,generate_intlookups
done
