#!/bin/bash

backends1=$(mktemp)
backends2=$(mktemp)

elliptics-node-info.py | grep -Po '(?<=backend_id=)\d+' | sort -n | sudo tee $backends1 > /dev/null
ubic reload elliptics
grep backend_id /etc/elliptics/parsed/elliptics-node-1.parsed | awk '{print $NF}' | tr -d , | sort -n | sudo tee $backends2 > /dev/null
echo $(diff -u $backends1 $backends2 | grep -P '^\+[0-9]+' | tr -d + | grep -v 44851)
