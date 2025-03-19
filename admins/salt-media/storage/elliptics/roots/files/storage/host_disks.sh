#!/bin/bash

# Print all non-shelf and non-raid block devices in system

all_disks=`mktemp`
shelf_disks=`mktemp`

lsblk -o NAME,TYPE | grep disk | awk '{print $1}' | sort -u > $all_disks
(for s in `sudo find_shelves`; do
    shelf_disks $s | awk '{print $NF}'
done | sed 's#/dev/##'; grep md /proc/mdstat | grep -Po 'sd[a-z]+[0-9]?\[[0-9]]' | sed 's/[0-9].*//') | sort -u > $shelf_disks

comm -23 $all_disks $shelf_disks

rm $all_disks $shelf_disks
