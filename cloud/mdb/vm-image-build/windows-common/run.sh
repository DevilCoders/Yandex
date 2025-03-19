#!/bin/bash	

set -xe

while [[ $# -gt 0 ]]
do
    key="$1"
    case $key in
        --keep-c)
        KEEPC=YES
        shift
        ;;
        --keep-d)
        KEEPD=YES
        shift
        ;;
        *)
	break
	;;
    esac
done
PRODUCT=$1
PRODUCT_VERSION=$2
IMAGE_TARGET="$1-$2.img"

if [ -z "$IMAGE_TARGET" ]; then
    IMAGE_TARGET="windows-common.img"
fi

echo "Using $IMAGE_TARGET"

if [ -z "$KEEPC" ]; then
    rm -f drivec.img
fi
if [ ! -f drivec.img ]; then
    cp ../$IMAGE_TARGET drivec.img
fi

if [ -z "$KEEPD" ]; then
    rm -rf drived.img
fi
if [ ! -f drived.img ]; then
    qemu-img create -f qcow2 drived.img 2G
fi

kvm \
    -drive file="drivec.img",if=virtio,index=0,media=disk \
    -drive file="drived.img",if=virtio,index=1,media=disk \
    -cpu SandyBridge \
    -smp 2 \
    -m 2048 \
    -usb \
    -device usb-tablet \
    -vnc 127.0.0.1:0 \
    -net nic,model=virtio,vlan=0 \
    -net tap,vlan=0,script=../tap_up.sh \
    -net nic,model=rtl8139
