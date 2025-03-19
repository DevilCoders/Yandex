#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="linuxmint-packages"
HOST="rsync-packages.linuxmint.com"
MODULE="packages"

REMOTE_TRACE="db/checksums.db"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then

    touch $FULL_LOCAL_PATH/Archive-LinuxMint-in-Progress-mirror.yandex.ru
    debian_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH/.mirror.yandex.ru
    rm -f $FULL_LOCAL_PATH/Archive-LinuxMint-in-Progress-mirror.yandex.ru
    setstamp
fi

compresslog
