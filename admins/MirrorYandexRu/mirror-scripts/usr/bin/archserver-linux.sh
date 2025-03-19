#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="archserver"
HOST="ftp5.gwdg.de"
MODULE="pub/linux/archserver/"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
