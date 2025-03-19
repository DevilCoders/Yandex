#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REPO_USER="gentoo"
REL_LOCAL_PATH="gentoo-distfiles"
HOST="masterdistfiles.gentoo.org"
MODULE="gentoo"

SPECIAL_PARAM="--numeric-ids"
EXCLUDE="
--exclude=/experimental/bittorrent-http-seeding/
--exclude=/releases/historical
--exclude=THIS-FILE-SHOULD-NOT-BE-PUBLIC.txt
"

export RSYNC_PASSWORD=vidgeryd

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
