#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"
DATA_DIR="$WORKING_DIR/data"

set -e

"$BIN_DIR/kikimr" server \
    --tcp \
    --node              1 \
    --grpc-port         9001 \
    --mon-port          8765 \
    --bootstrap-file    "$DATA_DIR/static/boot.txt" \
    --bs-file           "$DATA_DIR/static/bs.txt" \
    --channels-file     "$DATA_DIR/static/channels.txt" \
    --domains-file      "$DATA_DIR/static/domains.txt" \
    --log-file          "$DATA_DIR/static/log.txt" \
    --naming-file       "$DATA_DIR/static/names.txt" \
    --sys-file          "$DATA_DIR/static/sys.txt" \
    --ic-file           "$DATA_DIR/static/ic.txt" \
    --vdisk-file        "$DATA_DIR/static/vdisks.txt" \
    $@
