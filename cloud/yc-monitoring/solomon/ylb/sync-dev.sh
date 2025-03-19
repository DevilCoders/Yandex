#!/bin/bash

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

run yc-solomon-cli update -c "$CFG" --env hw-cgw-dev-lab --solomon main "$@"
run yc-solomon-cli update -c "$CFG" --env hw-cgw-ci-lab --solomon main "$@"
