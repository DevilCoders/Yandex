#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

rsync://msync.rockylinux.org/rocky/mirror/pub/rocky/

REL_LOCAL_PATH="rockylinux"
HOST="msync.rockylinux.org"
MODULE="rocky/mirror/pub/rocky"

REMOTE_TRACE="fullfilelist"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
