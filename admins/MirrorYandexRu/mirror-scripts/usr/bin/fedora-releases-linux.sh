#!/bin/bash

# RSYNC_PASSWORD="GC1WwPld" rsync yandexrunet@mirrors.kernel.org::t2fedora-enchilada

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="fedora/linux/releases/"
HOST="mirrors.kernel.org"
MODULE="t2fedora-enchilada/linux/releases/"

export RSYNC_PASSWORD="GC1WwPld"
REPO_USER="yandexrunet"

loadscripts
check4run
check4available
simple_sync
forcesetstamp
compresslog
