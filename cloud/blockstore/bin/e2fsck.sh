#!/usr/bin/env bash

DOMAIN=${DOMAIN:-local}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=$BIN_DIR/../config/$DOMAIN

# gdb --args \
$BIN_DIR/e2fsck \
    --plugin $BIN_DIR/libblockstore-plugin.so \
    --plugin-opts $CONFIG_DIR/client.txt \
    "$@"
