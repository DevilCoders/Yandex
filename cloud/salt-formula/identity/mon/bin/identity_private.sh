#!/bin/bash
name='identity'
check=$(/usr/bin/curl -fs "{{ pillar['identity']['monitoring_endpoint']['private'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
