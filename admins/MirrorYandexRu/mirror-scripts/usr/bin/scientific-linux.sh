#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="scientificlinux"
HOST="ftp.halifax.rwth-aachen.de"
MODULE="scientific"

EXCLUDE="
--exclude=4*/
--exclude=5*/
--exclude=documents/
--exclude=graphics/
--exclude=livecd/
--exclude=virtual.images/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
