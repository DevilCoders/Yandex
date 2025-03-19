#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/librealsense.intel.com/"
HOST="librealsense.intel.com"
SYNC_REPO_PORT="80"

mkdir -p /mirror/$REL_LOCAL_PATH/Debian/apt-repo/

loadscripts
check4run
check4available

debmirror --i18n --arch=amd64 --dist=focal --host=$HOST/Debian --root=apt-repo \
	--method=https --section=main --nosource --ignore-missing-release \
	--ignore-release-gpg --no-check-gpg --progress \
	/mirror/$REL_LOCAL_PATH/Debian/apt-repo/ >> $LOGFILE 2>&1

forcesetstamp
compresslog
