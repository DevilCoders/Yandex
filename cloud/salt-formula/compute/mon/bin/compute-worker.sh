#!/bin/bash
name='compute-worker'
check=$(/usr/bin/curl -fs "{{ pillar['compute-worker']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
