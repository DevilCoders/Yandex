#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/ubuntu-cloud"
HOST="ubuntu-cloud.archive.canonical.com"
SYNC_REPO_PORT="80"

loadscripts
check4run
check4available

DISTS="precise-proposed/cloud-tools,precise-proposed/folsom,precise-proposed/grizzly,precise-proposed/havana,precise-proposed/icehouse,precise-updates/cloud-tools,precise-updates/folsom,precise-updates/grizzly,precise-updates/havana,precise-updates/icehouse,trusty-proposed/juno,trusty-proposed/kilo,trusty-proposed/liberty,trusty-proposed/mitaka,trusty-updates/juno,trusty-updates/kilo,trusty-updates/liberty,trusty-updates/mitaka"

debmirror --ignore-release-gpg \
	--host=$HOST \
	--method=http \
	--root=ubuntu \
	--dist=$DISTS \
	--arch=i386,amd64 \
	--source \
	--progress \
	--postcleanup \
	/mirror/$REL_LOCAL_PATH/ >> $LOGFILE 2>&1

forcesetstamp
compresslog
