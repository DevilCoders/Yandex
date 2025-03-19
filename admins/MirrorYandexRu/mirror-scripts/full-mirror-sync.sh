#!/bin/bash

LANG=C

HOST="$1"

if [ $(id -u) -ne 0 ]; then
    echo "Root is required. Abort..."
    exit 0
fi

if [ "x$HOST" == "x" ]; then
    echo "No host selected. Abort..."
    exit 1
fi

#if [ ! "$(mount | grep '/storage/mirror on /mirror')" ]; then
#    echo "/mirror is not mounted"
#    echo "See https://wiki.yandex-team.ru/MirrorYandexRU"
#    exit 1
#fi

LOGFILE="/tmp/full-sync-$(date +%Y%m%d"-"%H%M%S).log"

echo "Please, run me in SCREEN"
sleep 5

echo "Stopping cron"
/etc/init.d/cron stop

echo "Logging to $LOGFILE"

for j in {1..2}; do
    echo "RUN $j"
    for i in `rsync $HOST::full-archive/ | grep "^d" | awk '{ print $NF }' | sed '1,2d'`; do
        echo "    --> Syncing $i"
        ls /fdjfjfjfjjff > /dev/null 2>&1
        while [ "$?" -ne "0" -a "$?" -ne "24" -a "$?" -ne 23 ]; do
            sleep 2
            rsync -avHP --delete --timeout=60 $HOST::full-archive/$i/ /mirror/$i/ >> $LOGFILE 2>&1
        done
    done
done

rsync -avHP --delete --timeout=60 pull-mirror.yandex.net::yandexrepo/ /storage/yandexrepo/

echo "Starting cron"
/etc/init.d/cron start

if [ -f $LOGFILE ]; then
    xz -9 $LOGFILE
fi
