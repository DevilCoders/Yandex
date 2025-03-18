#!/bin/bash

set -e

DIR=$(realpath $(dirname $0))
BIN="$DIR/../../bin/sandboxctl $SANDBOXCTL_XARGS"

$BIN create -W -N RUN_SCRIPT -c cmdline='dmesg > dmesg.txt; lscpu > lscpu.txt' \
     -C '{"save_as_resource" : {"dmesg.txt":"OTHER_RESOURCE", "lscpu.txt": "OTHER_RESOURCE"}}' \

