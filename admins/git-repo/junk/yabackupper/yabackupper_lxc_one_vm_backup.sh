#!/bin/bash 

my_config=$1

echo $my_config

set -x

. ${my_config}

vmname=$2
vmdir=$3
${rdiff_binary} ${rdiff_opts} --exclude-globbing-filelist ${files_exclude_list} ${vmdir} "${remote_host} ${ssh_options} -i ${ssh_key}"::${storage}/${vmname}

