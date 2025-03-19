#!/bin/bash

die () {
        echo "$1;$2"
        exit 0
}

. /usr/local/sbin/autodetect_environment

        if [ $is_virtual_host -eq 1 ] && [ $is_openvz_host -eq 1 ] ; then
                die 0 "OK, openvz CT, skip ncq checking"
        fi

        if [ $is_virtual_host -eq 1  ] && [ $is_lxc_host -eq 1 ]; then
                die 0 "OK, lxc CT, skip ncq checking"
        fi

        if [ $is_virtual_host -eq 1  ] && [ $is_kvm_host -eq 1 ]; then
                die 0 "OK, kvm CT, skip ncq checking"
        fi
        
		if [ $is_virtual_host -eq 1  ]; then
                die 0 "OK, virtual CT, skip ncq checking"
        fi

for HDD in `ls -l /sys/block/sd*/device/queue_depth |awk '{print $NF}' |cut -f4 -d/`; do 
        ncqcount=$(cat /sys/block/$HDD/device/queue_depth)
        size=$(cat /sys/block/$HDD/size)
        if [ $size -gt 0 -a $ncqcount -eq 1 ]; then
                echo "2; Ncq disable"
                exit;
        fi
done
echo "0; OK"
