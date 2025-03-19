#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="openbsd"
HOST="ftp.eu.openbsd.org"
MODULE="OpenBSD"

REMOTE_TRACE="timestamp"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

if [ -d "$MAINSYNCDIR/pub" -a ! -L "$MAINSYNCDIR/pub/OpenBSD" ]; then
    cd $MAINSYNCDIR/pub
    ln -s ../openbsd OpenBSD
fi

compresslog
