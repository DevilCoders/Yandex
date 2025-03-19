#!/usr/bin/env bash

prefix="https://www.redhat.com/archives/rhsa-announce/"
suffix=".txt.gz"
extract="gzip"
parse="../rhel.py"

now=$(date +%F)

mkdir -p rhel
(
cd rhel || exit 1

# Only 10 years due to EL Life Cycle limitations
# Unfortunately Yandex does not purchase ELP for RHEL
for i in $(seq 0 120)
do
    name=$(date +%Y-%B -d "$now -$i month")
    wget --header="accept-encoding: gzip" -N "$prefix$name$suffix" && ${extract} -cdfq "$name$suffix" > "$name".txt && ${parse} "$name".txt > "$name".json
done
)

exit 0
