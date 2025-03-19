#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="ubuntu-cdimage"
HOST="mirror.nl.leaseweb.net"
MODULE="ubuntu-cdimage"

SPECIAL_PARAM="--delete-excluded"

EXCLUDE="
--exclude=daily
--exclude=daily-live
--exclude=ports
--exclude=dvd
--exclude=non-ports
--exclude={/ubuntu-server/,/cdicons/,/custom/,/dvd/,/gobuntu/,/livecd-base/,/moblin/,/ports/,/tocd3.1/,/tocd3/,/vmware/}
--exclude=daily-preinstalled/
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
