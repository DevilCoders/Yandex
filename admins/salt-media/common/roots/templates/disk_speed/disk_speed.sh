#!/bin/bash
#
# Provides: disk_speed

me=${0##*/}    # strip path
me=${me%.*}    # strip extension


die() {
    echo "$1;$2"
    exit 0
}

speedmin=200
speedminname=none
speedminthreshold=50

for i in `ls /var/tmp/disk_speed/disk_speed_* 2>/dev/null`
do
    speed=`cat $i | awk 'BEGIN { FS = "." } ; { print $1 }'`;
    if [ -z "$speed" ]; then 
        speed=$speedmin
    fi
    if [ "$speed" -lt "$speedmin" ]; then
        speedmin=$speed
        speedminname=$i
    fi

done


if [ "$speedmin" -lt $speedminthreshold ]; then
    die "1" "Bad disk speed $speedmin in $speedminname";
else
    die "0" "Disk speed for all tested disks is OK";
fi
exit 0;
