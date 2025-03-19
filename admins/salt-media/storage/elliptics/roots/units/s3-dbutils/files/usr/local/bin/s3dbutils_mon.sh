{%- set status_dir = salt['pillar.get']('s3-dbutils:dirs:status_dir', []) -%}
#!/bin/bash

func_name=$1
status_dir={{ status_dir }}

check_status_file() {
    local status_file=$1
    
    if [[ ! -e ${status_file} ]]
    then
        echo "2;Did not run"
        return
    fi

    local mtime=`date +%s -r ${status_file}`
    local now=`date +%s`
    local diff=`echo "${now} - ${mtime}" | bc`
    local max_diff=$2

    if [[ ${diff} -gt ${max_diff} ]]
    then
        echo "2;Status file too old"
    else
        cat ${status_file}
    fi
}

check_status() {
    local script_name=$2
    local threshold=$3
    while read file
    do
        local out=`check_status_file "${status_dir}/${script_name}/${file}" ${threshold}`
        local status=`echo $out | awk -F";" '{print $1}'`
        local message=`echo $out | awk -F";" '{print $2}'`
        local crit=''
        local warn=''
        case "${status}" in
            1 ) warn="${warn}[${file}]: ${message}, " ;; 
            2 ) crit="${crit}[${file}]: ${message}, " ;;
        esac 
    done < <( ls ${status_dir}/${script_name}/ )
    
    if [[ ! -z ${crit} ]]
    then
      echo "2;${crit:0:100}"
    elif [[ ! -z $warn ]]
    then
      echo "1;${warn:0:100}"
    else
      echo "0;OK"
    fi
}

${func_name} $@
