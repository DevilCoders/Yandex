#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="opensuse"
HOST="stage.opensuse.org"
MODULE="opensuse-full-with-factory-dvd5/opensuse/"

EXCLUDE="
--exclude=packman/
--exclude=repositories/
--exclude=10.?/
--exclude=11.1/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog

