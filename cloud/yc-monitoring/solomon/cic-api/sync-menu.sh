#!/bin/bash

if [ "$FORCE_SYNC_MENU" != 1 ]; then
    echo "WARNING! Menu update is temporarily disabled due to problem https://st.yandex-team.ru/CLOUD-86220"
    exit 0
fi

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

run yc-solomon-cli menu -c "$CFG" --env testing --solomon main "$@"
run yc-solomon-cli menu -c "$CFG" --env preprod --solomon main "$@"
run yc-solomon-cli menu -c "$CFG" --env prod    --solomon main "$@"

