#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"
DATA_DIR="$WORKING_DIR/data"
DISK_FILE="$DATA_DIR/pdisk.data"

set -e

rm -f "$DISK_FILE"
dd if=/dev/zero of="$DISK_FILE" bs=1 count=0 seek=32G
