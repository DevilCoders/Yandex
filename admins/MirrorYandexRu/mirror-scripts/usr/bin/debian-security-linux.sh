#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="debian-security"
HOST="ftp.de.debian.org"
MODULE="debian-security"

REMOTE_TRACE="project/trace/security-master.debian.org"
REMOTELOCK="Archive-Update-in-Progress*"

loadscripts
check4run
check4available
check4remotelock
rsync_check

if [ "$RUNSYNC" == "1" ]; then

    touch $FULL_LOCAL_PATH/Archive-Update-in-Progress-mirror.yandex.ru
    debian_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH/project/trace/mirror.yandex.ru
    rm -f $FULL_LOCAL_PATH/Archive-Update-in-Progress-mirror.yandex.ru
    setstamp
fi

compresslog
