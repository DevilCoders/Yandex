#!/usr/bin/env bash

base_path=`dirname "${BASH_SOURCE[0]}"`

$base_path/utils/common/dump_hostsdata.py -i dc,invnum,name,model,memory,ssd,disk,switch,queue,rack,gpu_models,gpu_count,ffactor,net "$@" | sort
