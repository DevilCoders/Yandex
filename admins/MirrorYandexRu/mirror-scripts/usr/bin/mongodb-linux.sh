#!/bin/bash
# https://st.yandex-team.ru/MIRROR-15705
if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/repo.mongodb.org"
HOST="scotsnews.ru"
MODULE="repo.mongodb.org"


# https://yav.yandex-team.ru/secret/sec-01g62s83a5j38epck3gjchkha5/explore/versions
export RSYNC_PASSWORD="***"
REPO_USER="yandex"

REMOTE_TRACE="lastsync"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog
