#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mopslinux"
HOST="ftp.rpunet.ru"
MODULE="ftp"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
