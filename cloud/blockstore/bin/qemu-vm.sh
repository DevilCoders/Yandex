#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
DATA_DIR=`readlink -e $BIN_DIR/data`
CONFIG_DIR=$BIN_DIR/../clusters/$CLUSTER

QEMU=${QEMU:-$BIN_DIR/qemu-system-x86_64}
SETUP_IMAGE=${SETUP_IMAGE:-$DATA_DIR/ubuntu-16.04.6-server-amd64.iso}
LBS_VOLUME=${LBS_VOLUME:-$DATA_DIR/disk.qcow2}
NBS_VOLUME=${NBS_VOLUME:-vol0}
NBD_SOCKET_PATH=${NBD_SOCKET_PATH:-$DATA_DIR/nbd.sock}
VHOST_SOCKET_PATH=${VHOST_SOCKET_PATH:-$DATA_DIR/vhost.sock}
MEM_SIZE=${MEM_SIZE:-2G}

MACHINE_ARGS=" \
    -machine q35,accel=kvm,usb=off \
    -cpu Haswell-noTSX,+vmx \
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
    -netdev user,id=netdev0 \
    -device virtio-net-pci,netdev=netdev0,id=net0 \
    "

VNC_ARGS=" \
    -vnc 127.0.0.1:0 \
    "

SERIAL_ARGS=" \
    -chardev pty,id=charserial0 \
    -device isa-serial,chardev=charserial0,id=serial0 \
    "

USB_ARGS=" \
    -usb \
    -device usb-tablet,bus=usb-bus.0 \
    "

VGA_ARGS=" \
    -device VGA,id=vga0,vgamem_mb=16 \
    "

CDROM_ARGS=" \
    -cdrom $SETUP_IMAGE \
    "

LBS_ARGS=" \
    -drive format=qcow2,file=$LBS_VOLUME,id=lbs0,if=none,aio=native,cache=directsync,detect-zeroes=on \
    -device virtio-scsi-pci,id=scsi0 \
    -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=lbs0,id=scsi0-0-0-0 \
    "

NBS_ARGS=" \
    -object iothread,id=iot0 \
    -drive format=pluggable,id=nbs0,if=none,aio=native,cache=directsync,detect-zeroes=on,impl_path=$BIN_DIR/libblockstore-plugin.so,impl_volume=$NBS_VOLUME,impl_options=$CONFIG_DIR/client.txt \
    -device virtio-blk-pci,scsi=off,drive=nbs0,id=virtio-disk0,iothread=iot0 \
    "

NBD_ARGS=" \
    -object iothread,id=iot1 \
    -drive driver=nbd,id=nbd0,if=none,aio=native,cache=directsync,detect-zeroes=on,reconnect-delay=86400,file=nbd+unix:///?socket=$NBD_SOCKET_PATH \
    -device virtio-blk-pci,scsi=off,drive=nbd0,id=virtio-disk1,iothread=iot1 \
    "

VHOST_BLK_ARGS=" \
    -chardev socket,path=$VHOST_SOCKET_PATH,id=vhost0,reconnect=1 \
    -device vhost-user-blk-pci,chardev=vhost0,id=vhost-user-blk0,num-queues=1 \
    "

VHOST_SCSI_ARGS=" \
    -chardev socket,path=$VHOST_SOCKET_PATH,id=vhost0 \
    -device vhost-user-scsi-pci,chardev=vhost0,id=vhost-user-scsi0 \
    "

# gdb --args \
$QEMU \
    $MACHINE_ARGS \
    $MEMORY_ARGS \
    $NET_ARGS \
    $VNC_ARGS \
    $SERIAL_ARGS \
    $USB_ARGS \
    $VGA_ARGS \
    $NBS_ARGS \
