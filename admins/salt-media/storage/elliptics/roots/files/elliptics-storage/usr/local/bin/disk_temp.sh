#!/bin/bash
#
# Provides: disk_temp

me=${0##*/}    # strip path
me=${me%.*}    # strip extension


die() {
    echo "$1;$2"
    exit 0
}

tempmax=30
tempmaxname=none
tempmaxthreshold=50
# Dannye zapolnyautsya iz /etc/cron.d/sensors.sh
for i in `ls /var/tmp/disk_temp/disk_temp_* 2>/dev/null`
do
    temp=`cat $i | awk 'BEGIN { FS = "." } ; { print $1 }'`;
    if [ -z "$temp" ]; then 
        temp=$tempmax
    fi
    if [ "$temp" -eq "$temp" ] 2>/dev/null
    then
        if [ "$temp" -gt "$tempmax" ]; then
            tempmax=$temp
            tempmaxname=$i
        fi
    fi
done


if [ "$tempmax" -gt $tempmaxthreshold ]; then
    die "1" "Bad disk temperature $tempmax in $tempmaxname";
else
    die "0" "Disk temp for all disks is OK";
fi
exit 0;
