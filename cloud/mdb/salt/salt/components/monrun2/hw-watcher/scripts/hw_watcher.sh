#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

if [ ! -f /etc/hw_watcher/hw_watcher.conf ]
then
    die 0 "OK"
fi

message=""
status=0

for i in $(grep ^enable_module /etc/hw_watcher/hw_watcher.conf | cut -d= -f2 | sed 's/\,//g')
do
    ret=$(sudo -u hw-watcher hw_watcher "${i}" status 2>/dev/null | head -n1)
    ret_status=$(echo "${ret}" | cut -d\; -f1)
    if [ "${ret_status}" != "OK" ]
    then
        message="${message} $i: ${ret}"
        if [ "$i" = "ecc" ]
        then
            status=2
        elif [ "${status}" != "2" ]
        then
            status=1
        fi
    fi
done

if [ "${status}" = "0" ]
then
    message="OK"
fi

die ${status} "${message}"
