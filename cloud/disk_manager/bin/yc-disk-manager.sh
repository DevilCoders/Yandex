#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"

"$BIN_DIR/yc-disk-manager" \
    --config "dm-config-server.txt" \
    $@
