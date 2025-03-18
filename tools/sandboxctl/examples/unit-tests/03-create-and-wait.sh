#!/bin/bash

set -e

DIR=$(realpath $(dirname $0))
BIN="$DIR/../../bin/sandboxctl $SANDBOXCTL_XARGS"

$BIN create --wait -N TEST_TASK 
