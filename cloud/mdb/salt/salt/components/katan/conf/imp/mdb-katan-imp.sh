#!/usr/bin/env bash

LOGS=/var/log/mdb-katan/imp
/opt/yandex/mdb-katan/bin/mdb-katan-imp \
    /etc/yandex/mdb-katan/imp/imp.yaml >> $LOGS/stdout.log 2>> $LOGS/stderr.log

echo "$?" > /var/run/mdb-katan/imp/last-exit-status
