#!/bin/bash

dev=$1

if [ -z "$dev" -o ! -b "$dev" ]; then
    echo "Usage: $0 devname" >&2
    exit 1
fi

lsiutil -p 1 -a 16,0 | grep $(cat /sys/block/${dev##/dev/}/device/sas_address | cut -f 2 -d x) | awk '{print $6 " " $4}'
