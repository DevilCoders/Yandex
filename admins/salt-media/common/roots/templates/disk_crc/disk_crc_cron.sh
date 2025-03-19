#!/bin/bash

rm -rf /tmp/disk_crc_old 2>/dev/null
mv /tmp/disk_crc /tmp/disk_crc_old 2>/dev/null
mkdir -p /tmp/disk_crc 2>/dev/null
for i in `ls /dev/sd* | grep -v '[0-9]'`; do
SN=`echo $i | sed 's_/dev/__g'`
/usr/sbin/smartctl -a $i | grep UDMA_CRC | awk '{print $NF}' > /tmp/disk_crc/$SN
done

