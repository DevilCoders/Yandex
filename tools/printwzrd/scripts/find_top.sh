#!/bin/sh
# We use dot in floats, so set LC_ALL=C.
if [ $# -ne 2 ]
then
    echo 1>&2 "Usage: $0 SOURCE_LOG COUNT"
    echo 1>&2 "The result is written to stdout."
    exit 1
fi
BASE_DIR=$(dirname $0)
# We use dot in floats, so set LC_ALL=C.
$BASE_DIR/filter_lemmer_logs.py $1 |
    LC_ALL=C sort |
    $BASE_DIR/group_records.py |
    LC_ALL=C sort -t "	" -k2,2nr |
    head -n $2 | cut -f 1
