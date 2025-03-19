#!/bin/bash

POD_NAME='yc-billing-piper-pod'
POD_CONTAINERS=`docker ps --filter='name=k8s_' --format '{{.Names}}' | grep "_${POD_NAME}-"`

if [[ "$POD_CONTAINERS" =~ "k8s_POD" ]]; then
    echo "PASSIVE-CHECK:piper-pod;0;pod running"
else
    echo "PASSIVE-CHECK:piper-pod;2;pod not running"
fi

for NAME in piper unified-agent jaeger-agent tvmtool; do
    if [[ "$POD_CONTAINERS" =~ "k8s_${NAME}" ]]; then
        echo "PASSIVE-CHECK:piper-pod-${NAME};0;${NAME} running"
    else
        echo "PASSIVE-CHECK:piper-pod-${NAME};2;container ${NAME} not running"
    fi
done
