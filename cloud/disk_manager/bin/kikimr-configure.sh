#!/usr/bin/env bash

WORKING_DIR="$(readlink -e $(dirname $0))"
BIN_DIR="$WORKING_DIR/bin"
DATA_DIR="$WORKING_DIR/data"

set -e

echo "Generating static configuration"
"$BIN_DIR/kikimr_configure" cfg --static "$WORKING_DIR/cluster.yaml" "$BIN_DIR/kikimr" "$DATA_DIR/static"

echo "Generating dynamic configuration"
"$BIN_DIR/kikimr_configure" cfg --dynamic "$WORKING_DIR/cluster.yaml" "$BIN_DIR/kikimr" "$DATA_DIR/dynamic"

echo "Generating NBS configuration"
"$BIN_DIR/kikimr_configure" cfg --nbs "$WORKING_DIR/cluster.yaml" "$BIN_DIR/kikimr" "$DATA_DIR/nbs"
