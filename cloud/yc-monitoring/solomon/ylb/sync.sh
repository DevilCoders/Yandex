#!/bin/bash

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

run yc-solomon-cli update -c "$CFG" --env testing --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env testing --solomon preprod "$@"
run yc-solomon-cli update -c "$CFG" --env preprod --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env preprod --solomon prod "$@"
run yc-solomon-cli update -c "$CFG" --env prod    --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env prod    --solomon prod "$@"
