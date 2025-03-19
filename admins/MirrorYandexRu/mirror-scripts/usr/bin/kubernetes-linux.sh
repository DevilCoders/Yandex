#!/bin/bash     

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"     
    exit 112
fi

SYNC_REPO_PORT="80"
REL_LOCAL_PATH="mirrors/kubernetes"
HOST="packages.cloud.google.com"
MODULE=""
KUBERNETES_DIST="$(echo `curl -s https://$HOST/apt/dists/ | awk -F\" '/href/ { print $2 }' | sed 's@/$@@g;1d' | sed 's@/$@@g;1d' | awk -F/ '{ print $NF }' | grep -E 'kubernetes|minikube'` | sed 's@ @,@g')"

loadscripts
check4run
check4available

debmirror --dist=$KUBERNETES_DIST --host=$HOST --root=apt --arch=amd64 --method=http --section=main --progress --nosource --ignore-release-gpg --getcontents --postcleanup --rsync-options="--timeout=1" /mirror/$REL_LOCAL_PATH/ >> $LOGFILE 2>&1

forcesetstamp
compresslog
