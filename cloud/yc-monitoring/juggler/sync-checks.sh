#!/bin/bash
set -e
cd "$(dirname "$0")"

if [ -z "$1" ]; then
    echo "Usage: $(basename $0) (dev-builds|testing|preprod|prod|israel) [--apply]"
    exit 1
fi

ENV="$1"

if [ "$2" = "--apply" ]; then
    ANSIBLE_ARGS=""
else
    ANSIBLE_ARGS="--check"
fi

ansible-playbook yc_checks_config_$ENV.yml -vvv $ANSIBLE_ARGS
