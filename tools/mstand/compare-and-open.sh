#!/bin/sh
set -e

tmpfile=$(mktemp -t mstand_compare_XXXXXX.html)
echo "Saving to: ${tmpfile}"

./compare-metrics.py $@ --output-html-vertical "${tmpfile}"
xdg-open "${tmpfile}"
sleep 1

rm "${tmpfile}"
echo "Removed ${tmpfile}"
