#!/bin/sh
if [ "$1x" == "x" ]
then
	echo "Usage : initrd-broadcom-killer.sh <kernel-version>"
	exit 1
fi
KERNELVER=$1

TMPDIR=/tmp/initrd
mkdir -p $TMPDIR
rm -rf $TMPDIR/*
cd $TMPDIR

gzip -dc /boot/initrd.img-$KERNELVER | cpio -id
if [ $? -ne 0 ]
then
	echo "Can not extract initrd file /boot/initrd.img-$KERNELVER"
	exit 1;
fi

cp /boot/initrd.img-$KERNELVER /boot/initrd.img-$KERNELVER.backup
find . -name bnx2.ko -delete
find . | cpio --quiet --dereference -o -H newc | gzip -9 > /boot/initrd.img-$KERNELVER
