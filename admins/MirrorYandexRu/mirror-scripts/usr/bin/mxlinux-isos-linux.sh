#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REPO_USER="downstreamtestuser"
REL_LOCAL_PATH="mirrors/MX-Linux/MX-ISOs"
HOST="rsync-mxlinux.org"
MODULE="MX-Linux"

export RSYNC_PASSWORD=dstuPwd

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
