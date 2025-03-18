#!/usr/bin/env bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 (curdb|api)"
    exit 1
fi

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${ROOT}/gen-configs-balancer.sh $1
${ROOT}/gen-balancer-test.sh $1
${ROOT}/gen-configs-l7-balancer.sh $1
