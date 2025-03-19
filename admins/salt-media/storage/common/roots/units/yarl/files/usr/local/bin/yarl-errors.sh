#!/bin/bash

timetail_time=900

errors=$(timetail -n "${timetail_time}" -t tskv /var/log/yarl/yarl.log /var/log/yarl/nginx.log |grep 'failed to merge quotas' | wc -l)

if [ "${errors}" -gt 0 ]
then
    echo "2;${errors} errors in /var/log/yarl/{yarl, nginx}.log"
else
    echo "0;Ok"
fi
