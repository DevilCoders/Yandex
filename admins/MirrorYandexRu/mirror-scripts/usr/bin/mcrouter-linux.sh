#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/mcrouter"
HOST="facebook.github.io"
SYNC_REPO_PORT="80"

mkdir -p /mirror/$REL_LOCAL_PATH/

loadscripts
check4run
check4available

for i in $(echo "bionic"); do
    debmirror --i18n --arch=amd64 --dist=$i --host=$HOST/mcrouter/debrepo --root=$i \
	    --method=https --section=contrib --nosource --ignore-missing-release \
	    --ignore-release-gpg --no-check-gpg --progress /mirror/$REL_LOCAL_PATH/$i/
done >> $LOGFILE 2>&1

forcesetstamp
compresslog
