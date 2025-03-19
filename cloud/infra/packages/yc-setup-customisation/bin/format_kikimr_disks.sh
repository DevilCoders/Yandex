#!/bin/sh

retry() {
        local n=0
        local try=$1
        local sleep_time=$2
        local cmd="${@: 3}"

        until [[ $n -ge $try ]]
        do
                $cmd && break || {
                        echo "Command Failed.."
                        n=$((n+1))
                        echo "retry $n ::"
                        sleep $sleep_time
                }
        done

        if [[ $n -eq $try ]]
        then 
                echo "Command still failed, exitting"
                exit 1
        fi
}

echo "Formating kikimr disks"
retry 10 5 /sbin/partprobe
retry 10 5 /bin/udevadm settle
DISK_PATH="/dev/disk/by-partlabel/"
KIKIMR_DISKS=$(ls ${DISK_PATH}|grep KIKIMR ||:)
if [[ ! -z ${KIKIMR_DISKS} ]]
then
        for disk in ${KIKIMR_DISKS}; do
                echo "Formating ${DISK_PATH}${disk}"
                /bin/dd if=/dev/zero of=/dev/disk/by-partlabel/${disk} bs=1M count=1 conv=notrunc
        done
else
        echo "Nothing to format"
fi
