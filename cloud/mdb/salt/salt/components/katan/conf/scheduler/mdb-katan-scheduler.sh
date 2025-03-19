#!/usr/bin/env bash

LOGS=/var/log/mdb-katan/scheduler
/opt/yandex/mdb-katan/bin/mdb-katan-scheduler \
    --config=/etc/yandex/mdb-katan/scheduler/scheduler.yaml >> $LOGS/stdout.log 2>> $LOGS/stderr.log

echo "$?" > /var/run/mdb-katan/scheduler/last-exit-status
