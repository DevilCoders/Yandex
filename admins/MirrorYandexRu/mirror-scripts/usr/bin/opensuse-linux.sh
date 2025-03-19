#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="opensuse"
HOST="ftp.lysator.liu.se"
MODULE="pub/opensuse"

REMOTE_TRACE=".gwdgmirror"

EXCLUDE="
--exclude=FOSDEM/
--exclude=discontinued/
--exclude=education/
--exclude=packman/
--exclude=ports/
--exclude=project/
--exclude=repositories/
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
