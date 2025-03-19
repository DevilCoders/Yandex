#! /usr/bin/env bash

CN=$1
TEAM=$2
EMAIL=$3
#LOCK_FILE="/tmp/yaroot-create-p12.lock"
LOCK_NAME="revoke.lock"

function check_args() {
    #log_info "check arguments..."
    if [[ -z $CN ]]; then
    #    log_err "arguments too low" && exit 1
    exit 1
    fi
    #log_ok
}
check_args

zk-flock -w 7200 $LOCK_NAME "/usr/bin/yaroot-real-revoke-cert.sh \"$CN\""

