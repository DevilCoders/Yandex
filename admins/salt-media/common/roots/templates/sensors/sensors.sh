#!/bin/bash

mkdir /var/tmp/disk_temp 2>/dev/null; ls /dev/sd* | grep -v '[0-9]' | sed 's/\/dev\///g' 2>/dev/null | while read BlockDevice; do /usr/sbin/hddtemp -n /dev/$BlockDevice > /var/tmp/disk_temp/disk_temp_$BlockDevice; done; /usr/bin/ipmitool sensor 2>&1 > /var/tmp/ipmi_sensor

