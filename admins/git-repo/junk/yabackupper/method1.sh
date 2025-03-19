#!/bin/bash

my_config="/root/rdiff-dev/config.file"

echo $my_config

set -x

. ${my_config}

get_lxc_vms_list () {
    for i in `ls /etc/lxctl/ | grep -v lxctl.yaml | sed 's/\.yaml//g'`; do
	echo $i; 
    done
}
    
for i in $(get_lxc_vms_list); do 
    flock -n /tmp/lock.lock -c "/root/rdiff-dev/yabackupper_lxc_one_vm_backup.sh ${my_config} $i /var/lxc/root/${i}";
done


