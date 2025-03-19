#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}
IC_PORT=${IC_PORT:-29502}
GRPC_PORT=${GRPC_PORT:-9001}
SERVER_PORT=${SERVER_PORT:-9766}
DATA_SERVER_PORT=${DATA_SERVER_PORT:-9767}
SECURE_SERVER_PORT=${SECURE_SERVER_PORT:-9768}
MON_PORT=${MON_PORT:-8766}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=`readlink -e $BIN_DIR/nbs`

BLOCKSTORE_SERVER=`readlink -e $BIN_DIR/blockstore-server`

#ya tool gdb --args \
$BLOCKSTORE_SERVER \
    --domain             Root \
    --node-broker        localhost:$GRPC_PORT \
    --ic-port            $IC_PORT \
    --mon-port           $MON_PORT \
    --server-port        $SERVER_PORT \
    --data-server-port   $DATA_SERVER_PORT \
    --secure-server-port $SECURE_SERVER_PORT \
    --discovery-file     $CONFIG_DIR/nbs-discovery.txt \
    --domains-file       $CONFIG_DIR/nbs-domains.txt \
    --ic-file            $CONFIG_DIR/nbs-ic.txt \
    --log-file           $CONFIG_DIR/nbs-log.txt \
    --sys-file           $CONFIG_DIR/nbs-sys.txt \
    --server-file        $CONFIG_DIR/nbs-server.txt \
    --storage-file       $CONFIG_DIR/nbs-storage.txt \
    --naming-file        $CONFIG_DIR/nbs-names.txt \
    --diag-file          $CONFIG_DIR/nbs-diag.txt \
    --auth-file          $CONFIG_DIR/nbs-auth.txt \
    --dr-proxy-file      $CONFIG_DIR/nbs-dr-proxy.txt \
    --service            kikimr \
    --load-configs-from-cms \
    $@
