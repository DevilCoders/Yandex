#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="archlinux"
HOST="ftp.hosteurope.de"
MODULE="archlinux"

UPDATE="0"

REMOTE_TRACE="lastsync"

EXCLUDE="
--exclude=/gnome/
--exclude=/images/
--exclude=/release/
--exclude=/i586/
--exclude=/0.*/
--exclude=archive
"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    setstamp
fi

compresslog
