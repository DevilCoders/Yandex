#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}
NODE=${NODE:-1}
GRPC_PORT=${GRPC_PORT:-9001}
MON_PORT=${MON_PORT:-8765}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`

# gdb --args \
$BIN_DIR/kikimr server \
    --tcp \
    --node              $NODE \
    --grpc-port         $GRPC_PORT \
    --mon-port          $MON_PORT \
    --bootstrap-file    $BIN_DIR/static/boot.txt \
    --bs-file           $BIN_DIR/static/bs.txt \
    --channels-file     $BIN_DIR/static/channels.txt \
    --domains-file      $BIN_DIR/static/domains.txt \
    --log-file          $BIN_DIR/static/log.txt \
    --naming-file       $BIN_DIR/static/names.txt \
    --sys-file          $BIN_DIR/static/sys.txt \
    --ic-file           $BIN_DIR/static/ic.txt \
    --vdisk-file        $BIN_DIR/static/vdisks.txt \
