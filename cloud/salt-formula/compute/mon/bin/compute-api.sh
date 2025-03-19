#!/bin/bash
name='compute-api'
check=$(/usr/bin/curl -fs "{{ pillar['compute-api']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
