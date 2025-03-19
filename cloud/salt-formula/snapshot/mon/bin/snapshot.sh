#!/bin/bash
name='snapshot'
check=$(/usr/bin/curl -fs "{{ pillar['snapshot']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
