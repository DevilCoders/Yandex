#!/bin/bash

root_freespace=$(LANG=C df -hP / | grep -v "Filesystem.*Size.*Used" | awk '{print $5}' | sed 's/\%//g') 

if ! [[ ${root_freespace} =~ ^[0-9]+$ ]] ; then
    echo "2; not a number"; exit 1
fi

if [[ ${root_freespace} -le "97" ]]; then
    echo "0; ok, only ${root_freespace}% used"
elif [[ ${root_freespace} == "98" ]]; then
    echo "2; 2% free only"
elif [[ ${root_freespace} -ge "99" ]]; then
    echo "2; 0% or 1% free, rejecting balancers";
    iptruler all down;
    iptruler all down;
fi

