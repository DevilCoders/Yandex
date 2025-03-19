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

SRC_DIR=`abspath ${E2FS_TREE:-$BIN_DIR/../../../../../../YandexCloud/e2fsprogs}`
BUILD_DIR=$SRC_DIR/build

pushd $BUILD_DIR

git pull --recurse
../configure
make -j 16

popd

ln -svf $BUILD_DIR/e2fsck/e2fsck $BIN_DIR/
