#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="epel"
HOST="ftp-stud.hs-esslingen.de"
MODULE="fedora-epel"

REMOTE_TRACE="fullfilelist"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    setstamp
fi

compresslog
