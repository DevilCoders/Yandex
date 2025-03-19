#!/bin/bash

exec 2>/dev/null

if [[ $(hostname | grep lxd) ]]
then
        #won't work for 14.04
    num=$(/usr/sbin/invoke-rc.d atop status | grep '/usr/bin/atop' | awk '{print $1}' | grep -o '[0-9]\+')
else
    num=$(pidof atop)
fi

logfile=$(ls -t /var/log/atop | grep atop | sed -n '1p')

if [[ $num ]]
then
    if [[ $(atop -r /var/log/atop/$logfile | head |sed -n '3s!/!!gp' | grep $(date +%Y%m%d)) ]]
    then
        if [[ $(ps -o args $num | tail -n 1 | awk '{print $NF}') -eq 60 ]]
        then
                echo "0;OK"
        else
                echo "1;WARN: atop collects stats less than once a minute"
        fi
    else
        echo "1;WARN: atop is probably not rotating"
    fi
else
    echo "2;CRIT: atop is not running"
fi
