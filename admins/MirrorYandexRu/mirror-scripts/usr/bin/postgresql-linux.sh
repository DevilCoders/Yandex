#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/postgresql"
HOST="apt.postgresql.org"
MODULE="pgsql-ftp/repos/apt/"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
