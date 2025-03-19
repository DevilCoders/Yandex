#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"

"$BIN_DIR/accessservice-mock" \
    --config "accessservice-mock-config.txt" \
    $@
