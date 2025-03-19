#!/bin/bash

clear_vars () {
    ssh_key=""
    remote_host=""
    storage=""
    files_exclude_list=""
    parallel=""
    method=""
    rdiff_opts=""
    mail=""
    mailto=""
    rdiff_binary=""
    ssh_options=""
    vms_exclude_list=""
}

logger () {
    log_file=/var/log/yabackupper.log
    while read line; do
	echo "$(date +"[%d/%b/%Y:%H:%M:%S %z]") ${line}" >> ${log_file}; 
    done
}

wrong_method () {
    method=$1
    echo -e "Sorry, but method $method is not known for me. 
You can use this method for now: 
    yabackupper_dom0_allvms.sh"
}

