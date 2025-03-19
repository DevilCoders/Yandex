#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REPO_USER="rsuser"
REL_LOCAL_PATH="mirrors/MX-Linux/MX-Packages"
HOST="iso.mxrepo.com"
MODULE="workspace"

export RSYNC_PASSWORD=T1tpw4rstmr

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
