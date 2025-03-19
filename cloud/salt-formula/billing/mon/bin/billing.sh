#!/bin/bash
name='billing'
check=$(/usr/bin/curl -fs "{{ pillar['billing']['monitoring_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down" ; exit
fi
