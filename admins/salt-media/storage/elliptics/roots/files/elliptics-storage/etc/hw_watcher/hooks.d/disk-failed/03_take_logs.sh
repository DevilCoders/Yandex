#!/bin/bash

PATH="${PATH}:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/sbin:/usr/local/bin"

cmd=$@
disk=$1
name=$2
problem_code=$3
shelf=$4
slot=$5
partition_list=$6

# Диск отсутствует в системе. Пока переходим в UNKNOWN
if [ "$disk" != "None" ]
then
    name=${disk/\/dev\//}
    if grep -q 'ST10000NM0016' /sys/block/$name/device/model
    then
        if [ ! -d /var/tmp/seagate_logs ]; then mkdir -p /var/tmp/seagate_logs; fi
        cd /var/tmp/seagate_logs || echo "No such dir"
        /usr/local/bin/SeaDragon_Logs_Utility -d $disk --sm2
        /usr/local/bin/SeaDragon_Logs_Utility -d $disk --uds
    else
        cat /sys/block/$name/device/model
    fi
fi
exit 0
