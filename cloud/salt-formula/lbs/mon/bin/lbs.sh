#!/bin/bash
name='lbs'
check=$(/usr/bin/curl -fs "{{ pillar['lbs']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
