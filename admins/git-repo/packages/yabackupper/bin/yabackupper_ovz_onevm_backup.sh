#!/bin/bash

my_config=$1

. /usr/lib/yabackupper/procedures.sh
clear_vars
. ${my_config}

vmname=$2
vmdir=$3
echo "[INFO] starting to execute command: ${rdiff_binary} ${rdiff_opts} --exclude-globbing-filelist ${files_exclude_list} ${vmdir} "${remote_host} ${ssh_options} -i ${ssh_key}"::${storage}/$(hostname -f)/${vmname}-ovz" | logger
${rdiff_binary} ${rdiff_opts} --exclude-globbing-filelist ${files_exclude_list} ${vmdir} "${remote_host} ${ssh_options} -i ${ssh_key}"::${storage}/$(hostname -f)/${vmname}-ovz 2>&1 | logger

sleep 5
