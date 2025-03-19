#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin

die() {
    echo "$1;$2"
    exit 0
}

if [ -f /usr/local/sbin/autodetect_environment ] ; then
    is_virtual_host=0
    . /usr/local/sbin/autodetect_environment >/dev/null 2>&1 || true
    if [ "$is_virtual_host" -eq 1 ] ; then
        die 0 "OK"
    fi
fi

if grep 'model name' /proc/cpuinfo | grep -q 'KVM'
then
    die 0 "OK"
elif grep -q 'Booting paravirtualized kernel on KVM' /var/log/dmesg 2>/dev/null
then
    die 0 "OK"
fi

micron500_bad_fw=$(for c in $(sudo /usr/bin/lsscsi | grep Micron_M500DC_MT | awk '{print $6}'); do sudo /usr/sbin/smartctl -i $c | grep Firmware  ; done | grep -vc 0144)
micron5100_bad_fw=$(for c in $(sudo /usr/bin/lsscsi | grep Micron_5100 | awk '{print $6}'); do sudo /usr/sbin/smartctl -i $c | grep Firmware  ; done | grep -vc 'D0MU071')
micron5200_bad_fw=$(for c in $(sudo /usr/bin/lsscsi | grep Micron_5200 | awk '{print $6}'); do sudo /usr/sbin/smartctl -i $c | grep Firmware  ; done | grep -vc 'D1MU020')
sandisk_bad_fw=$(for c in $(sudo /usr/bin/lsscsi | grep SDLF1CRR-019T | awk '{print $6}'); do sudo /usr/sbin/smartctl -i $c | grep Firmware  ; done | grep -vc 'ZR11RPA1')

if [ $micron500_bad_fw -ne 0 ]
then
   die 1 "${micron500_bad_fw} MICRON M500 disks have fw not equal 0144"
elif [ $micron5100_bad_fw -ne 0 ]
then
   die 1 "${micron5100_bad_fw} MICRON M5100 disks have fw not equal D0MU071"
elif [ $micron5200_bad_fw -ne 0 ]
then
   die 1 "${micron5200_bad_fw} MICRON M5200 disks have fw not equal D1MU020"
elif [ $sandisk_bad_fw -ne 0 ]
then
   die 1 "${sandisk_bad_fw} SANDISK disks have fw not equal ZR11RPA1"
else
   die 0 "OK"
fi
