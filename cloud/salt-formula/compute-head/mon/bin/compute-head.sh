#!/bin/bash
name='compute-head'

if /usr/bin/curl -fs "{{ pillar['compute-head']['monitoring_endpoint'] }}" -o /dev/null ; then
    echo "PASSIVE-CHECK:$name;0;ok"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
