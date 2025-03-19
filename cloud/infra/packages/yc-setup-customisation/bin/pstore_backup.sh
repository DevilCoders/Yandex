#!/bin/bash

PSTORE_PATH=/sys/fs/pstore
PSTORE_BACKUP_PATH=/var/lib/systemd/pstore

set -e

ubuntu_ver=$(lsb_release -r | cut -f2)
if [ ${ubuntu_ver} != "16.04" ];
then
    echo "ERROR! Use pstore backup temporarely only  for 16.04! Until systemd-pstore.service is not released!"
    exit 1
fi

echo "Mount pstore inside chroot!"
mount -t pstore pstore ${PSTORE_PATH}

[ ! -d ${PSTORE_BACKUP_PATH} ] && mkdir -p ${PSTORE_BACKUP_PATH}

if [ ! -z "$(find ${PSTORE_PATH}  -type f)" ];
then
    echo "${PSTORE_PATH} is not empty! Moving content to ${PSTORE_BACKUP_PATH}"
    mv ${PSTORE_PATH}/* ${PSTORE_BACKUP_PATH}
fi

echo "Umount pstore from chroot!"
umount ${PSTORE_PATH}
