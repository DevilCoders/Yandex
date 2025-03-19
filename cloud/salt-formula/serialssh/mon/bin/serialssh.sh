#!/bin/bash
name='serialssh'

if /usr/bin/curl -fs "{{ pillar['serialssh']['monitoring_endpoint'] }}" -o /dev/null ; then
    echo "PASSIVE-CHECK:$name;0;ok"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
