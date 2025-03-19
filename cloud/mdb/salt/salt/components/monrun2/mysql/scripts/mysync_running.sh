#!/bin/bash

die () {
    echo "$1;$2"
    exit 0
}

mysync_id=$( ps aux | grep mysync | grep -v 'grep' | grep -v 'mysync_check' | awk '{print $2}')
if [[ -z "$mysync_id" ]]; then
    die 2 'mysync is dead'
else
    restarts=$(systemctl show mysync -p NRestarts)
    tokens=(${restarts//=/ })
    if [[ ${tokens[1]} -ne 0 ]]; then
        msg="mysync has restarted ${tokens[1]} times"
        die 2 "$msg"
    else
        die 0 'OK'
    fi
fi