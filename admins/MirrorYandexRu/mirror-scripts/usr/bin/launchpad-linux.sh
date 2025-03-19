#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/launchpad"
HOST="ppa.launchpad.net"
SYNC_REPO_PORT="80"
REPOS="ansible/ansible git-core/ppa ondrej/php adiscon/v8-stable graphics-drivers/ppa longsleep/golang-backports mc3man/trusty-media saltstack/salt saltstack/salt-depends saltstack/salt-testing saltstack/salt-daily ubuntu-toolchain-r/test trevorjay/pyflame deadsnakes/ppa hnakamur/wrk wireguard/wireguard trevorjay/pyflame hnakamur/wrk"

loadscripts
check4run
check4available


for repo in $REPOS; do
    mkdir -p /mirror/$REL_LOCAL_PATH/$repo/
    DISTS=$(curl -s http://$HOST/$repo/ubuntu/dists/ | grep -v devel | grep -Po 'href="\K\w+'| xargs echo | sed 's/\ /,/g')
    echo "repos $DISTS"
    debmirror --i18n --arch=amd64,i386 --dist=$DISTS --host=$HOST/$repo/ \
        --root=ubuntu/ --method=http --section=main --ignore-missing-release \
        --ignore-release-gpg --progress /mirror/$REL_LOCAL_PATH/$repo/
done >> $LOGFILE 2>&1

forcesetstamp
compresslog
