#!/bin/bash

function run { (set -x; "$@") }

DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

SCRIPTS_DIR=$(dirname $(realpath "$BASH_SOURCE"))
source "$SCRIPTS_DIR/common.sh"

run yc-solomon-cli update -c "$CFG" --env testing --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env preprod --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env prod    --solomon main "$@"

if [[ ! -z "$TEAMCITY" ]]
then
  ISRAEL_SECRET_ID=e6q6971faa1309nd50oc
  SOLOMON_IAM_TOKEN=$(get_iam_token $ISRAEL_SECRET_ID "israel")
fi
run yc-solomon-cli update -c "$CFG" --env israel  --solomon israel "$@"
