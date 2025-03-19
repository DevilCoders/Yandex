#!/bin/bash

# RSYNC_PASSWORD="GC1WwPld" rsync yandexrunet@mirrors.kernel.org::t2fedora-enchilada

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="fedora-secondary"
HOST="ftp-stud.hs-esslingen.de"
MODULE="fedora-secondary"

EXCLUDE="
--exclude=15/
--exclude=16/
--exclude=17/
--exclude=18/
--exclude=19/
--exclude=20/
--exclude=21/
--exclude=22/
--exclude=23/
--exclude=24/
--exclude=25/
--exclude=25-*/
--exclude=25_*/
--exclude=lasttransfer"

REMOTE_TRACE="fullfilelist"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync
    setstamp
fi

compresslog
