#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/elastic"
HOST="artifacts.elastic.co"
SYNC_REPO_PORT="80"

loadscripts
check4run
check4available

for i in $(echo "6 7 8"); do
    debmirror --i18n --arch=amd64,i386 --dist=stable \
	--host=$HOST/packages/$i.x --root=apt --method=https --section=main \
	--nosource --ignore-missing-release --ignore-release-gpg \
	--no-check-gpg --progress /mirror/$REL_LOCAL_PATH/$i/
done >> $LOGFILE 2>&1

forcesetstamp
compresslog
