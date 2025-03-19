#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/parabola"
HOST="repo.parabolagnulinux.org"
MODULE="repos"
SYNC_REPO_PORT="875"

SPECIAL_PARAM="--port=$SYNC_REPO_PORT"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
