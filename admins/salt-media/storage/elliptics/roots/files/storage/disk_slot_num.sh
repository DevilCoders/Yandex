#!/bin/bash

# 1 - device name
# 2 - anything, if empty it is verified that the disk is not in a shelf

disk=${1/\/dev\//}

if [ -z "$disk" ]; then
    echo "Usage: $0 /dev/sdX"
    exit 1
fi

if [ -z "$2" ]; then
    # Shelf_tool is too slow to use, but this is a sanity check which probably should be enabled by default
    if $(shelf_tool | grep -w "$disk"); then
        echo "Disk $1 is in a shelf"
        exit 1
    fi
fi

first_slot=$(
    for d in /sys/class/ata_port/ata*; do
        if [ $(find $d/device/host* -maxdepth 1 -name 'target*' | wc -l) -gt 0 ]; then
            echo ${d/\/sys\/class\/ata_port\/ata/}
        fi
     done | sort -n | head -1
);

readlink /sys/block/$disk/device | awk -F / '{print $NF}' | cut -f 1 -d :
