#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
DATA_DIR=$BIN_DIR/data
CONFIG_DIR=$BIN_DIR/../clusters/$CLUSTER

set -e
$BIN_DIR/kikimr_configure cfg --static $CONFIG_DIR/cluster.yaml $BIN_DIR/kikimr $BIN_DIR/static
$BIN_DIR/kikimr_configure cfg --dynamic $CONFIG_DIR/cluster.yaml $BIN_DIR/kikimr $BIN_DIR/dynamic
$BIN_DIR/kikimr_configure cfg --nbs $CONFIG_DIR/cluster.yaml $BIN_DIR/kikimr $BIN_DIR/nbs

# create Disk Registry Proxy config

cat > $BIN_DIR/nbs/nbs-dr-proxy.txt <<- "EOF"
Owner: 154157143141154
OwnerIdx: 1
EOF
