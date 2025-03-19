#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/redcorelinux"
HOST="rsync.redcorelinux.org"
MODULE="files"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
