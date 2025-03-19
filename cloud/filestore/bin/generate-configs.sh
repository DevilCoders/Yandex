#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

abspath() {
    path=$1

    if [ ! -d $path ]; then
        echo $path
        return
    fi

    (cd $1; pwd)
}

BIN_DIR=`find_bin_dir`
ARC_DIR=`abspath $BIN_DIR/../../../`

$BIN_DIR/config_generator -a $ARC_DIR -s $ARC_DIR/cloud/storage/core/tools/ops/config_generator/services/nfs

