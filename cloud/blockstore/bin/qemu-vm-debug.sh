#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
DATA_DIR=`readlink -e $BIN_DIR/data`
LOGS_DIR=`readlink -e $BIN_DIR/logs`

QEMU=${QEMU:-$BIN_DIR/qemu-system-x86_64}
KERNEL_IMAGE=${KERNEL_IMAGE:-$DATA_DIR/vmlinux}
DISK_IMAGE=${DISK_IMAGE:-$DATA_DIR/rootfs.img}
VHOST_SOCKET_PATH=${VHOST_SOCKET_PATH:-$DATA_DIR/vhost.sock}
MEM_SIZE=${MEM_SIZE:-2G}
SSH_PORT=${SSH_PORT:-2222}

MACHINE_ARGS=" \
    -cpu host \
    -smp 2,sockets=2,cores=1,threads=1 \
    -enable-kvm \
    -m $MEM_SIZE \
    -nodefaults \
    -no-user-config \
    -boot strict=on \
    -name debug-threads=on \
    "

MEMORY_ARGS=" \
    -object memory-backend-memfd,id=mem,size=$MEM_SIZE,share=on \
    -numa node,memdev=mem \
    "

NET_ARGS=" \
    -netdev user,id=net0,hostfwd=tcp::$SSH_PORT-:22 \
    -device e1000,netdev=net0 \
    "

DISK_ARGS=" \
    -drive file=$DISK_IMAGE,index=0,media=disk,format=qcow2 \
    "

VHOST_BLK_ARGS=" \
    -chardev socket,path=$VHOST_SOCKET_PATH,id=vhost0,reconnect=1 \
    -device vhost-user-blk-pci,chardev=vhost0,id=vhost-user-blk0,num-queues=1 \
    "

VHOST_SCSI_ARGS=" \
    -chardev socket,path=$VHOST_SOCKET_PATH,id=vhost0 \
    -device vhost-user-scsi-pci,chardev=vhost0,id=vhost-user-scsi0,num_queues=1 \
    "

# ya tool gdb --args \
$QEMU \
    $MACHINE_ARGS \
    $MEMORY_ARGS \
    $NET_ARGS \
    $DISK_ARGS \
    $VHOST_SCSI_ARGS \
    -nographic \
    -monitor stdio \
    -chardev file,path=$LOGS_DIR/serial.log,id=serial0,mux=on \
    -device isa-serial,chardev=serial0 \
    -device isa-debugcon,iobase=0x402,chardev=serial0 \
    -trace file=$LOGS_DIR/trace.log \
    -kernel $KERNEL_IMAGE \
    -append "console=ttyS0 root=/dev/sda rw" \
    -s \
    $@
