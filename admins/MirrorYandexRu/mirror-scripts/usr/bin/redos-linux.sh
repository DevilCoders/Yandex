#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REPO_USER="mirror"
REL_LOCAL_PATH="redos"
HOST="repo.red-soft.ru"
MODULE="mirror"
REMOTE_TRACE=".TIME"

export RSYNC_PASSWORD=YandeX

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
