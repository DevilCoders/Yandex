#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/gnu"
HOST="ftp.nluug.nl"
MODULE="gnu"

REMOTE_TRACE="mirror-updated-timestamp.txt"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
