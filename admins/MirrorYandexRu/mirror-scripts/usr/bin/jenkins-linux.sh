#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/jenkins"
HOST="ftp.nluug.nl"
MODULE="jenkins"

REMOTE_TRACE="TIME"

EXCLUDE="
--exclude=repodata/
"

loadscripts
check4run
check4available
rsync_check

if [ "$RUNSYNC" == "1" ]; then
    simple_sync

    # reindex redhat repos
    for repo in `ls -d /mirror/mirrors/jenkins/redhat*`; do
        /usr/sbin/chroot /var/cache/fedora-21-chroot createrepo_c -d --xz --update $repo >> $LOGFILE
    done

    echo "$(date -u)" > $FULL_LOCAL_PATH$YA_TRACE
fi

compresslog
