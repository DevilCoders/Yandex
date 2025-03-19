#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="gentoo-portage"
HOST="rsync.de.gentoo.org"
MODULE="gentoo-portage"

SPECIAL_PARAM="--numeric-ids"

EXCLUDE="
--exclude=/releases/historical/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
