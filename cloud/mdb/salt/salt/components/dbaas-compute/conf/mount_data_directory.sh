#!/bin/sh

set -xe

if [ $# -ne 2 ]
then
    /bin/echo "Usage: $(/usr/bin/basename "$0") <directory> <large-disk-mkfs-options>"
    exit 1
fi

TARGET="$1"
LARGE_DISK_MKFS_OPTIONS="$2"

LOG_FILE=/var/log/mount_data_directory.log
DIRTY_DISKS="/etc/dirty_disks"

to_log() {
    echo "$(date +'%Y-%m-%d %H:%M:%S') [$$] $1" >> $LOG_FILE
}

LABEL="$(/bin/hostname -f | /usr/bin/md5sum | /usr/bin/head -c 16)"

to_log "Start script"

if [ -f ${DIRTY_DISKS} ]; then
    to_log "ERROR! Looks like there are some dirty disks: $(/bin/cat ${DIRTY_DISKS}). You have to ask COMPUTE (read MDB-11608 for more information). Exiting."
    exit 3
fi

if /bin/grep -q md0 /proc/mdstat
then
    PARTITION="/dev/md0"
    to_log "$PARTITION will be used as data device"
else
    BOOT_DEVICE="$(/bin/mount | /usr/bin/cut -f 1,3 -d ' ' | /bin/grep -E '\ /$' | /usr/bin/cut -d ' ' -f1 | /bin/sed 's/[0-9]*//g')"
    DATA_DEVICE="$(/bin/ls /dev/vd* | /bin/grep -v "${BOOT_DEVICE}" | /bin/sed 's/[0-9]*//g' | /usr/bin/sort -u)"
    to_log "Boot device: $BOOT_DEVICE, data device: $DATA_DEVICE"
    PARTITION="${DATA_DEVICE}1"
    if ! /bin/ls "$PARTITION"
    then
        to_log "Partition $PARTITION doesn't exist. It will be created on $DATA_DEVICE."
        to_log "Run mklabel gpt."
        /sbin/parted --script "${DATA_DEVICE}" mklabel gpt
        to_log "Run mkpart primary."
        /sbin/parted --script "${DATA_DEVICE}" mkpart primary 0% 100%
        to_log "Run partprobe"
        /sbin/partprobe "${DATA_DEVICE}"
    fi

    PARTITION_MAX_WAIT=30
    for try in $(seq $PARTITION_MAX_WAIT)
    do
        ls "$PARTITION" && break
        test "$try" -lt $PARTITION_MAX_WAIT || exit 1
        to_log "$PARTITION does not exist yet, waiting for it. Current try is $try"
        sleep 1
    done

    to_log "$PARTITION exists"
fi

if ! /sbin/blkid -p "${PARTITION}" | /bin/grep -q ext4
then
    to_log "create fs on $PARTITION"
    SIZE="$(/sbin/blockdev --getsize64 "$PARTITION")"
    to_log "$PARTITION size $SIZE"
    # 1099511627776 is 1TB
    if [ "$SIZE" -ge 1099511627776 ]
    then
        to_log "initialize $PARTITION with $LARGE_DISK_MKFS_OPTIONS"
        /sbin/mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0,discard ${LARGE_DISK_MKFS_OPTIONS} "${PARTITION}"
    else
        to_log "initialize $PARTITION ext4 defaults"
        /sbin/mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0,discard "${PARTITION}"
    fi
    to_log "Create label $LABEL on $PARTITION."
    /sbin/tune2fs -L "$LABEL" "$PARTITION"
else
    to_log "$PARTITION already has ext4 fs. Check labels."
    PART_LABEL="$(/sbin/blkid $PARTITION -s LABEL -o export | /bin/grep LABEL | /usr/bin/cut -d '=' -f 2)"

    if [ -z "$PART_LABEL" ]; then
        to_log "Warning! There is no label on $PARTITION. Either this is a foreign disk, or it is just first run on an old one. Create label $LABEL."
        /sbin/tune2fs -L "$LABEL" "$PARTITION"
    elif [ "$LABEL" != "$PART_LABEL" ]; then
        to_log "ERROR! Label on $PARTITION ($PART_LABEL) does not equal expected label ($LABEL). Looks like this is a foreign disk. You have to ask COMPUTE (read MDB-11608 for more information). Exiting."
        exit 2
    else
        to_log "Label on $PARTITION equal expected $LABEL"
    fi
fi

RESTORE=0

if /bin/ls "${TARGET}"/* >/dev/null 2>&1
then
    RESTORE=1
    /bin/mkdir -p /tmp/backupdata
    /bin/mv "${TARGET}"/* /tmp/backupdata
fi

if ! /bin/grep -q "${TARGET}" /etc/fstab
then
    to_log "Add $PARTITION to fstab"
    /bin/echo "$(/sbin/blkid "${PARTITION}" -s UUID -o export | grep UUID) ${TARGET} ext4 noatime,nodiratime,errors=remount-ro 0 1" >> /etc/fstab
else
    to_log "fstab already contains $TARGET"
fi

/bin/mount "${TARGET}"

if [ "${RESTORE}" = "1" ]
then
    to_log "Restore data from /tmp/backupdata/"
    mv /tmp/backupdata/* "${TARGET}/" && /bin/rmdir /tmp/backupdata
fi

if ! test "$(/usr/bin/stat -c %Y /boot/initrd.img* | /usr/bin/sort -gu | /usr/bin/head -n 1)" -gt "$(/usr/bin/stat -c %Y /etc/mdadm/mdadm.conf)"
then
    to_log "Run update-initramfs"
    /usr/sbin/update-initramfs -u -k all
fi

to_log "Finish script"
