#!/usr/bin/env bash

prefix="https://lists.ubuntu.com/archives/ubuntu-security-announce/"
suffix=".txt.gz"
extract="gzip"
parse="../usn.py"

now=$(date +%F)

mkdir -p ubuntu
(
cd ubuntu || exit 1

# Only 5 years due to Ubuntu LTS Life Cycle limitations
for i in $(seq 0 60)
do
    name=$(date +%Y-%B -d "$now -$i month")
    wget -N "$prefix$name$suffix" && ${extract} -cdfq "$name$suffix" > "$name".txt && ${parse} "$name".txt > "$name".json
done
)

exit 0
