#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="altlinux"
HOST="rsync.altlinux.org"
MODULE="ALTLinux"

EXCLUDE="
--exclude=2.*/
--exclude=3.0/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
