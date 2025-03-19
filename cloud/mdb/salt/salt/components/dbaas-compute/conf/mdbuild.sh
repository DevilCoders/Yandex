#!/bin/sh

set -e

to_log() {
    echo "$(date +'%Y-%m-%d %H:%M:%S') [$$] $1"
}

TARGET="/dev/md0"
DIRTY_DISKS_FILE="/etc/dirty_disks"

to_log "Start script"

if ! /bin/ls "${TARGET}" >/dev/null 2>&1
then
    to_log "$TARGET doesn't exist. It will be created."
    NUM_DEVICES=$(grep nvme-disk /sys/block/vd*/serial 2>/dev/null | cut -d/ -f4 | sed 's/^/\/dev\//g' | /usr/bin/wc -l)
    DEVICES=$(grep nvme-disk /sys/block/vd*/serial 2>/dev/null | cut -d/ -f4 | sed 's/^/\/dev\//g' | /usr/bin/xargs echo)

    # We filter all "used" devices here (all devices with partitions, filesystems or raids)
    DIRTY_DISKS=$(/sbin/blkid -p -u raid -o device ${DEVICES} | /usr/bin/xargs echo)
    if [ "${DIRTY_DISKS}" != "" ]; then
        to_log "ERROR! There are dirty nvme disks: ${DIRTY_DISKS}. Looks like this is a foreign disk. You have to ask COMPUTE (read MDB-11608 for more information). Exiting."
        echo "${DIRTY_DISKS}" > ${DIRTY_DISKS_FILE}
        exit 1
    fi

    if [ "${NUM_DEVICES}" -gt 1 ]
    then
        to_log "Creating new md device with ${NUM_DEVICES} devices: ${DEVICES}"
        /usr/bin/yes | /sbin/mdadm --create "${TARGET}" --force --chunk=64 --level=0 --raid-devices="${NUM_DEVICES}" ${DEVICES}
        /bin/cat <<EOF > /etc/mdadm/mdadm.conf
CREATE owner=root group=disk mode=0660 auto=yes
HOMEHOST <system>
MAILADDR root
$(/sbin/mdadm --examine --scan | /bin/sed 's/\/dev\/md\/0/\/dev\/md0/g')
EOF
        /usr/sbin/update-initramfs -u -k all
    else
        to_log "There are only ${NUM_DEVICES} devices: ${DEVICES}. Nothing to create."
    fi
else
    to_log "$TARGET already exists."
fi

to_log "Finish script"
