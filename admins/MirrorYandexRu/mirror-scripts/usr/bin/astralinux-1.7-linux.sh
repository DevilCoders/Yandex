#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/astralinux/stable/1.7_x86-64"
HOST="download.astralinux.ru"
SYNC_REPO_PORT="80"
REPOS="repository-base repository-main repository-update repository-extended"

loadscripts
check4run
check4available


for repo in $REPOS; do
    mkdir -p /mirror/mirrors/astralinux/stable/1.7_x86-64/$repo/

    debmirror --i18n --arch=amd64 --dist=1.7_x86-64 \
	    --host=$HOST/astra/stable/1.7_x86-64 --root=$repo \
	    --method=http --nosource --section=main,contrib,non-free --ignore-missing-release \
	    --ignore-release-gpg --progress /mirror/$REL_LOCAL_PATH/$repo/ >> $LOGFILE 2>&1

done

forcesetstamp
compresslog
