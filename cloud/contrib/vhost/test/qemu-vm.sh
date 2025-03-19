#!/usr/bin/env bash


BIN_DIR=$(realpath "${BIN_DIR:-./}")
QEMU=${QEMU:-./x86_64-softmmu/qemu-system-x86_64}
SETUP_IMAGE=${SETUP_IMAGE:-$BIN_DIR/images/ubuntu-16.04.3-server-amd64.iso}
LBS_VOLUME=${LBS_VOLUME:-/mnt/data/ubuntu16.qcow2}
RAW_VOLUME=${RAW_VOLUME:-vol0}
NBS_VOLUME=${NBS_VOLUME:-vol0}
NBS_VOLUME2=${NBS_VOLUME2:-vol1}
NBS_OPTIONS=$(realpath "${NBS_OPTIONS:-$BIN_DIR/../config/local/client.txt}")
NBS_TOKEN=${NBS_TOKEN:-$NBS_VOLUME.token}
BIOS_PATH=/usr/local/share/qemu/bios-256k.bin
QMP_SOCK_PATH=/tmp/qmp.sock
VHOST_USER_BLK_SOCK=${VHOST_USER_BLK_SOCK:-/tmp/vhost-user-blk.sock}

CPU_HYPERV_ARGS=" \
    -cpu Haswell-noTSX,l3-cache=on,+vmx,hv_relaxed,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_crash,hv_reset,hv_vpindex,hv_runtime,hv_synic,hv_stimer \
    -smp 2,sockets=1,cores=1,threads=2 \
    "

CPU_ARGS=" \
    -cpu Haswell-noTSX,+vmx,l3-cache=on,+spec-ctrl,+ssbd \
    -smp 2,cores=1,threads=2,sockets=1 \
    "

#-object memory-backend-ram,id=ram-node0,size=4096M,host-nodes=0,policy=bind \
#-numa node,cpus=0,memdev=ram-node0 \

MACHINE_ARGS=" \
    -machine q35,accel=kvm,usb=off,sata=on,zeropage-scan=false,nvdimm \
    -enable-kvm \
    -m 4096M,maxmem=8G,slots=2 \
    -nodefaults \
    -no-user-config \
    -boot menu=on \
    -name testvm,debug-threads=on \
    -qmp unix:$QMP_SOCK_PATH,server,nowait \
    -device pvpanic \
    -device usb-ehci \
    -device usb-tablet \
    -smbios type=1,manufacturer=Yandex,product=xeon-e5-2660,uuid=23000006-2c11-0ee0-23a1-c6e83735f7fb,family=\"YandexCloud\",serial=YC-cb0h1rg278e6t0rjbtvr  \
    -chardev file,path=/tmp/seabios.log,id=debugcon \
    -device isa-debugcon,iobase=0x402,chardev=debugcon \
    -bios /home/wrfsh/seabios/out/bios.bin \
    \
    -object memory-backend-file,id=mem0,share,mem-path=/tmp/nvdimm0,size=32M \
    -device nvdimm,memdev=mem0,id=nv0 \
    "

GA_ARGS="\
    -chardev socket,path=/tmp/qga.sock,server,nowait,id=qga0 \
    -device virtio-serial,bus=pcie.0 \
    -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0 \
    "

VNC_ARGS=" \
    -vnc 127.0.0.1:0 \
    "

SERIAL_ARGS=" \
    -chardev file,path=/tmp/serial0.log,id=charserial0 \
    -device isa-serial,chardev=charserial0,id=serial0 \
    "

NET_ARGS=" \
    -netdev user,id=netdev0,hostfwd=tcp::20002-:22 \
    -device virtio-net-pci,netdev=netdev0,id=net0,bus=pcie.0 \
    "

VGA_ARGS=" \
    -vga std
    "
    #-device VGA,id=vga0,vgamem_mb=16,bus=pcie.0 \

CDROM_ARGS=" \
    -cdrom $SETUP_IMAGE \
    "

LBS_ARGS=" \
    -drive format=qcow2,file=$LBS_VOLUME,node-name=node-name-idcnl1ppehnelec6d9sp,if=none,aio=native,cache=directsync,detect-zeroes=on,serial=lbsdrive0 \
    -device virtio-blk-pci,scsi=off,bus=pcie.0,drive=node-name-idcnl1ppehnelec6d9sp,id=virtio-disk0,bootindex=1,num-queues=4
    "

NBS_ARGS=" \
    -object iothread,id=iot0 \
    -drive format=pluggable,impl_path=$BIN_DIR/tests/libblockstore-mock.so,impl_volume=$NBS_VOLUME,impl_options=$NBS_OPTIONS,id=drive1,if=none,aio=native,cache=writethrough,detect-zeroes=on,impl_mount_token_path=$BIN_DIR/tests/t0.txt \
    -device virtio-blk-pci,scsi=off,drive=drive1,id=virtio-disk1,iothread=iot0,bus=pcie.0 \
    "

RAW_ARGS="\
    -object iothread,id=raw-iot0 \
    -drive format=raw,id=raw-drive-0,if=none,file=$RAW_VOLUME,aio=threads \
    -device virtio-blk,scsi=off,drive=raw-drive-0,id=virtio-raw-drive-0,iothread=raw-iot0 \
    "

VHOST_USER_BLK_ARGS="\
    -chardev socket,id=vhost-user-blk0.sock,path=$VHOST_USER_BLK_SOCK \
    -device vhost-user-blk-pci,bus=pcie.0,chardev=vhost-user-blk0.sock,num-queues=1 \
    "
TRACE_ARGS="\
     -trace events=/tmp/qemu-trace-events-lol
    "

gdb --args \
$QEMU \
    $MACHINE_ARGS \
    $CPU_ARGS \
    $VNC_ARGS \
    $SERIAL_ARGS \
    $GA_ARGS \
    $NET_ARGS \
    $VGA_ARGS \
    $LBS_ARGS \
    $VHOST_USER_BLK_ARGS \

