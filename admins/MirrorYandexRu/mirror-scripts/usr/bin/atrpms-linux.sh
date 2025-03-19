#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="fedora/atrpms"
HOST="ftp.tu-chemnitz.de"
MODULE="ftp/pub/linux/ATrpms"

EXCLUDE="
--exclude=/*/redhat/
--exclude=/src/fc5*/
--exclude=/fc5*/
--exclude=/src/fc{6}-{i386,x86_64,ppc}/
--exclude=fc{6}-{i386,x86_64,ppc}
--exclude=/src/*/redhat/
--exclude=/debug/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
