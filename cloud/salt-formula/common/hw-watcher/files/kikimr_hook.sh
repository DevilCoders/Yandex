#!/bin/sh

DISK_LABELS_PATH="/dev/disk/by-partlabel"
LABEL_TO_FORMAT=""
HOSTNAME="$(hostname -f)"

mail_send() {

    echo "$1" | mail -s "kikimr hw-watcher $HOSTNAME" staerist@yandex-team.ru

}

case $1 in
    "None"|"")
        echo "Disk not found"
        mail_send "Disk not found"
        exit 0;
    ;;
    *)
        DISK="$(echo $1 | sed -e 's/\/dev\///g')"
    ;;
esac

/usr/bin/partx -u /dev/$DISK
/sbin/partprobe
/bin/udevadm settle

UDEV_DISKS_LIST="$(ls ${DISK_LABELS_PATH} | grep KIKIMR)"
SGDISK_DISKS_LIST="$(for DSK in `ls /dev/|egrep "sd[a-z]$|nvme[0-3]n1$"`;do sgdisk -p /dev/${DSK}|awk '/KIKIMR/ {print $7}';done | sort)"

if [ "$UDEV_DISKS_LIST" != "$SGDISK_DISKS_LIST" ]; then
    echo "Problem with labels"
    mail_send "Problem with labels"
    exit 0;
fi

/sbin/partprobe

for FIND_LABEL_TO_FORMAT in ${UDEV_DISKS_LIST}; do
    if readlink ${DISK_LABELS_PATH}/${FIND_LABEL_TO_FORMAT} | grep -q ${DISK}; then
        LABEL_TO_FORMAT="${FIND_LABEL_TO_FORMAT}"
        break
    fi
done

echo "disk for format: ${LABEL_TO_FORMAT}"

if [ -z "${LABEL_TO_FORMAT}" ]; then
    echo "label to format not found"
    mail_send "label to format not found"
    exit 0;
fi

echo "Cleanup disk"
dd if=/dev/zero of=${DISK_LABELS_PATH}/${LABEL_TO_FORMAT} bs=1M count=1 || true
echo "Restart kikimr"
service kikimr stop || true
service kikimr start || true
