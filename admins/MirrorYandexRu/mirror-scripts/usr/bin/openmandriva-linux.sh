#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="openmandriva"
HOST="pull-mirror.yandex.net"
MODULE="full-archive/$REL_LOCAL_PATH"

REMOTE_TRACE=".mirror.yandex.ru"

loadscripts
check4run
check4available
rsync_check
slave_sync
compresslog

