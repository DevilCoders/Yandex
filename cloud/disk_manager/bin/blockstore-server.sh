#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"
DATA_DIR="$WORKING_DIR/data"

"$BIN_DIR/blockstore-server" \
    --domain             "Root" \
    --node-broker        "localhost:9001" \
    --ic-port            29502 \
    --mon-port           8766 \
    --domains-file       "$DATA_DIR/nbs/nbs-domains.txt" \
    --ic-file            "$DATA_DIR/nbs/nbs-ic.txt" \
    --log-file           "$DATA_DIR/nbs/nbs-log.txt" \
    --sys-file           "$DATA_DIR/nbs/nbs-sys.txt" \
    --server-file        "$DATA_DIR/nbs/nbs-server.txt" \
    --storage-file       "$DATA_DIR/nbs/nbs-storage.txt" \
    --naming-file        "$DATA_DIR/nbs/nbs-names.txt" \
    --diag-file          "$DATA_DIR/nbs/nbs-diag.txt" \
    --auth-file          "$DATA_DIR/nbs/nbs-auth.txt" \
    $@
