#!/bin/bash

my_config=$1

. /usr/lib/yabackupper/procedures.sh
clear_vars
. ${my_config}

get_lxc_vms_list () {
    for i in `ls /etc/lxctl/ | grep -v lxctl.yaml | sed 's/\.yaml//g'`; do
	echo $i; 
    done
}

check_is_vm_excluded () {
    vmname=$1
    excluded=2
    cat ${vms_exclude_list} | grep ${vmname} 1>/dev/null 2>&1 && excluded=1 || excluded=0 
    if [[ $excluded == "1" ]]; then 
	echo "[INFO] ${vmname} is excluded for this config" | logger; return 1 
    elif [[ $excluded == "0" ]]; then 
	echo "[INFO] ${vmname} is not excluded for this config" | logger ; return 0
    else
	echo "[ERROR] There are strange error with check_is_vm_excluded() for ${vname}, so i will backup it"
    fi
}

run_lxc_vms_backup () {
    for i in $(get_lxc_vms_list); do 
	lockname=/tmp/yabackupper-lxcvm-${i}.lock;
	echo "[INFO] starting to backup lxc-vm $i" | logger;
	if [[ $parallel == "yes" ]]; then
	    check_is_vm_excluded $i && flock -n $lockname -c "/usr/bin/yabackupper_lxc_onevm_backup.sh ${my_config} $i /var/lxc/root/${i}" & 
	else 
	    check_is_vm_excluded $i && flock -n $lockname -c "/usr/bin/yabackupper_lxc_onevm_backup.sh ${my_config} $i /var/lxc/root/${i}" ; 
	fi
    done
}

run_ovz_vms_backup () {
    vzlist -a -o ctid,hostname -H 2>/dev/null || vzlist -a -o veid,hostname | while read "VEID" "hostname"; do
	lockname=/tmp/yabackupper-ovzvm-${VEID}.lock;
	echo "[INFO] starting to backup openvz-vm ${VEID}: ${hostname}";
	# TODO: gather VE_PRIVATE from vz.conf
	VE_PRIVATE="/var/lib/vz/private/";
	echo ${VE_PRIVATE};
        if [[ $parallel == "yes" ]]; then
            check_is_vm_excluded ${hostname} && flock -n $lockname -c "/usr/bin/yabackupper_ovz_onevm_backup.sh ${my_config} ${hostname} ${VE_PRIVATE}${VEID}" & 
	else
	    check_is_vm_excluded ${hostname} && flock -n $lockname -c "/usr/bin/yabackupper_ovz_onevm_backup.sh ${my_config} ${hostname} ${VE_PRIVATE}${VEID}" ;
	fi
    done
}

check_islxc () {
    if [ -f /usr/bin/lxctl ]; then
	echo "[INFO] It seems, that i am LXC host. Starting to backup lxc-vms." | logger
	run_lxc_vms_backup
    else echo "[INFO] It seems, that i am not lxc-host" | logger
    fi 
}

check_isovz () {
    if [ -f /usr/sbin/vzctl ]; then
	echo "[INFO] It seems, that i am OVZ host. Starting to backup ovz-vms." | logger
	run_ovz_vms_backup
    else echo "[INFO] It seems, that i am not ovz-host" | logger
    fi 
}    

check_islxc
check_isovz

