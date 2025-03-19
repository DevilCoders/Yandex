#!/usr/bin/env bash

cluster_type=$1
status_file="/var/run/mdb-backup/scheduler/${cluster_type}-last-exit-status"
log_file="/var/log/mdb-backup/scheduler-${cluster_type}.log"

if [[ ! -e $status_file ]]; then
    echo '0;OK'
    exit 0
fi


last_status=$(cat "$status_file")

if [[ "$last_status" == "0" ]]; then
    echo '0;OK'
    exit 0
elif [[ "$last_status" == "42" ]]; then
    echo '0;Lock was not acquired'
    exit 0;
fi

last_error=$(grep ERROR ${log_file} | grep -oP 'error": "\K[^"]+')
echo "2;CRIT;Last sync exit with code: $last_status. Last errors: $last_error"
