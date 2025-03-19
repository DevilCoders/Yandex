#!/usr/bin/env bash

CONFIG_NAME="$1"
CLUSTER="$2"

if [ -z "$CONFIG_NAME" ] || [ -z "$CLUSTER" ]
then
    echo "usage: ./inspect.sh CONFIG_NAME CLUSTER, e.g. ./inspect.sh storage cloud_preprod_compute_sas"
    exit 1
fi

pssh run -p 20 "cat /Berkanavt/nbs-server/cfg/nbs-$CONFIG_NAME.txt" "C@$CLUSTER" > tmp.txt &&
cat tmp.txt | ./process_pssh_output.py
