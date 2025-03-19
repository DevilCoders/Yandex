#!/bin/bash

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

run yc-solomon-cli update -c "$CFG" --env israel --solomon israel "$@"
