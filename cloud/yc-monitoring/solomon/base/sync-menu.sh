#!/bin/bash

if [ "$FORCE_SYNC_MENU" != 1 ]; then
    echo "WARNING! Menu update is temporarily disabled due to problem https://st.yandex-team.ru/CLOUD-86220"
    exit 0
fi

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

SCRIPTS_DIR=$(dirname $(realpath "$0"))
source "$SCRIPTS_DIR/common.sh"

run yc-solomon-cli menu -c "$CFG" --env testing --solomon main "$@"
run yc-solomon-cli menu -c "$CFG" --env preprod --solomon main "$@"
run yc-solomon-cli menu -c "$CFG" --env prod    --solomon main "$@"

ISRAEL_SECRET_ID=e6q6971faa1309nd50oc
SOLOMON_IAM_TOKEN=$(get_iam_token $ISRAEL_SECRET_ID "israel")
run yc-solomon-cli menu -c "$CFG" --env israel  --solomon israel "$@"
