#!/bin/bash

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

source $DIR/../base/sync.sh "$@"

echo "========================== L7 in PROD Cloud Solomon =========================="
run yc-solomon-cli update -c "$CFG" --env preprod       --solomon prod --tag l7 "$@"
run yc-solomon-cli update -c "$CFG" --env prod          --solomon prod --tag l7 "$@"

echo "================================== YC-Search ================================="
run yc-solomon-cli update -c "$CFG" --env preprod --solomon preprod --tag yc-search "$@"
run yc-solomon-cli update -c "$CFG" --env prod    --solomon prod    --tag yc-search "$@"
