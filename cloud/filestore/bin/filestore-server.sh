#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=`readlink -e $BIN_DIR/nfs`

IC_PORT=${IC_PORT:-29011}
KIKIMR_PORT=${KIKIMR_PORT:-9001}
SERVER_PORT=${SERVER_PORT:-9021}
MON_PORT=${MON_PORT:-8767}

# ya tool gdb --args \
$BIN_DIR/filestore-server \
    --domain             Root \
    --scheme-shard-dir   /Root/NFS \
    --node-broker        localhost:$KIKIMR_PORT \
    --ic-port            $IC_PORT \
    --mon-port           $MON_PORT \
    --server-port        $SERVER_PORT \
    --sys-file           $CONFIG_DIR/nfs-sys.txt \
    --log-file           $CONFIG_DIR/nfs-log.txt \
    --domains-file       $CONFIG_DIR/nfs-domains.txt \
    --naming-file        $CONFIG_DIR/nfs-names.txt \
    --diag-file          $CONFIG_DIR/nfs-diag.txt \
    --ic-file            $CONFIG_DIR/nfs-ic.txt \
    --server-file        $CONFIG_DIR/nfs-server.txt \
    --storage-file       $CONFIG_DIR/nfs-storage.txt \
    --service            kikimr \
    $@
