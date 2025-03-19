#!/bin/bash

# CADMIN-8562
which detect_vms.sh 2>&1 >> /dev/null

if [ $? -eq 0 ]; then
    detect_vms.sh load_average
    if [ $? -eq 0 ]; then
        echo "PASSIVE-CHECK:la;0;Check disabled by excluded_vms.conf"
        exit 0
    fi
fi

. /usr/local/sbin/autodetect_environment

CONF=/etc/monitoring/la.conf

CPU_THRESHOLD=$(awk '/processor/ {cpunum=$NF} END { print (cpunum+1)*2}' < /proc/cpuinfo)
THRESHOLD=30
if [ -r $CONF ]; then
    THRESHOLD=$(cat $CONF)
elif (( THRESHOLD < CPU_THRESHOLD )); then
    THRESHOLD=$CPU_THRESHOLD
fi

load_average(){
answer=$(sed 's/\..*/ /g' /proc/loadavg)
if (( answer > THRESHOLD ))
    then echo "PASSIVE-CHECK:la;2; Load average is big: $answer"
else
    echo "PASSIVE-CHECK:la;0;OK, la: $answer less then: $THRESHOLD"
fi
exit 0
}

if [ "$is_dom0_host" -eq 1 ]; then
    load_average;
fi

if [ "$(echo $HOSTNAME | egrep '^src-mskm9|^src-rtmp-mskm9|^src-mskstoredata|^src-rtmp-mskstoredata')" ]; then
    load_average
elif [ "$is_openvz_host" -eq 1 ] || [ "$is_lxc_host" -eq 1 ]; then
    echo "PASSIVE-CHECK:la;0;OK, virtual host, skip LA checking"
else
    load_average
fi


if [[ $is_virtual_host -eq 1 && $is_kvm_host -eq 1 ]]; then
    load_average;
fi

if [ "$is_classic_host" -eq 1 ]; then
    load_average;
fi

