#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=`readlink -e $BIN_DIR/nfs`

# ya tool gdb --args \
$BIN_DIR/filestore-http-proxy \
    --config=$CONFIG_DIR/nfs-http-proxy.txt \
    $@
