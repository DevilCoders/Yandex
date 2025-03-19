#!/bin/bash
name='serialproxy'

if /usr/bin/curl -fs "{{ pillar['serialproxy']['monitoring_endpoint'] }}" -o /dev/null ; then
    echo "PASSIVE-CHECK:$name;0;ok"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
