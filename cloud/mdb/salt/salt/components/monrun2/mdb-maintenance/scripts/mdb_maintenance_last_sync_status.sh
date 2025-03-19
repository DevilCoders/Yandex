#!/usr/bin/env bash

status_file='/var/run/mdb-maintenance-sync/last-exit-status'

# executed on different node
if [[ ! -e $status_file ]]
then
    echo '0;OK'
    exit 0
fi


last_status=$(cat "$status_file")

# sync success
if [[ "$last_status" == "0" ]]
then
    echo '0;OK'
    exit 0
fi

last_errors=$(tail -n1 /var/log/mdb-maintenance/mdb-maintenance-sync.log | tr '\n' '.')
echo "2;CRIT;Last sync exit with code: $last_status. Last errors: $last_errors"
