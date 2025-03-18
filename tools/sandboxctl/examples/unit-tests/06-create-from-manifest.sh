#!/bin/bash

set -e

DIR=$(realpath $(dirname $0))
BIN="$DIR/../../bin/sandboxctl $SANDBOXCTL_XARGS"
manifest=$(mktemp /tmp/06-create-from-manifest-XXXXX.yaml)

$BIN create -W -N RUN_SCRIPT \
       -c cmdline='dmesg > dmesg.txt; lscpu > lscpu.txt' \
       -C '{"save_as_resource" : {"dmesg.txt":"OTHER_RESOURCE", "lscpu.txt": "OTHER_RESOURCE"}}' \
       --inject-body '{"requirements" : {"cores": "8"}}' \
       -o $manifest  --dry-run \

cmp $DIR/06-create-from-manifest.yaml $manifest
$BIN create -W -i $manifest
unlink $manifest
