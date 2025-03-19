#!/bin/bash
#
# Provides: cpu_overheat

me=${0##*/}    # strip path
me=${me%.*}    # strip extension


die() {
    echo "$1;$2"
    exit 0
}

tempmax=30
tempmaxname=none
tempmaxthreshold=72
# Dannye zapolnyautsya iz /etc/cron.d/sensors.sh
for i in `ls /var/tmp/ipmi_sensor 2>/dev/null`
do
    temp=`cat /var/tmp/ipmi_sensor | grep CPU | grep Temp | head -1 | awk -F "|" '{print $2}' | awk -F "." '{print $1}' | sed 's/ //g'`;
    if [ -z "$temp" ]; then 
        temp=$tempmax
    fi
    if [ "$temp" == "0x4" ] 2>/dev/null
    then
	temp=73
    fi
    if [ "$temp" -eq "$temp" ] 2>/dev/null
    then
        if [ "$temp" -gt "$tempmax" ]; then
            tempmax=$temp
        fi
    fi
done

if [ "$tempmax" -gt $tempmaxthreshold ]; then
    die "1" "Bad CPU temperature $tempmax";
else
    die "0" "CPU temp is OK";
fi
exit 0;
