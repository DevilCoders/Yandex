#!/bin/bash

function run { (set -x; "$@") }
DIR=$(dirname "$0")
CFG=$DIR/config.yaml
set -e

source $DIR/../base/sync.sh "$@"