#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="debian-cd"
HOST="cdimage.debian.org"
MODULE="debian-cd"

EXCLUDE="--exclude=7.4.0/7.4.0-live"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
