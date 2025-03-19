#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`

$BIN_DIR/blockstore-http-proxy --config=$BIN_DIR/nbs/nbs-http-proxy.txt $@
