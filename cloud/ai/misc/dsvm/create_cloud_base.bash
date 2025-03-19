#!/bin/bash
set -e
set -x

# https://medium.com/@kenichishibata/resize-aws-ebs-4d6e2bf00feb

MOUNT_DIR="/mnt/ubuntu-prepare_$((1 + RANDOM % 9999999999))"

IMG_FILE=$1

function resize_image {
    IMG=$1
    SIZE=$2
    qemu-img resize $IMG +$SIZE
}

function grow_guest_part {
    DIR=$1
    GROW=$(which growpart)
    RESIZER=$(which resize2fs)
    mkdir -p $(dirname $DIR/$GROW $DIR/RESIZER)
    cp $GROW $DIR/$GROW
    cp $RESIZER $DIR/$RESIZER
    chroot $DIR $GROW /dev/nbd0 1 ||:
    chroot $DIR $RESIZER /dev/nbd0p1 ||:
}

function mount_image {
    DIR=$1
    IMG=$2

    modprobe nbd max_part=8
    qemu-nbd -c /dev/nbd0 $IMG
    partprobe

    mkdir $DIR
    mount /dev/nbd0p1 $DIR
    mount -t proc /proc/ $DIR/proc
    mount --rbind /sys $DIR/sys
    mount --make-rslave $DIR/sys
    mount --rbind /dev $DIR/dev
    mount --make-rslave $DIR/dev
    mount --rbind /run $DIR/run
    mount --make-rslave $DIR/run

    cp ./prepare.bash $DIR
    chmod +x $DIR/prepare.bash
}

function prepare {
    DIR=$1
    chroot $DIR /bin/bash -c '/prepare.bash'
    
    # zero free blocks to maximize compression effect
    mount -o remount,ro $DIR
    zerofree /dev/nbd0p1
}

function unmount {
    DIR=$1
    umount $DIR/proc
    umount -R $DIR/sys
    umount -R $DIR/dev
    umount -R $DIR
    qemu-nbd -d /dev/nbd0
    rm -rf $DIR
}

function compress_qcow2 {
    FILE=$1
    qemu-img convert -c -f qcow2 -O qcow2 $FILE $FILE"_compressed"
}

if [ -f $1 ] && [ ! -z $1 ]
  then
    IMG_FILE=$1
  else
    echo "Invalid image-file name $1!" 1>&2
    exit 1
fi
#base image size is not enough for our packages
IMG_SIZE=${2:-5G}
resize_image $IMG_FILE $IMG_SIZE
mount_image $MOUNT_DIR $IMG_FILE
grow_guest_part $MOUNT_DIR
prepare $MOUNT_DIR
unmount $MOUNT_DIR
compress_qcow2 $IMG_FILE
