#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`

COMMAND_NAME=$1
shift 1

$BIN_DIR/blockstore-client $COMMAND_NAME --config=$BIN_DIR/nbs/nbs-client.txt $@
