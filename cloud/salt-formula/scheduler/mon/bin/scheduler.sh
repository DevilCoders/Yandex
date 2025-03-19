#!/bin/bash
name='scheduler'
check=$(/usr/bin/curl -fs "{{ pillar['scheduler']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
