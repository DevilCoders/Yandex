#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="ubuntu-cloud-images"
HOST="cloud-images.ubuntu.com"
MODULE="cloud-images"

REMOTE_TRACE=".trace/cloud-images-banjo"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    setstamp
fi

compresslog
