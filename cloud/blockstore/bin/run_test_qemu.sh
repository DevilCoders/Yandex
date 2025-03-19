#!/usr/bin/env bash

set +e

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
STORAGE_DIR=$BIN_DIR/../../storage
QEMU_DIR=$STORAGE_DIR/core/tools/testing/qemu

: ${QEMU_BIN_DIR:=$QEMU_DIR/bin}

QEMU_BIN_TAR=$QEMU_BIN_DIR/qemu-bin.tar.gz
QEMU_REL=/usr/bin/qemu-system-x86_64
QEMU_FIRMWARE_REL=/usr/share/qemu

: ${QEMU:=$QEMU_BIN_DIR$QEMU_REL}
: ${QEMU_FIRMWARE:=$QEMU_BIN_DIR$QEMU_FIRMWARE_REL}

[[ ( ! -x $QEMU && ${QEMU%$QEMU_REL} == $QEMU_BIN_DIR ) ||
    ( ! -d $QEMU_FIRMWARE &&
      ${QEMU_FIRMWARE%$QEMU_FIRMWARE_REL} == $QEMU_BIN_DIR ) ]] &&
      tar -xzf $QEMU_BIN_TAR -C $QEMU_BIN_DIR


: ${QMP_PORT:=4444}
: ${DISK_IMAGE:=$QEMU_DIR/image/rootfs.img}

CONFIG_DIR=$BIN_DIR/nbs
: ${NBS_VOLUME:=vol0}

: ${MEM_SIZE:=4G}

MACHINE_ARGS=" \
    -L $QEMU_FIRMWARE \
    -snapshot \
    -cpu host \
    -smp 16,sockets=1,cores=16,threads=1 \
    -enable-kvm \
    -m $MEM_SIZE \
    -name debug-threads=on \
    -qmp tcp:localhost:$QMP_PORT,server,nowait \
    "

MEMORY_ARGS=" \
    -object memory-backend-memfd,id=mem,size=$MEM_SIZE,share=on \
    -numa node,memdev=mem \
    "

NET_ARGS=" \
    -netdev user,id=netdev0,hostfwd=tcp::22222-:22 \
    -device virtio-net-pci,netdev=netdev0,id=net0 \
    "

DISK_ARGS=" \
    -object iothread,id=iot0 \
    -drive format=qcow2,file=$DISK_IMAGE,id=lbs0,if=none,aio=native,cache=none,discard=unmap \
    -device virtio-blk-pci,scsi=off,drive=lbs0,id=virtio-disk0,iothread=iot0,bootindex=1 \
    "

NBS_ARGS=" \
    -object iothread,id=iot1 \
    -drive format=pluggable,id=nbs0,if=none,detect-zeroes=on,impl_path=$BIN_DIR/libblockstore-plugin.so,impl_volume=$NBS_VOLUME,impl_options=$CONFIG_DIR/nbs-client.txt \
    -device virtio-blk-pci,scsi=off,drive=nbs0,id=virtio-disk1,iothread=iot1 \
    "

# ya tool gdb --args \
$QEMU \
    $MACHINE_ARGS \
    $MEMORY_ARGS \
    $NET_ARGS \
    $DISK_ARGS \
    $NBS_ARGS \
    -nographic \
    -s \
    $@
