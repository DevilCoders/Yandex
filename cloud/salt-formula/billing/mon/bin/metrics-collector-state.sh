#!/bin/bash
name='yc-multiprocess-metrics-collector'
health_url="http://localhost:8085/readiness"
check=$(/usr/bin/curl -fs ${health_url})

if [[ ${check} ]] ; then
    echo ${check}
else
    echo "PASSIVE-CHECK:$name;2;Down" ; exit
fi
