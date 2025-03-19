#!/bin/bash
name='billing-private'
check=$(/usr/bin/curl -fs "{{ pillar['billing']['monitoring_private_endpoint'] }}")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down" ; exit
fi
