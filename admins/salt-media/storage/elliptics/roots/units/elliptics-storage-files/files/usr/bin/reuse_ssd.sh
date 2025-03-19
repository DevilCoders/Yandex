#!/bin/bash

PATH="${PATH}:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/sbin:/usr/local/bin"

dry_run="yes"
if [ ! -z $1 ] && [ $1 == "-n" ]; then
    shift
else
    dry_run="no"
fi

dryrun () {
    if [ $dry_run == "no" ]; then
        "$@"
    else
        log "dry run: $@"
        true # set the $? to 0
    fi
}

ctm="date +'%Y/%m/%d %H:%M:%S'"
log() {
    echo -e "$(eval $ctm)\t$@"
}

cache=`df -h |grep -P "(/place/cocaine-isolate-daemon/ssd4app|/cache)" |grep -v storage |grep -P "/dev/sd\w+" -o | sort | uniq`
if [ ! -z $cache ]
then
    if mount | grep -q /place/cocaine-isolate-daemon/ssd4app; then
        # obsolete configuration
        dryrun ubic stop cocaine-runtime
        dryrun ubic stop cocaine-isolate-daemon
        dryrun portoctl gc
        dryrun eval "timeout 300 service yandex-porto stop || true"
        dryrun umount /place/cocaine-isolate-daemon/ssd4app
        dryrun eval "timeout 300 service yandex-porto start || true"
        dryrun ubic start cocaine-isolate-daemon
        dryrun ubic start cocaine-runtime
    fi

    dryrun umount $cache
    dryrun sed -i '/ \/cache /d' /etc/fstab
    dryrun sed -i '/\/place\/cocaine-isolate-daemon\/ssd4app/d' /etc/fstab
    dryrun rm -rf /cache
fi

# mount SSD
disks=($(ls /dev/sd*[a-z]))
max_rootdir=`ls /srv/storage/ -1 |grep -P "\d+" | sort -n | tail -n1`
create_backends=0

for d in ${disks[*]}
do
    grep -q "$d " /etc/mtab && continue
    name=${d/\/dev\//}
    if [ $(cat /sys/block/$name/queue/rotational) -eq 0 ]
    then
        ((max_rootdir++))
        dryrun mkfs.ext4 -F -m0 -E lazy_itable_init=0,lazy_journal_init=0 $d || exit 1
        uuid=$(blkid -o value -s UUID $d)
        echo "UUID=$uuid /srv/storage/$max_rootdir ext4 noatime,defaults,nofail,nobootwait,errors=remount-ro 0 2" >> /etc/fstab
        dryrun mkdir -p /srv/storage/$max_rootdir
        dryrun mount /srv/storage/$max_rootdir || exit 1
        create_backends=1
    fi
done

if [ $create_backends -eq 1 ]
then
    mr_proper.py --create-backends
fi
