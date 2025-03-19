#!/bin/bash

set -ev

sudo apt-get --yes install qemu-utils zerofree
FSCK_VER=$(dpkg-query -W -f='${Version}' e2fsprogs)
if [ $(printf "%s\n" $FSCK_VER 1.43 | sort -rV | head -n1) != $FSCK_VER ]; then
	echo "Unsupported verion of e2fsprogs. Make sure it is newer than 1.43"
	exit
fi

# TODO: Override this vars it with parameters
WORK_DIR=/tmp/
BASE_IMG_URL="https://s3.mds.yandex.net/yc-bootstrap/ubuntu-bionic-20180711-1157"
BASE_IMG=/tmp/ubuntu.img

S3_ACCESS_KEY=pdBVRAAdm8fI8bV8e5rS
S3_SECRET_KEY=bj6gvI2goXyD3u2J32sYLvfBKXiXuswH882yr89E

IMG_DEV=nbd3
IMG_DIR=qemu

if [ ! -f ${BASE_IMG} ]; then 
    curl $BASE_IMG_URL > $BASE_IMG
fi
FILETYPE=$(file -ib $BASE_IMG | awk -F';' '{print $1}')
echo $FILETYPE
if [ "${FILETYPE##text}" != $FILETYPE ]; then
	echo "Base image has unsupported filetype"
	exit
fi

DEVICE="/dev/"${IMG_DEV}

modprobe nbd
umount ${WORK_DIR}${IMG_DIR}/proc || echo "Ok"
umount ${WORK_DIR}${IMG_DIR}/sys || echo "Ok"
umount ${WORK_DIR}${IMG_DIR} || echo "Ok"
qemu-nbd -d ${DEVICE}

qemu-img resize ${BASE_IMG} 15G

mkdir -p ${WORK_DIR}${IMG_DIR}
qemu-nbd -c ${DEVICE} ${BASE_IMG}
partprobe ${DEVICE}

# fix GPT
parted ${DEVICE} print Fix

# resize
MAXSIZEMB=`printf %s\\n 'unit MB print list' | parted | grep "Disk ${DEVICE}" | cut -d' ' -f3 | tr -d MB`
echo ${DEVICE}
echo ${MAXSIZEMB}
# parted --script ${DEVICE} unit MB resizepart 1 ${MAXSIZEMB}"MB" Yes
parted ${DEVICE} unit MB resizepart 1 ${MAXSIZEMB}

e2fsck -f ${DEVICE}"p1"
resize2fs ${DEVICE}"p1"
fsck -y ${DEVICE}"p1"

# TODO mount `fdisk -l /dev/$IMG_DEV | grep $IMG_DEV | grep Linux | awk '{print $$1}'` ${WORK_DIR}$IMG_DIR;
mount ${DEVICE}"p1" ${WORK_DIR}${IMG_DIR}
mount --bind /proc/ ${WORK_DIR}${IMG_DIR}/proc/
mount --bind /sys/ ${WORK_DIR}${IMG_DIR}/sys/
mv -f ${WORK_DIR}${IMG_DIR}/etc/resolv.conf ${WORK_DIR}${IMG_DIR}/etc/resolv.conf.old
cp /etc/resolv.conf ${WORK_DIR}${IMG_DIR}/etc/resolv.conf

cp -r ./sync ${WORK_DIR}${IMG_DIR}/

chroot ${WORK_DIR}${IMG_DIR} bash -c "cd /sync && ./setup.sh"
echo -e "datasource:\n Ec2:\n  strict_id: false\n" > ${WORK_DIR}${IMG_DIR}/etc/cloud/cloud.cfg.d/00_Ec2.cfg

rm -rf ${WORK_DIR}${IMG_DIR}/sync

mv ${WORK_DIR}${IMG_DIR}/etc/resolv.conf.old ${WORK_DIR}${IMG_DIR}/etc/resolv.conf

mount -o remount,ro ${WORK_DIR}${IMG_DIR}
zerofree ${DEVICE}"p1"

umount ${WORK_DIR}${IMG_DIR}/proc || echo "Ok"
umount ${WORK_DIR}${IMG_DIR}/sys || echo "Ok"
umount ${WORK_DIR}${IMG_DIR} || echo "Ok"
qemu-nbd -d /dev/$IMG_DEV

qemu-img convert -c -f qcow2 -O qcow2 ${BASE_IMG}{,_compressed}

exit

PNAME=dsvm-bionic-$(date +%Y%m%d-%H%M)
s3cmd put \
  --access_key="$S3_ACCESS_KEY" \
  --secret_key="$S3_SECRET_KEY" \
  --host="s3.mds.yandex.net" \
  --host-bucket="yc-marketplace.s3.mds.yandex.net" ${BASE_IMG}"_compressed" s3://yc-marketplace/$PNAME

