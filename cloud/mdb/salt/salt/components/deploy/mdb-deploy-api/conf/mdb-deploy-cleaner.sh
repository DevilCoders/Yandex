#!/usr/bin/env bash

LOGS=/var/log/mdb-deploy-cleaner
/opt/yandex/mdb-deploy-cleaner/mdb-deploy-cleaner \
    --config-path=/etc/yandex/mdb-deploy-cleaner \
    --limit=10000 \
    --wait-after-remove=10s >> $LOGS/stdout.log 2>> $LOGS/stderr.log

echo "$?" > /var/run/mdb-deploy-cleaner/last-exit-status
