#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

loadscripts
check4run

rsync -avHP --delete --timeout=60 pull-mirror.yandex.net::yandexrepo/ /storage/yandexrepo/

compresslog
