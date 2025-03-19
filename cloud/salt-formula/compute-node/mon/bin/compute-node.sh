#!/bin/bash
name='compute-node'
check=$(/usr/bin/curl -fs "{{ pillar['compute-node']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
