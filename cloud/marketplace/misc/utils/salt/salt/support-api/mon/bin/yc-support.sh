#!/bin/bash
name='support'
health_url="http://localhost:5001/health?monrun=1"

check=$(/usr/bin/curl -fs ${health_url})

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:$check"
else
    echo "PASSIVE-CHECK:$name;2;Down" ; exit
fi
