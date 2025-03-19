#!/bin/bash

empty_dirs=$(find /data -maxdepth 3 -type d -empty -regex '/data.*/[a-z0-9.-]+\.yandex\.net$' -regextype posix-egrep 2>/dev/null)

for empty_dir in $empty_dirs
do
    echo "Removing empty dir $empty_dir"
    rm -rf "$empty_dir"
done
