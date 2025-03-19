#!/bin/bash     

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"     
    exit 112
fi

SYNC_REPO_PORT="80"
REL_LOCAL_PATH="mirrors/docker"
HOST="download.docker.com"
MODULE=""
DOCKER_DIST="$(echo `curl -s https://$HOST/linux/ubuntu/dists/ | awk -F\" '/href/ { print $2 }' | sed 's@/$@@g;1d'` | sed 's@ @,@g')"

loadscripts
check4run
check4available

debmirror --dist=$DOCKER_DIST --host=$HOST --root=linux/ubuntu --arch=amd64 --method=http --section=stable,edge,nightly,test --progress --nosource --ignore-release-gpg --getcontents --postcleanup --rsync-options="--timeout=1" /mirror/$REL_LOCAL_PATH/ >> $LOGFILE 2>&1

forcesetstamp
compresslog
