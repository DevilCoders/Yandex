#!/bin/bash
#
# Provides: disk_temp

me=${0##*/}    # strip path
me=${me%.*}    # strip extension


die() {
    echo "$1;$2"
    exit 0
}

if [ -f /etc/monitoring/disk_temp.conf ]; then
    . /etc/monitoring/disk_temp.conf
fi

temp_threshold=${temp_threshold:-52}
max_warm_disks=${max_warm_disks:-3}

count=0
disks=()

for d in /dev/sd*[a-z]
do
    temp=`hddtemp -q -n $d 2>/dev/null`;
    if [ -z "$temp" ]; then 
        continue;
    fi
    if [ "$temp" -ge "$temp_threshold" ]; then
        count=$(($count+1))
        disks+=($d)
    fi
done

if [ "$count" -gt $max_warm_disks ]; then
    code=2
elif [ "$count" -gt 0 ]; then
    code=1
else
    code=0
fi

if [ "$count" -gt 5 ]; then
    die "$code" "Temperature of $count disks is >= $temp_threshold C";
elif [ "$count" -gt 0 ]; then
    die "$code" "Temperature of disks ${disks[*]} is >= $temp_threshold C";
else
    die "$code" "Disk temp for all disks is OK";
fi
exit 0;
