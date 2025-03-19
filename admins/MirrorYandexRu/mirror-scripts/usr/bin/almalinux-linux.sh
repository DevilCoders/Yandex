#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="almalinux"
HOST="repo-us.almalinux.org"
MODULE="almalinux"

REMOTE_TRACE="TIME"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
