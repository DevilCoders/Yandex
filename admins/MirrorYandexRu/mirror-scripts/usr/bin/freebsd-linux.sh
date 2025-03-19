#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="freebsd"
HOST="ftp3.de.FreeBSD.org"
MODULE="FreeBSD"

REMOTE_TRACE="TIMESTAMP"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

if [ -d "$MAINSYNCDIR/pub" -a ! -L "$MAINSYNCDIR/pub/FreeBSD" ]; then
    cd $MAINSYNCDIR/pub
    ln -s ../freebsd FreeBSD
fi

compresslog
