#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"

"$BIN_DIR/yc-snapshot" \
    -config "snapshot-config.toml" \
    $@
