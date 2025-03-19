#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=`readlink -e $BIN_DIR/nfs`

SERVER_PORT=${SERVER_PORT:-9021}
VHOST_PORT=${VHOST_PORT:-9022}
MON_PORT=${MON_PORT:-8768}
VERBOSE=${VERBOSE:-info}

# ya tool gdb --args \
$BIN_DIR/filestore-vhost \
    --vhost-file        $CONFIG_DIR/nfs-vhost.txt \
    --diag-file         $CONFIG_DIR/nfs-diag.txt \
    --mon-port          $MON_PORT \
    --verbose           $VERBOSE \
    $@
