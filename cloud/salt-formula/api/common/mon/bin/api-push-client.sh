#!/bin/bash
name='api-push-client'
check=$(push-client -c /etc/yandex/statbox-push-client/push-client.yaml --status 2>&1 | grep "status: ok")

if [[ $check ]] ; then
    echo "PASSIVE-CHECK:api-push-client;0;OK"
else
    echo "PASSIVE-CHECK:api-push-client;2;Down"; exit
fi
