#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
CONFIG_DIR=$BIN_DIR/../clusters/$CLUSTER

QEMU_NBD=${QEMU_NBD:-$BIN_DIR/qemu-nbd}
DEVICE=${DEVICE:-/dev/nbd0}
LBS_VOLUME=${LBS_VOLUME:-$BIN_DIR/images/ubuntu-16.04-server-cloudimg-amd64-disk1.img}
NBS_VOLUME=${VOLUME:-vol0}

LBS_ARGS=" \
    --format=qcow2 \
    $LBS_VOLUME \
    "

OPTIONS="${CONFIG_DIR}/plugin.txt"

NBS_ARGS=" \
    --image-opts \
    driver=pluggable,impl_path=${BIN_DIR}/libblockstore-plugin.so,impl_volume=${NBS_VOLUME},impl_options=${OPTIONS} \
    "

# gdb --args \
$QEMU_NBD \
    --verbose \
    --connect $DEVICE \
    --aio=native \
    --cache=directsync \
    --detect-zeroes=on \
    $NBS_ARGS \
