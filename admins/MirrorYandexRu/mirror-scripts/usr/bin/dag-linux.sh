#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="fedora/dag"
HOST="ftp.tu-chemnitz.de"
MODULE="ftp/pub/linux/dag"

REMOTE_TRACE="TIMESTAMP"

EXCLUDE="
--exclude=el3/
--exclude=el4/
--exclude=source/
"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
