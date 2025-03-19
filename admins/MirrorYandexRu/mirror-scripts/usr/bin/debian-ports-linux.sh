#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="debian-ports"
HOST="ftp.de.debian.org"
MODULE="debian-ports"

REMOTE_TRACE="project/trace/porta.debian.org"
REMOTELOCK="Archive-Update-in-Progress*"

loadscripts
check4run
check4available
check4remotelock
rsync_check

if [ "$RUNSYNC" == "1" ]; then

    touch $FULL_LOCAL_PATH/Archive-Update-in-Progress-mirror.yandex.ru
    debian_sync

    while true; do
        if [ $(find $FULL_LOCAL_PATH -maxdepth 1 -name "Archive-Update-in-Progress*" | wc -l ) != "0" ]; then
            date -u > $FULL_LOCAL_PATH/.waiting
            check4remotelock
            debian_sync
        else
            rm -f $FULL_LOCAL_PATH/.waiting
            break
        fi
    done

    echo "$(date -u)" > $FULL_LOCAL_PATH/project/trace/mirror.yandex.ru
    rm -f $FULL_LOCAL_PATH/Archive-Update-in-Progress-mirror.yandex.ru
    setstamp
    echo $(date -u) > $FULL_LOCAL_PATH/.mastersync
fi

compresslog
