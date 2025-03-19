#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="opensuse/repositories"
HOST="pull-mirror.yandex.net"
MODULE="full-archive/$REL_LOCAL_PATH"

loadscripts
check4run
check4available

$RSYNC_OPT $RSYNC_REPO $FULL_LOCAL_PATH > $LOGFILE 2>&1

compresslog
