#!/bin/bash
{%- set endpoint = pillar['access-service']['monitoring_endpoint'] %}
name='access-service'

/usr/bin/curl -fs "http://{{ endpoint['host'] }}:{{ endpoint['port'] }}{{ endpoint['path'] }}"
if [[ $? -eq 0 ]]; then
    echo "PASSIVE-CHECK:$name;0;Ok"
else
    echo "PASSIVE-CHECK:$name;2;Down"
fi
