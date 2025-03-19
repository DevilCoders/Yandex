#!/bin/bash
#
# Provides: disk_crc

me=${0##*/}    # strip path
me=${me%.*}    # strip extension

die() {
    echo "$1;$2"
    exit 0
}

for i in `find /tmp/disk_crc/ -type f 2>/dev/null`
do
    crc=`cat $i`;
    sn=`echo $i | sed 's-/tmp/disk_crc/--g'`;
    crc_old=`cat /tmp/disk_crc_old/$sn 2>/dev/null`;

    if [ -z "$crc" ]; then
        crc=0
    fi

    if [ -z "$crc_old" ]; then
        crc_old=$crc
    fi

    if [ "$crc" -gt "$crc_old" ] 2>/dev/null
    then
        die "1" "UDMA_CRC event found for $sn (now:$crc, was:$crc_old)";
    fi
done

die "0" "UDMA_CRC for all disks is OK";
exit 0;
